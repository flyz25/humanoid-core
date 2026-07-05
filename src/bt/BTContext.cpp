#include <humanoid/bt/BTContext.h>

#include <mutex>
#include <utility>

namespace humanoid::bt {

namespace {

std::shared_ptr<runtime::ExecutionContext> MakeExecutionContext() {
  return std::make_shared<runtime::ExecutionContext>(runtime::kInvalidExecutionContextId,
                                                     runtime::ExecutionScope::BehaviorTree);
}

std::shared_ptr<runtime::Blackboard> MakeBlackboard() {
  return std::make_shared<runtime::Blackboard>();
}

} // namespace

BTContext::BTContext() : BTContext(MakeExecutionContext(), MakeBlackboard()) {}

BTContext::BTContext(std::shared_ptr<runtime::ExecutionContext> execution_context,
                     std::shared_ptr<runtime::Blackboard> blackboard)
    : execution_context_(execution_context ? std::move(execution_context) : MakeExecutionContext()),
      blackboard_(blackboard ? std::move(blackboard) : MakeBlackboard()) {}

BTContext::BTContext(const BTContext& other) {
  std::shared_lock<std::shared_mutex> lock{other.mutex_};
  execution_context_ = other.execution_context_;
  blackboard_ = other.blackboard_;
  if (!execution_context_) {
    execution_context_ = MakeExecutionContext();
  }
  if (!blackboard_) {
    blackboard_ = MakeBlackboard();
  }
}

BTContext& BTContext::operator=(const BTContext& other) {
  if (this == &other) {
    return *this;
  }

  std::unique_lock<std::shared_mutex> this_lock{mutex_, std::defer_lock};
  std::shared_lock<std::shared_mutex> other_lock{other.mutex_, std::defer_lock};
  std::lock(this_lock, other_lock);
  execution_context_ = other.execution_context_;
  blackboard_ = other.blackboard_;
  if (!execution_context_) {
    execution_context_ = MakeExecutionContext();
  }
  if (!blackboard_) {
    blackboard_ = MakeBlackboard();
  }
  return *this;
}

BTContext::BTContext(BTContext&& other) {
  std::unique_lock<std::shared_mutex> lock{other.mutex_};
  execution_context_ = std::move(other.execution_context_);
  blackboard_ = std::move(other.blackboard_);
  if (!execution_context_) {
    execution_context_ = MakeExecutionContext();
  }
  if (!blackboard_) {
    blackboard_ = MakeBlackboard();
  }
}

BTContext& BTContext::operator=(BTContext&& other) {
  if (this == &other) {
    return *this;
  }

  std::scoped_lock lock{mutex_, other.mutex_};
  execution_context_ = std::move(other.execution_context_);
  blackboard_ = std::move(other.blackboard_);
  if (!execution_context_) {
    execution_context_ = MakeExecutionContext();
  }
  if (!blackboard_) {
    blackboard_ = MakeBlackboard();
  }
  return *this;
}

std::shared_ptr<runtime::ExecutionContext> BTContext::Execution() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return execution_context_;
}

bool BTContext::SetExecution(std::shared_ptr<runtime::ExecutionContext> execution_context) {
  if (!execution_context) {
    return false;
  }
  std::unique_lock<std::shared_mutex> lock{mutex_};
  execution_context_ = std::move(execution_context);
  return true;
}

std::shared_ptr<runtime::Blackboard> BTContext::Blackboard() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return blackboard_;
}

bool BTContext::SetBlackboard(std::shared_ptr<runtime::Blackboard> blackboard) {
  if (!blackboard) {
    return false;
  }
  std::unique_lock<std::shared_mutex> lock{mutex_};
  blackboard_ = std::move(blackboard);
  return true;
}

bool BTContext::CancellationRequested() const {
  const std::shared_ptr<runtime::ExecutionContext> execution = Execution();
  return execution && execution->CancellationRequested();
}

} // namespace humanoid::bt
