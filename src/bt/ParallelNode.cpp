#include <humanoid/bt/ParallelNode.h>

#include <algorithm>
#include <functional>
#include <future>
#include <utility>
#include <vector>

#include <humanoid/bt/BTContext.h>

namespace humanoid::bt {

ParallelNode::ParallelNode(std::string name, std::size_t success_threshold,
                           std::size_t failure_threshold, BTChildren children)
    : CompositeNode(std::move(name), std::move(children)), success_threshold_(success_threshold),
      failure_threshold_(std::max<std::size_t>(1U, failure_threshold)) {}

BTStatus ParallelNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (context.CancellationRequested()) {
    return BTStatus::Aborted;
  }

  if (children_.empty()) {
    return BTStatus::Success;
  }

  std::vector<std::future<BTStatus>> futures;
  futures.reserve(children_.size());
  for (const std::unique_ptr<BTNode>& child : children_) {
    std::reference_wrapper<BTNode> child_ref{*child};
    futures.push_back(std::async(std::launch::async, [&context, child_ref]() mutable {
      try {
        return child_ref.get().Tick(context);
      } catch (...) {
        return BTStatus::Failure;
      }
    }));
  }

  std::size_t success_count{0U};
  std::size_t failure_count{0U};
  std::size_t running_count{0U};
  std::size_t aborted_count{0U};

  for (std::future<BTStatus>& future : futures) {
    switch (future.get()) {
    case BTStatus::Success:
      ++success_count;
      break;
    case BTStatus::Failure:
      ++failure_count;
      break;
    case BTStatus::Running:
    case BTStatus::Idle:
      ++running_count;
      break;
    case BTStatus::Aborted:
      ++aborted_count;
      break;
    }
  }

  if (aborted_count > 0U || context.CancellationRequested()) {
    return BTStatus::Aborted;
  }
  if (failure_count >= failure_threshold_) {
    return BTStatus::Failure;
  }

  const std::size_t required_successes =
      success_threshold_ == 0U ? children_.size() : std::min(success_threshold_, children_.size());
  if (success_count >= required_successes) {
    return BTStatus::Success;
  }
  if (running_count > 0U) {
    return BTStatus::Running;
  }
  return BTStatus::Failure;
}

} // namespace humanoid::bt
