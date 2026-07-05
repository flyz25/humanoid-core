#include <humanoid/bt/DecoratorNode.h>

#include <utility>

namespace humanoid::bt {

DecoratorNode::DecoratorNode(std::string name, std::unique_ptr<BTNode> child)
    : child_(std::move(child)), name_(std::move(name)) {
  if (name_.empty()) {
    name_ = "Decorator";
  }
}

std::string_view DecoratorNode::Name() const noexcept { return name_; }

BTStatus DecoratorNode::Initialize(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  OnInitializeLocked();
  if (!child_) {
    return BTStatus::Failure;
  }
  return child_->Initialize(context);
}

void DecoratorNode::Reset(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  OnResetLocked();
  if (child_) {
    child_->Reset(context);
  }
}

void DecoratorNode::Shutdown(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  OnResetLocked();
  if (child_) {
    try {
      child_->Shutdown(context);
    } catch (...) {
    }
  }
}

bool DecoratorNode::SetChild(std::unique_ptr<BTNode> child) {
  if (!child) {
    return false;
  }
  std::lock_guard<std::mutex> lock{mutex_};
  child_ = std::move(child);
  OnResetLocked();
  return true;
}

bool DecoratorNode::HasChild() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return static_cast<bool>(child_);
}

void DecoratorNode::OnInitializeLocked() {}

void DecoratorNode::OnResetLocked() {}

} // namespace humanoid::bt
