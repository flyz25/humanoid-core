#include <humanoid/mission/MissionExecutor.h>

#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandResult.h>

namespace humanoid::mission {
namespace {

[[nodiscard]] MissionResult Result(MissionStatus status, std::string message) {
  MissionResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] core::CommandTimestamp Now() noexcept {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
}

[[nodiscard]] core::Command PrepareCommand(const MissionStep& step) {
  core::Command command = step.command;
  command.timestamp = Now();
  if (step.hasTimeout() && (!command.hasTimeout() || step.timeout < command.timeout)) {
    command.timeout = step.timeout;
  }
  return command;
}

[[nodiscard]] MissionResult FromCommandResult(const core::CommandResult& command_result) {
  switch (command_result.status) {
  case core::CommandStatus::Completed:
    return Result(MissionStatus::Completed, command_result.message);
  case core::CommandStatus::Cancelled:
    return Result(MissionStatus::Cancelled, command_result.message);
  case core::CommandStatus::Pending:
  case core::CommandStatus::Queued:
  case core::CommandStatus::Running:
  case core::CommandStatus::Failed:
  case core::CommandStatus::Timeout:
  case core::CommandStatus::Rejected:
    return Result(MissionStatus::Failed, command_result.message);
  }

  return Result(MissionStatus::Failed, "Command returned an unknown status");
}

} // namespace

class MissionExecutor::Impl final {
public:
  explicit Impl(std::shared_ptr<core::CommandDispatcher> dispatcher)
      : dispatcher_(std::move(dispatcher)) {}

  ~Impl() noexcept {
    try {
      static_cast<void>(Stop());
    } catch (...) {
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] MissionResult Start(Mission mission) {
    std::lock_guard<std::mutex> control_lock{control_mutex_};
    JoinCompletedWorker();

    if (!dispatcher_) {
      return Result(MissionStatus::Failed, "Mission executor has no command dispatcher");
    }
    if (!mission.isValid()) {
      return Result(MissionStatus::Failed, "Mission is not valid for execution");
    }
    if (worker_.joinable()) {
      return Result(MissionStatus::Failed, "Mission executor is already active");
    }

    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      pause_requested_ = false;
      cancel_requested_ = false;
      stop_requested_ = false;
      current_step_index_.reset();
      current_step_id_.reset();
      current_command_id_.reset();
      status_ = MissionStatus::Running;
      last_result_ = Result(MissionStatus::Running, "Mission execution started");
    }

    try {
      worker_ = std::jthread([this, mission = std::move(mission)]() { RunMission(mission); });
    } catch (const std::exception& exception) {
      std::lock_guard<std::mutex> lock{state_mutex_};
      status_ = MissionStatus::Failed;
      last_result_ = Result(MissionStatus::Failed,
                            std::string{"Unable to start mission worker: "} + exception.what());
      return last_result_;
    } catch (...) {
      std::lock_guard<std::mutex> lock{state_mutex_};
      status_ = MissionStatus::Failed;
      last_result_ = Result(MissionStatus::Failed, "Unable to start mission worker");
      return last_result_;
    }

    return Result(MissionStatus::Running, "Mission execution started");
  }

  [[nodiscard]] MissionResult Pause() {
    std::lock_guard<std::mutex> lock{state_mutex_};
    if (status_ == MissionStatus::Paused) {
      return last_result_;
    }
    if (status_ != MissionStatus::Running) {
      return Result(MissionStatus::Failed, "Mission is not running");
    }

    pause_requested_ = true;
    status_ = MissionStatus::Paused;
    last_result_ = Result(MissionStatus::Paused, "Mission pause requested");
    return last_result_;
  }

  [[nodiscard]] MissionResult Resume() {
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      if (status_ != MissionStatus::Paused) {
        return Result(MissionStatus::Failed, "Mission is not paused");
      }

      pause_requested_ = false;
      status_ = MissionStatus::Running;
      last_result_ = Result(MissionStatus::Running, "Mission resumed");
    }

    condition_.notify_all();
    return Result(MissionStatus::Running, "Mission resumed");
  }

