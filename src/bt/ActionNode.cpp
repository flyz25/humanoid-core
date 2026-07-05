#include <humanoid/bt/ActionNode.h>

#include <exception>
#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

ActionNode::ActionNode(std::string name, ActionCallback callback)
    : name_(std::move(name)), callback_(std::move(callback)) {
  if (name_.empty()) {
    name_ = "Action";
  }
}

std::string_view ActionNode::Name() const noexcept { return name_; }

BTStatus ActionNode::Initialize(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  return callback_ ? BTStatus::Idle : BTStatus::Failure;
}

BTStatus ActionNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    return BTStatus::Aborted;
  }
  if (!callback_) {
    return BTStatus::Failure;
  }

  try {
    return callback_(context);
  } catch (const std::exception&) {
    return BTStatus::Failure;
  } catch (...) {
    return BTStatus::Failure;
  }
}

void ActionNode::Reset(BTContext&) { std::lock_guard<std::mutex> lock{mutex_}; }

void ActionNode::Shutdown(BTContext&) { std::lock_guard<std::mutex> lock{mutex_}; }

} // namespace humanoid::bt
