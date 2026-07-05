#include <humanoid/bt/RetryNode.h>

#include <algorithm>
#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

RetryNode::RetryNode(std::string name, std::size_t max_attempts, std::unique_ptr<BTNode> child)
    : DecoratorNode(std::move(name), std::move(child)),
      max_attempts_(std::max<std::size_t>(1U, max_attempts)) {}

BTStatus RetryNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    failed_attempts_ = 0U;
    return BTStatus::Aborted;
  }
  if (!child_) {
    return BTStatus::Failure;
  }

  const BTStatus status = child_->Tick(context);
  switch (status) {
  case BTStatus::Success:
    failed_attempts_ = 0U;
    return BTStatus::Success;
  case BTStatus::Failure:
    ++failed_attempts_;
    if (failed_attempts_ >= max_attempts_) {
      failed_attempts_ = 0U;
      return BTStatus::Failure;
    }
    child_->Reset(context);
    return BTStatus::Running;
  case BTStatus::Aborted:
    failed_attempts_ = 0U;
    return BTStatus::Aborted;
  case BTStatus::Idle:
  case BTStatus::Running:
    return BTStatus::Running;
  }

  return BTStatus::Failure;
}

void RetryNode::OnInitializeLocked() { failed_attempts_ = 0U; }

void RetryNode::OnResetLocked() { failed_attempts_ = 0U; }

} // namespace humanoid::bt
