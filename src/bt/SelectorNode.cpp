#include <humanoid/bt/SelectorNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

SelectorNode::SelectorNode(std::string name, SelectorMemoryPolicy memory_policy,
                           BTChildren children)
    : CompositeNode(std::move(name), std::move(children)), memory_policy_(memory_policy) {}

BTStatus SelectorNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    current_index_ = 0U;
    return BTStatus::Aborted;
  }

  if (children_.empty()) {
    current_index_ = 0U;
    return BTStatus::Failure;
  }

  std::size_t index = memory_policy_ == SelectorMemoryPolicy::Memory ? current_index_
                                                                     : static_cast<std::size_t>(0U);
  while (index < children_.size()) {
    const BTStatus status = children_[index]->Tick(context);
    switch (status) {
    case BTStatus::Failure:
      ++index;
      break;
    case BTStatus::Running:
      current_index_ = memory_policy_ == SelectorMemoryPolicy::Memory ? index : 0U;
      return BTStatus::Running;
    case BTStatus::Success:
      current_index_ = 0U;
      return BTStatus::Success;
    case BTStatus::Aborted:
      current_index_ = 0U;
      return BTStatus::Aborted;
    case BTStatus::Idle:
      current_index_ = memory_policy_ == SelectorMemoryPolicy::Memory ? index : 0U;
      return BTStatus::Running;
    }
  }

  current_index_ = 0U;
  return BTStatus::Failure;
}

void SelectorNode::OnInitializeLocked() { current_index_ = 0U; }

void SelectorNode::OnResetLocked() { current_index_ = 0U; }

} // namespace humanoid::bt
