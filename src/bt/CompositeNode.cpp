#include <humanoid/bt/CompositeNode.h>

#include <algorithm>
#include <utility>

namespace humanoid::bt {

namespace {

void RemoveNullChildren(BTChildren& children) {
  const auto new_end =
      std::remove_if(children.begin(), children.end(),
                     [](const std::unique_ptr<BTNode>& child) { return child == nullptr; });
  children.erase(new_end, children.end());
}

} // namespace

CompositeNode::CompositeNode(std::string name, BTChildren children)
    : children_(std::move(children)), name_(std::move(name)) {
  if (name_.empty()) {
    name_ = "Composite";
  }
  RemoveNullChildren(children_);
}

std::string_view CompositeNode::Name() const noexcept { return name_; }

BTStatus CompositeNode::Initialize(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  OnInitializeLocked();
  for (const std::unique_ptr<BTNode>& child : children_) {
    const BTStatus status = child->Initialize(context);
    if (status == BTStatus::Aborted || status == BTStatus::Failure) {
      return status;
    }
  }
  return BTStatus::Idle;
}

void CompositeNode::Reset(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  OnResetLocked();
  for (const std::unique_ptr<BTNode>& child : children_) {
    child->Reset(context);
  }
}

void CompositeNode::Shutdown(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  OnResetLocked();
  for (const std::unique_ptr<BTNode>& child : children_) {
    try {
      child->Shutdown(context);
    } catch (...) {
    }
  }
}

bool CompositeNode::AddChild(std::unique_ptr<BTNode> child) {
  if (!child) {
    return false;
  }
  std::lock_guard<std::mutex> lock{mutex_};
  children_.push_back(std::move(child));
  return true;
}

std::size_t CompositeNode::ChildCount() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return children_.size();
}

void CompositeNode::OnInitializeLocked() {}

void CompositeNode::OnResetLocked() {}

} // namespace humanoid::bt
