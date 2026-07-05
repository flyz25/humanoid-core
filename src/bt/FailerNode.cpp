#include <humanoid/bt/FailerNode.h>

#include <utility>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

FailerNode::FailerNode(std::string name, std::unique_ptr<BTNode> child)
    : DecoratorNode(std::move(name), std::move(child)) {}

BTStatus FailerNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    return BTStatus::Aborted;
  }
  if (!child_) {
    return BTStatus::Failure;
  }

  switch (child_->Tick(context)) {
  case BTStatus::Success:
  case BTStatus::Failure:
  case BTStatus::Idle:
    return BTStatus::Failure;
  case BTStatus::Running:
    return BTStatus::Running;
  case BTStatus::Aborted:
    return BTStatus::Aborted;
  }

  return BTStatus::Failure;
}

} // namespace humanoid::bt
