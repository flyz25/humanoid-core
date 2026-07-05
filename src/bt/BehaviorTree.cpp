#include <humanoid/bt/BehaviorTree.h>

#include <chrono>
#include <exception>
#include <utility>

namespace humanoid::bt {

namespace {

runtime::ExecutionState ToExecutionState(BTStatus status) noexcept {
  switch (status) {
  case BTStatus::Idle:
    return runtime::ExecutionState::Created;
  case BTStatus::Running:
    return runtime::ExecutionState::Running;
  case BTStatus::Success:
    return runtime::ExecutionState::Completed;
  case BTStatus::Failure:
    return runtime::ExecutionState::Failed;
  case BTStatus::Aborted:
    return runtime::ExecutionState::Aborted;
  }

  return runtime::ExecutionState::Failed;
}

} // namespace

BehaviorTree::BehaviorTree(std::unique_ptr<BTNode> root, BTContext context)
    : root_(std::move(root)), context_(std::move(context)) {}

BehaviorTree::~BehaviorTree() noexcept { (void)Shutdown(); }

BTStatus BehaviorTree::Initialize() {
  std::lock_guard<std::mutex> lock{mutex_};
  return InitializeLocked();
}

BTStatus BehaviorTree::Tick() {
  std::lock_guard<std::mutex> lock{mutex_};
  if (shutdown_ || !root_) {
    return AbortLocked();
  }

  if (isTerminal(status_)) {
    return status_;
  }

  if (!initialized_) {
    const BTStatus initialized_status = InitializeLocked();
    if (initialized_status != BTStatus::Idle) {
      return initialized_status;
    }
  }

  if (context_.CancellationRequested()) {
    return AbortLocked();
  }

  try {
    const std::shared_ptr<runtime::ExecutionContext> execution = context_.Execution();
    execution->SetState(runtime::ExecutionState::Running);
    status_ = root_->Tick(context_);
    ApplyRuntimeStateLocked(status_);
    return status_;
  } catch (const std::exception&) {
    status_ = BTStatus::Failure;
    ApplyRuntimeStateLocked(status_);
    return status_;
  } catch (...) {
    status_ = BTStatus::Failure;
    ApplyRuntimeStateLocked(status_);
    return status_;
  }
}

bool BehaviorTree::Reset() {
  std::lock_guard<std::mutex> lock{mutex_};
  if (shutdown_ || !root_) {
    return false;
  }

  try {
    root_->Reset(context_);
    status_ = BTStatus::Idle;
    initialized_ = false;
    ApplyRuntimeStateLocked(status_);
    return true;
  } catch (const std::exception&) {
    status_ = BTStatus::Failure;
    ApplyRuntimeStateLocked(status_);
    return false;
  } catch (...) {
    status_ = BTStatus::Failure;
    ApplyRuntimeStateLocked(status_);
    return false;
  }
}

bool BehaviorTree::Shutdown() noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  if (shutdown_) {
    return false;
  }

  if (root_) {
    try {
      root_->Shutdown(context_);
    } catch (...) {
      status_ = BTStatus::Aborted;
    }
  }

  if (status_ == BTStatus::Running) {
    status_ = BTStatus::Aborted;
  }
  initialized_ = false;
  shutdown_ = true;
  ApplyRuntimeStateLocked(status_);
  return true;
}

BTStatus BehaviorTree::Status() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return status_;
}

bool BehaviorTree::HasRoot() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return static_cast<bool>(root_);
}

BTContext& BehaviorTree::Context() noexcept { return context_; }

const BTContext& BehaviorTree::Context() const noexcept { return context_; }

BTStatus BehaviorTree::InitializeLocked() {
  if (shutdown_ || !root_) {
    return AbortLocked();
  }

  if (initialized_) {
    return status_;
  }

  try {
    const std::shared_ptr<runtime::ExecutionContext> execution = context_.Execution();
    execution->SetState(runtime::ExecutionState::Starting);
    if (!execution->StartTimestamp().has_value()) {
      execution->SetStartTimestamp(
          std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()));
    }
    status_ = root_->Initialize(context_);
    initialized_ = true;
    ApplyRuntimeStateLocked(status_);
    return status_;
  } catch (const std::exception&) {
    status_ = BTStatus::Failure;
    ApplyRuntimeStateLocked(status_);
    return status_;
  } catch (...) {
    status_ = BTStatus::Failure;
    ApplyRuntimeStateLocked(status_);
    return status_;
  }
}

BTStatus BehaviorTree::AbortLocked() noexcept {
  status_ = BTStatus::Aborted;
  ApplyRuntimeStateLocked(status_);
  return status_;
}

void BehaviorTree::ApplyRuntimeStateLocked(BTStatus status) noexcept {
  try {
    const std::shared_ptr<runtime::ExecutionContext> execution = context_.Execution();
    if (execution) {
      execution->SetState(ToExecutionState(status));
    }
  } catch (...) {
  }
}

} // namespace humanoid::bt
