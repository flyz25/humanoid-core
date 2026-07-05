#include <humanoid/bt/LimiterNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

LimiterNode::LimiterNode(std::string name, std::size_t max_ticks, std::unique_ptr<BTNode> child)
    : DecoratorNode(std::move(name), std::move(child)), max_ticks_(max_ticks) {}

BTStatus LimiterNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    return BTStatus::Aborted;
  }
  if (!child_) {
    return BTStatus::Failure;
  }
  if (tick_count_ >= max_ticks_) {
    return BTStatus::Failure;
  }

  ++tick_count_;
  return child_->Tick(context);
}

void LimiterNode::OnInitializeLocked() { tick_count_ = 0U; }

void LimiterNode::OnResetLocked() { tick_count_ = 0U; }

} // namespace humanoid::bt
