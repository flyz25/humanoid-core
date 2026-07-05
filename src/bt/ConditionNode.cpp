#include <humanoid/bt/ConditionNode.h>

#include <exception>
#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

ConditionNode::ConditionNode(std::string name, ConditionPredicate predicate)
    : name_(std::move(name)), predicate_(std::move(predicate)) {
  if (name_.empty()) {
    name_ = "Condition";
  }
}

std::string_view ConditionNode::Name() const noexcept { return name_; }

BTStatus ConditionNode::Initialize(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  return predicate_ ? BTStatus::Idle : BTStatus::Failure;
}

BTStatus ConditionNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    return BTStatus::Aborted;
  }
  if (!predicate_) {
    return BTStatus::Failure;
  }

  try {
    return predicate_(context) ? BTStatus::Success : BTStatus::Failure;
  } catch (const std::exception&) {
    return BTStatus::Failure;
  } catch (...) {
    return BTStatus::Failure;
  }
}

void ConditionNode::Reset(BTContext&) { std::lock_guard<std::mutex> lock{mutex_}; }

void ConditionNode::Shutdown(BTContext&) { std::lock_guard<std::mutex> lock{mutex_}; }

} // namespace humanoid::bt
