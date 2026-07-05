#include <humanoid/bt/WaitNode.h>

#include <exception>
#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

WaitNode::WaitNode(std::string name, ConditionPredicate predicate)
    : name_(std::move(name)), predicate_(std::move(predicate)) {
  if (name_.empty()) {
    name_ = "Wait";
  }
}

std::string_view WaitNode::Name() const noexcept { return name_; }

BTStatus WaitNode::Initialize(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  satisfied_ = false;
  return predicate_ ? BTStatus::Idle : BTStatus::Failure;
}

BTStatus WaitNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    satisfied_ = false;
    return BTStatus::Aborted;
  }
  if (!predicate_) {
    return BTStatus::Failure;
  }
  if (satisfied_) {
    return BTStatus::Success;
  }

  try {
    satisfied_ = predicate_(context);
    return satisfied_ ? BTStatus::Success : BTStatus::Running;
  } catch (const std::exception&) {
    satisfied_ = false;
    return BTStatus::Failure;
  } catch (...) {
    satisfied_ = false;
    return BTStatus::Failure;
  }
}

void WaitNode::Reset(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  satisfied_ = false;
}

void WaitNode::Shutdown(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  satisfied_ = false;
}

} // namespace humanoid::bt
