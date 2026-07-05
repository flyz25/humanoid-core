#include <humanoid/bt/SequenceNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

SequenceNode::SequenceNode(std::string name, SequenceMemoryPolicy memory_policy,
                           BTChildren children)
    : CompositeNode(std::move(name), std::move(children)), memory_policy_(memory_policy) {}

BTStatus SequenceNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    current_index_ = 0U;
    return BTStatus::Aborted;
  }

  if (children_.empty()) {
    current_index_ = 0U;
    return BTStatus::Success;
  }

  std::size_t index = memory_policy_ == SequenceMemoryPolicy::Memory ? current_index_
                                                                     : static_cast<std::size_t>(0U);
  while (index < children_.size()) {
    const BTStatus status = children_[index]->Tick(context);
    switch (status) {
    case BTStatus::Success:
      ++index;
      break;
    case BTStatus::Running:
      current_index_ = memory_policy_ == SequenceMemoryPolicy::Memory ? index : 0U;
      return BTStatus::Running;
    case BTStatus::Failure:
      current_index_ = 0U;
      return BTStatus::Failure;
    case BTStatus::Aborted:
      current_index_ = 0U;
      return BTStatus::Aborted;
    case BTStatus::Idle:
      current_index_ = memory_policy_ == SequenceMemoryPolicy::Memory ? index : 0U;
      return BTStatus::Running;
    }
  }

  current_index_ = 0U;
  return BTStatus::Success;
}

void SequenceNode::OnInitializeLocked() { current_index_ = 0U; }

void SequenceNode::OnResetLocked() { current_index_ = 0U; }

} // namespace humanoid::bt