  [[nodiscard]] MissionResult Cancel() {
    std::optional<core::CommandId> command_to_cancel;
    MissionResult result;
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      if (status_ == MissionStatus::Pending || isTerminal(status_)) {
        return Result(MissionStatus::Failed, "No active mission to cancel");
      }

      cancel_requested_ = true;
      pause_requested_ = false;
      status_ = MissionStatus::Cancelled;
      last_result_ = Result(MissionStatus::Cancelled, "Mission cancellation requested");
      result = last_result_;
      command_to_cancel = current_command_id_;
    }

    condition_.notify_all();
    CancelCommandIfPossible(command_to_cancel);
    return result;
  }

  [[nodiscard]] MissionResult Stop() {
    std::lock_guard<std::mutex> control_lock{control_mutex_};

    std::optional<core::CommandId> command_to_cancel;
    bool preserve_terminal_result = false;
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      if (!worker_.joinable()) {
        if (!isTerminal(status_)) {
          status_ = MissionStatus::Pending;
          last_result_ = Result(MissionStatus::Completed, "Mission executor stopped");
        }
        return last_result_;
      }

      if (isTerminal(status_)) {
        preserve_terminal_result = true;
      } else {
        stop_requested_ = true;
        cancel_requested_ = true;
        pause_requested_ = false;
        status_ = MissionStatus::Cancelled;
        last_result_ = Result(MissionStatus::Cancelled, "Mission stop requested");
        command_to_cancel = current_command_id_;
      }
    }

    if (!preserve_terminal_result) {
      condition_.notify_all();
      CancelCommandIfPossible(command_to_cancel);
    }
    if (worker_.joinable()) {
      worker_.join();
    }

    return GetLastResult();
  }

  [[nodiscard]] MissionResult ExecuteStep(const MissionStep& step) {
    try {
      return ExecuteStepWithRetry(step);
    } catch (const std::exception& exception) {
      return Result(MissionStatus::Failed,
                    std::string{"Mission step execution failed: "} + exception.what());
    } catch (...) {
      return Result(MissionStatus::Failed, "Mission step execution failed with an unknown error");
    }
  }

  [[nodiscard]] MissionStatus GetStatus() const {
    std::lock_guard<std::mutex> lock{state_mutex_};
    return status_;
  }

  [[nodiscard]] std::optional<MissionStepIndex> GetCurrentStepIndex() const {
    std::lock_guard<std::mutex> lock{state_mutex_};
    return current_step_index_;
  }

  [[nodiscard]] std::optional<MissionStepId> GetCurrentStepId() const {
    std::lock_guard<std::mutex> lock{state_mutex_};
    return current_step_id_;
  }

  [[nodiscard]] MissionResult GetLastResult() const {
    std::lock_guard<std::mutex> lock{state_mutex_};
    return last_result_;
  }

