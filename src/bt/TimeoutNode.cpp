#include <humanoid/bt/TimeoutNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

TimeoutNode::TimeoutNode(std::string name, std::chrono::nanoseconds timeout,
                         std::unique_ptr<BTNode> child)
    : DecoratorNode(std::move(name), std::move(child)), timeout_(timeout) {}

BTStatus TimeoutNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    ClearTimer();
    return BTStatus::Aborted;
  }
  if (!child_) {
    return BTStatus::Failure;
  }

  const auto now = std::chrono::steady_clock::now();
  if (!start_time_.has_value()) {
    start_time_ = now;
  }
  if (TimedOut(now)) {
    child_->Reset(context);
    ClearTimer();
    return BTStatus::Failure;
  }

  const BTStatus status = child_->Tick(context);
  const auto after_tick = std::chrono::steady_clock::now();
  if ((status == BTStatus::Running || status == BTStatus::Idle) && TimedOut(after_tick)) {
    child_->Reset(context);
    ClearTimer();
    return BTStatus::Failure;
  }

  switch (status) {
  case BTStatus::Success:
  case BTStatus::Failure:
  case BTStatus::Aborted:
    ClearTimer();
    return status;
  case BTStatus::Idle:
  case BTStatus::Running:
    return BTStatus::Running;
  }

  ClearTimer();
  return BTStatus::Failure;
}

void TimeoutNode::OnInitializeLocked() { ClearTimer(); }

void TimeoutNode::OnResetLocked() { ClearTimer(); }

bool TimeoutNode::TimedOut(std::chrono::steady_clock::time_point now) const noexcept {
  if (!start_time_.has_value()) {
    return false;
  }
  if (timeout_ <= std::chrono::nanoseconds{0}) {
    return true;
  }
  return now - *start_time_ >= timeout_;
}

void TimeoutNode::ClearTimer() noexcept { start_time_.reset(); }

} // namespace humanoid::bt
