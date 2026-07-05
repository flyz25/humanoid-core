#include <humanoid/bt/DelayNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

DelayNode::DelayNode(std::string name, std::chrono::nanoseconds duration)
    : name_(std::move(name)), duration_(duration) {
  if (name_.empty()) {
    name_ = "Delay";
  }
}

std::string_view DelayNode::Name() const noexcept { return name_; }

BTStatus DelayNode::Initialize(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  ClearTimer();
  return BTStatus::Idle;
}

BTStatus DelayNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    ClearTimer();
    return BTStatus::Aborted;
  }
  if (completed_) {
    return BTStatus::Success;
  }
  if (duration_ <= std::chrono::nanoseconds{0}) {
    completed_ = true;
    return BTStatus::Success;
  }

  const auto now = std::chrono::steady_clock::now();
  if (!start_time_.has_value()) {
    start_time_ = now;
    return BTStatus::Running;
  }
  if (now - *start_time_ >= duration_) {
    completed_ = true;
    ClearTimer();
    return BTStatus::Success;
  }
  return BTStatus::Running;
}

void DelayNode::Reset(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  ClearTimer();
}

void DelayNode::Shutdown(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  ClearTimer();
}

void DelayNode::ClearTimer() noexcept {
  start_time_.reset();
  completed_ = false;
}

} // namespace humanoid::bt