private:
  void JoinCompletedWorker() {
    bool should_join = false;
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      should_join = worker_.joinable() && isTerminal(status_);
    }

    if (should_join && worker_.joinable()) {
      worker_.join();
    }
  }

  [[nodiscard]] bool WaitUntilRunnable() {
    std::unique_lock<std::mutex> lock{state_mutex_};
    condition_.wait(lock,
                    [this]() { return !pause_requested_ || cancel_requested_ || stop_requested_; });
    if (cancel_requested_ || stop_requested_) {
      return false;
    }

    status_ = MissionStatus::Running;
    last_result_ = Result(MissionStatus::Running, "Mission execution running");
    return true;
  }

  void RunMission(const Mission& mission) {
    try {
      for (MissionStepIndex index = 0U; index < mission.steps.size(); ++index) {
        const MissionStep& step = mission.steps[index];
        if (!step.enabled) {
          continue;
        }
        if (!WaitUntilRunnable()) {
          Finish(MissionStatus::Cancelled, "Mission cancelled before next step");
          return;
        }

        {
          std::lock_guard<std::mutex> lock{state_mutex_};
          current_step_index_ = index;
          current_step_id_ = step.id;
          current_command_id_ = step.command.id;
        }

        const MissionResult step_result = ExecuteStepWithRetry(step);
        {
          std::lock_guard<std::mutex> lock{state_mutex_};
          current_command_id_.reset();
        }

        if (CancellationRequested()) {
          Finish(MissionStatus::Cancelled, "Mission cancelled");
          return;
        }
        if (!step_result.isSuccess()) {
          Finish(MissionStatus::Failed, step_result.message);
          return;
        }
      }

      Finish(MissionStatus::Completed, "Mission completed");
    } catch (const std::exception& exception) {
      Finish(MissionStatus::Failed, std::string{"Mission execution failed: "} + exception.what());
    } catch (...) {
      Finish(MissionStatus::Failed, "Mission execution failed with an unknown error");
    }
  }

  [[nodiscard]] MissionResult ExecuteStepWithRetry(const MissionStep& step) {
    if (!dispatcher_) {
      return Result(MissionStatus::Failed, "Mission executor has no command dispatcher");
    }
    if (!step.isValid()) {
      return Result(MissionStatus::Failed, "Mission step is not valid for execution");
    }

    MissionResult result = Result(MissionStatus::Failed, "Mission step did not execute");
    for (MissionStepRetryCount attempt = 0U; attempt <= step.retry; ++attempt) {
      if (CancellationRequested()) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled before execution");
      }

      result = ExecuteStepOnce(step);
      if (result.isSuccess() || result.status == MissionStatus::Cancelled ||
          attempt == step.retry) {
        return result;
      }
    }

    return result;
  }

  [[nodiscard]] MissionResult ExecuteStepOnce(const MissionStep& step) {
    core::Command command = PrepareCommand(step);
    const core::CommandResult command_result = dispatcher_->Execute(command);
    MissionResult result = FromCommandResult(command_result);
    if (result.message.empty()) {
      result.message = result.isSuccess() ? "Mission step completed" : "Mission step failed";
    }
    return result;
  }

  [[nodiscard]] bool CancellationRequested() const {
    std::lock_guard<std::mutex> lock{state_mutex_};
    return cancel_requested_ || stop_requested_;
  }

  void Finish(MissionStatus status, std::string message) {
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      status_ = status;
      last_result_ = Result(status, std::move(message));
      current_command_id_.reset();
    }
    condition_.notify_all();
  }

  void CancelCommandIfPossible(std::optional<core::CommandId> command_id) {
    if (dispatcher_ && command_id.has_value()) {
      try {
        static_cast<void>(dispatcher_->Cancel(*command_id));
      } catch (...) {
      }
    }
  }

  std::shared_ptr<core::CommandDispatcher> dispatcher_;
  mutable std::mutex state_mutex_;
  std::mutex control_mutex_;
  std::condition_variable condition_;
  std::jthread worker_;
  MissionStatus status_{MissionStatus::Pending};
  MissionResult last_result_;
  std::optional<MissionStepIndex> current_step_index_;
  std::optional<MissionStepId> current_step_id_;
  std::optional<core::CommandId> current_command_id_;
  bool pause_requested_{false};
  bool cancel_requested_{false};
  bool stop_requested_{false};
};

MissionExecutor::MissionExecutor(std::shared_ptr<core::CommandDispatcher> dispatcher)
    : impl_(std::make_unique<Impl>(std::move(dispatcher))) {}

MissionExecutor::~MissionExecutor() noexcept = default;

MissionResult MissionExecutor::Start(Mission mission) { return impl_->Start(std::move(mission)); }

MissionResult MissionExecutor::Pause() { return impl_->Pause(); }

MissionResult MissionExecutor::Resume() { return impl_->Resume(); }

MissionResult MissionExecutor::Cancel() { return impl_->Cancel(); }

MissionResult MissionExecutor::Stop() { return impl_->Stop(); }

MissionResult MissionExecutor::ExecuteStep(const MissionStep& step) {
  return impl_->ExecuteStep(step);
}

MissionStatus MissionExecutor::GetStatus() const { return impl_->GetStatus(); }

std::optional<MissionStepIndex> MissionExecutor::GetCurrentStepIndex() const {
  return impl_->GetCurrentStepIndex();
}

std::optional<MissionStepId> MissionExecutor::GetCurrentStepId() const {
  return impl_->GetCurrentStepId();
}

MissionResult MissionExecutor::GetLastResult() const { return impl_->GetLastResult(); }

} // namespace humanoid::mission
