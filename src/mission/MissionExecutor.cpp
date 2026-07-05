#include <humanoid/mission/MissionExecutor.h>

#include <chrono>
#include <condition_variable>
#include <exception>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/mission/ConditionEvaluator.h>

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

[[nodiscard]] bool IsCommandStep(const MissionStep& step) noexcept {
  return !step.skip && !step.abort && !step.wait.has_value() && !step.delay.has_value();
}

[[nodiscard]] std::optional<MissionStepTimeout> EffectiveTimeout(const MissionStep& step) {
  std::optional<MissionStepTimeout> timeout;
  if (step.hasTimeout()) {
    timeout = step.timeout;
  }
  if (step.timeoutPolicy.isEnabled() &&
      (!timeout.has_value() || step.timeoutPolicy.timeout < *timeout)) {
    timeout = step.timeoutPolicy.timeout;
  }
  return timeout;
}

[[nodiscard]] core::Command PrepareCommand(const MissionStep& step) {
  core::Command command = step.command;
  command.timestamp = Now();
  const std::optional<MissionStepTimeout> timeout = EffectiveTimeout(step);
  if (timeout.has_value() && (!command.hasTimeout() || *timeout < command.timeout)) {
    command.timeout = *timeout;
  }
  return command;
}

[[nodiscard]] RetryAttemptCount EffectiveMaxAttempts(const MissionStep& step) noexcept {
  const RetryAttemptCount legacy_attempts =
      step.retry == std::numeric_limits<MissionStepRetryCount>::max() ? step.retry
                                                                      : step.retry + 1U;
  return step.retryPolicy.maxAttempts > legacy_attempts ? step.retryPolicy.maxAttempts
                                                        : legacy_attempts;
}

