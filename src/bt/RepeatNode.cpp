#include <humanoid/bt/RepeatNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

RepeatNode::RepeatNode(std::string name, std::size_t repeat_count, std::unique_ptr<BTNode> child)
    : DecoratorNode(std::move(name), std::move(child)), repeat_count_(repeat_count) {}

BTStatus RepeatNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    completed_count_ = 0U;
    return BTStatus::Aborted;
  }
  if (!child_) {
    return BTStatus::Failure;
  }

  const BTStatus status = child_->Tick(context);
  switch (status) {
  case BTStatus::Success:
    ++completed_count_;
    child_->Reset(context);
    if (repeat_count_ != 0U && completed_count_ >= repeat_count_) {
      completed_count_ = 0U;
      return BTStatus::Success;
    }
    return BTStatus::Running;
  case BTStatus::Failure:
    completed_count_ = 0U;
    return BTStatus::Failure;
  case BTStatus::Aborted:
    completed_count_ = 0U;
    return BTStatus::Aborted;
  case BTStatus::Idle:
  case BTStatus::Running:
    return BTStatus::Running;
  }

  return BTStatus::Failure;
}

void RepeatNode::OnInitializeLocked() { completed_count_ = 0U; }

void RepeatNode::OnResetLocked() { completed_count_ = 0U; }

} // namespace humanoid::bt
