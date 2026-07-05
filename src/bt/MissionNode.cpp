#include <humanoid/bt/MissionNode.h>

#include <exception>
#include <utility>

#include <humanoid/bt/BTContext.h>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionResult.h>
#include <humanoid/mission/MissionStatus.h>

namespace humanoid::bt {
namespace {

[[nodiscard]] BTStatus FromMissionStatus(mission::MissionStatus status) noexcept {
  switch (status) {
  case mission::MissionStatus::Completed:
    return BTStatus::Success;
  case mission::MissionStatus::Cancelled:
    return BTStatus::Aborted;
  case mission::MissionStatus::Failed:
    return BTStatus::Failure;
  case mission::MissionStatus::Pending:
  case mission::MissionStatus::Running:
  case mission::MissionStatus::Paused:
    return BTStatus::Running;
  }

  return BTStatus::Failure;
}

} // namespace

MissionNode::MissionNode(std::string name, std::shared_ptr<mission::MissionExecutor> executor,
                         mission::Mission mission)
    : name_(std::move(name)), executor_(std::move(executor)), mission_(std::move(mission)) {
  if (name_.empty()) {
    name_ = "Mission";
  }
}

MissionNode::~MissionNode() noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  StopActiveLocked();
}

std::string_view MissionNode::Name() const noexcept { return name_; }

BTStatus MissionNode::Initialize(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  terminal_status_.reset();
  started_ = false;
  return executor_ ? BTStatus::Idle : BTStatus::Failure;
}

BTStatus MissionNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (terminal_status_.has_value()) {
    return *terminal_status_;
  }
  if (context.CancellationRequested()) {
    if (executor_ && started_) {
      try {
        static_cast<void>(executor_->Cancel());
      } catch (...) {
      }
    }
    started_ = false;
    terminal_status_ = BTStatus::Aborted;
    return *terminal_status_;
  }
  if (!executor_) {
    terminal_status_ = BTStatus::Failure;
    return *terminal_status_;
  }

  try {
    if (!started_) {
      const mission::MissionResult start_result = executor_->Start(mission_);
      const BTStatus start_status = FromMissionStatus(start_result.status);
      if (start_status != BTStatus::Running) {
        terminal_status_ = start_status;
        return *terminal_status_;
      }
      started_ = true;
      return BTStatus::Running;
    }

    const BTStatus status = FromMissionStatus(executor_->GetStatus());
    if (status == BTStatus::Success || status == BTStatus::Failure || status == BTStatus::Aborted) {
      terminal_status_ = status;
      started_ = false;
      return *terminal_status_;
    }
    return BTStatus::Running;
  } catch (const std::exception&) {
    started_ = false;
    terminal_status_ = BTStatus::Failure;
    return *terminal_status_;
  } catch (...) {
    started_ = false;
    terminal_status_ = BTStatus::Failure;
    return *terminal_status_;
  }
}

void MissionNode::Reset(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  StopActiveLocked();
  terminal_status_.reset();
}

void MissionNode::Shutdown(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  StopActiveLocked();
}

void MissionNode::StopActiveLocked() noexcept {
  if (executor_ && started_) {
    try {
      static_cast<void>(executor_->Stop());
    } catch (...) {
    }
  }
  started_ = false;
}

} // namespace humanoid::bt