[[nodiscard]] bool LooksLikeTimeout(const MissionResult& result) {
  return result.message.find("timeout") != std::string::npos ||
         result.message.find("timed out") != std::string::npos;
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
  Impl(std::shared_ptr<core::CommandDispatcher> dispatcher,
       std::shared_ptr<ConditionEvaluator> condition_evaluator)
      : dispatcher_(std::move(dispatcher)), condition_evaluator_(std::move(condition_evaluator)) {}

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
          current_command_id_ =
              IsCommandStep(step) ? std::optional<core::CommandId>{step.command.id} : std::nullopt;
        }

        const MissionResult step_result = ExecuteFlowStep(step);
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
    return ExecuteFlowStep(step);
  }

  [[nodiscard]] MissionResult ExecuteFlowStep(const MissionStep& step) {
    if (!dispatcher_) {
      return Result(MissionStatus::Failed, "Mission executor has no command dispatcher");
    }
    if (!step.isValid()) {
      return Result(MissionStatus::Failed, "Mission step is not valid for execution");
    }
    if (step.skip) {
      return Result(MissionStatus::Completed, "Mission step skipped");
    }
    if (step.abort) {
      return Result(MissionStatus::Failed, "Mission step requested abort");
    }

    const MissionResult condition_result = EvaluateStepConditions(step);
    if (!condition_result.isSuccess()) {
      return condition_result;
    }
    if (condition_result.message == "Mission step skipped by condition") {
      return condition_result;
    }

    MissionResult result = Result(MissionStatus::Completed, "Mission step completed");
    for (LoopIterationCount iteration = 0U; iteration < step.loopPolicy.iterations; ++iteration) {
      if (CancellationRequested()) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled before loop iteration");
      }

      if (!WaitUntilRunnable()) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled while paused");
      }

      result = ExecuteStepWithRetryPolicy(step);
      if (!result.isSuccess()) {
        return result;
      }
    }

    return result;
  }

  [[nodiscard]] MissionResult EvaluateStepConditions(const MissionStep& step) const {
    if (step.conditions.empty()) {
      return Result(MissionStatus::Completed, "Mission step conditions satisfied");
    }
    if (!condition_evaluator_) {
      return Result(MissionStatus::Failed, "Mission condition evaluator is unavailable");
    }

    for (const MissionCondition& condition : step.conditions) {
      const ConditionEvaluationResult evaluation = condition_evaluator_->Evaluate(condition);
      if (evaluation.Succeeded()) {
        continue;
      }

      if (condition.failureAction == MissionConditionFailureAction::Skip) {
        return Result(MissionStatus::Completed, "Mission step skipped by condition");
      }
      if (evaluation.evaluated) {
        return Result(MissionStatus::Failed, "Mission condition failed");
      }
      return Result(MissionStatus::Failed, evaluation.event.message);
    }

    return Result(MissionStatus::Completed, "Mission step conditions satisfied");
  }

  [[nodiscard]] MissionResult ExecuteStepWithRetryPolicy(const MissionStep& step) {
    MissionResult result = Result(MissionStatus::Failed, "Mission step did not execute");
    const RetryAttemptCount max_attempts = EffectiveMaxAttempts(step);
    for (RetryAttemptCount attempt = 0U; attempt < max_attempts; ++attempt) {
      if (CancellationRequested()) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled before execution");
      }

      if (!WaitUntilRunnable()) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled while paused");
      }

      result = ExecuteStepOnce(step);
      if (result.isSuccess() || result.status == MissionStatus::Cancelled ||
          attempt + 1U >= max_attempts) {
        return result;
      }
      const bool timeout_failure = LooksLikeTimeout(result);
      if ((timeout_failure && !step.retryPolicy.retryOnTimeout) ||
          (!timeout_failure && !step.retryPolicy.retryOnFailure)) {
        return result;
      }
      if (step.retryPolicy.delayBetweenAttempts > RetryDelay::zero() &&
          !WaitForDuration(step.retryPolicy.delayBetweenAttempts)) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled before retry");
      }
    }

    return result;
  }

  [[nodiscard]] MissionResult ExecuteStepOnce(const MissionStep& step) {
    if (step.wait.has_value()) {
      return ExecuteTimedFlowWait(step.wait->duration, step);
    }
    if (step.delay.has_value()) {
      return ExecuteTimedFlowWait(step.delay->duration, step);
    }

    core::Command command = PrepareCommand(step);
    const core::CommandResult command_result = dispatcher_->Execute(command);
    MissionResult result = FromCommandResult(command_result);
    if (result.message.empty()) {
      result.message = result.isSuccess() ? "Mission step completed" : "Mission step failed";
    }
    return result;
  }

  [[nodiscard]] MissionResult ExecuteTimedFlowWait(std::chrono::milliseconds duration,
                                                   const MissionStep& step) {
    const std::optional<MissionStepTimeout> timeout = EffectiveTimeout(step);
    if (timeout.has_value() && *timeout < duration) {
      if (!WaitForDuration(*timeout)) {
        return Result(MissionStatus::Cancelled, "Mission step cancelled during timed wait");
      }
      if (step.timeoutPolicy.abortOnTimeout) {
        return Result(MissionStatus::Failed, "Mission step timed out");
      }
      return Result(MissionStatus::Completed, "Mission step timed out and was skipped");
    }

    if (!WaitForDuration(duration)) {
      return Result(MissionStatus::Cancelled, "Mission step cancelled during timed wait");
    }
    return Result(MissionStatus::Completed, "Mission timed wait completed");
  }

  [[nodiscard]] bool WaitForDuration(std::chrono::milliseconds duration) {
    if (duration <= std::chrono::milliseconds::zero()) {
      return !CancellationRequested();
    }

    std::unique_lock<std::mutex> lock{state_mutex_};
    const bool interrupted = condition_.wait_for(
        lock, duration, [this]() { return cancel_requested_ || stop_requested_; });
    return !interrupted && !cancel_requested_ && !stop_requested_;
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
  std::shared_ptr<ConditionEvaluator> condition_evaluator_;
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
    : impl_(std::make_unique<Impl>(std::move(dispatcher), nullptr)) {}

MissionExecutor::MissionExecutor(std::shared_ptr<core::CommandDispatcher> dispatcher,
                                 std::shared_ptr<ConditionEvaluator> condition_evaluator)
    : impl_(std::make_unique<Impl>(std::move(dispatcher), std::move(condition_evaluator))) {}

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
