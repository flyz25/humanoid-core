#include <humanoid/bt/BehaviorTreeRuntime.h>

#include <condition_variable>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

#include <humanoid/bt/BehaviorTreeFactory.h>

namespace humanoid::bt {
namespace {

[[nodiscard]] runtime::RuntimeJobResult TerminalResult(BTStatus status) {
  switch (status) {
  case BTStatus::Success:
    return {runtime::ExecutionState::Completed, "Behavior tree completed"};
  case BTStatus::Failure:
    return {runtime::ExecutionState::Failed, "Behavior tree failed"};
  case BTStatus::Aborted:
    return {runtime::ExecutionState::Aborted, "Behavior tree aborted"};
  case BTStatus::Idle:
  case BTStatus::Running:
    return {runtime::ExecutionState::Failed, "Behavior tree returned a nonterminal status"};
  }
  return {runtime::ExecutionState::Failed, "Behavior tree returned an unknown status"};
}

[[nodiscard]] std::optional<runtime::ResourceLock>
AcquireResource(const BehaviorTreeJobOptions& options,
                const std::shared_ptr<runtime::ResourceManager>& resource_manager) {
  if (options.resourceId.empty()) {
    return runtime::ResourceLock{};
  }
  if (options.resourceTimeout == std::chrono::milliseconds::zero()) {
    return resource_manager->TryAcquire(options.resourceId, options.resourceMode);
  }
  return resource_manager->Acquire(options.resourceId, options.resourceMode,
                                   options.resourceTimeout);
}

[[nodiscard]] runtime::RuntimeJobResult
ExecuteTree(runtime::RuntimeJobContext& job_context, const std::shared_ptr<BehaviorTree>& tree,
            const BehaviorTreeJobOptions& options,
            const std::shared_ptr<runtime::Blackboard>& blackboard,
            const std::shared_ptr<runtime::ResourceManager>& resource_manager) {
  if (!tree) {
    return {runtime::ExecutionState::Failed, "Behavior tree is required"};
  }
  if (options.tickInterval <= std::chrono::milliseconds::zero() ||
      options.resourceTimeout < std::chrono::milliseconds::zero()) {
    return {runtime::ExecutionState::Failed,
            "Behavior tree tick interval must be positive and resource timeout nonnegative"};
  }

  std::optional<runtime::ResourceLock> resource_lock = AcquireResource(options, resource_manager);
  if (!resource_lock.has_value()) {
    return {runtime::ExecutionState::Failed, "Behavior tree resource is unavailable"};
  }

  if (!tree->Context().SetExecution(job_context.SharedExecution()) ||
      !tree->Context().SetBlackboard(blackboard)) {
    return {runtime::ExecutionState::Failed, "Behavior tree runtime binding failed"};
  }

  std::mutex wait_mutex;
  std::condition_variable wait_condition;
  const runtime::CancellationToken cancellation = job_context.Cancellation();
  runtime::CancellationRegistration registration =
      cancellation.Register([&wait_condition]() { wait_condition.notify_all(); });

  BTStatus status = tree->Initialize();
  while (!isTerminal(status)) {
    if (job_context.IsCancellationRequested()) {
      static_cast<void>(job_context.Execution().RequestCancellation());
      static_cast<void>(tree->Shutdown());
      return {runtime::ExecutionState::Cancelled, "Behavior tree cancelled"};
    }
    if (!job_context.WaitIfPaused()) {
      static_cast<void>(job_context.Execution().RequestCancellation());
      static_cast<void>(tree->Shutdown());
      return {runtime::ExecutionState::Cancelled, "Behavior tree cancelled while paused"};
    }

    status = tree->Tick();
    if (isTerminal(status)) {
      continue;
    }

    std::unique_lock<std::mutex> wait_lock{wait_mutex};
    wait_condition.wait_for(wait_lock, options.tickInterval,
                            [&job_context]() { return job_context.IsCancellationRequested(); });
  }
  return TerminalResult(status);
}

[[nodiscard]] runtime::RuntimeJob MakeRuntimeJob(BehaviorTreeJobOptions options,
                                                 runtime::RuntimeJobCallback callback) {
  runtime::RuntimeJob job;
  job.id = options.executionId;
  job.priority = options.priority;
  job.executionMode = options.executionMode;
  job.scope = runtime::ExecutionScope::BehaviorTree;
  job.metadata = std::move(options.metadata);
  job.callback = std::move(callback);
  return job;
}

} // namespace

BehaviorTreeRuntime::BehaviorTreeRuntime(std::shared_ptr<runtime::RuntimeScheduler> scheduler,
                                         std::shared_ptr<runtime::Blackboard> blackboard,
                                         std::shared_ptr<runtime::ResourceManager> resource_manager,
                                         std::shared_ptr<const BehaviorTreeFactory> factory)
    : scheduler_(std::move(scheduler)), blackboard_(std::move(blackboard)),
      resource_manager_(std::move(resource_manager)), factory_(std::move(factory)) {
  if (!scheduler_ || !blackboard_ || !resource_manager_ || !factory_) {
    throw std::invalid_argument{"BehaviorTreeRuntime dependencies must not be null"};
  }
}

runtime::RuntimeJobHandle BehaviorTreeRuntime::Submit(std::string_view root_node_type,
                                                      BehaviorTreeJobOptions options) const {
  const std::string root_type{root_node_type};
  const auto factory = factory_;
  const auto blackboard = blackboard_;
  const auto resource_manager = resource_manager_;
  const BehaviorTreeJobOptions execution_options = options;

  runtime::RuntimeJobCallback callback =
      [factory, blackboard, resource_manager, root_type,
       execution_options](runtime::RuntimeJobContext& context) mutable {
        std::shared_ptr<BehaviorTree> tree = factory->CreateTree(root_type);
        if (!tree) {
          return runtime::RuntimeJobResult{runtime::ExecutionState::Failed,
                                           "Behavior tree root type is not registered"};
        }
        return ExecuteTree(context, tree, execution_options, blackboard, resource_manager);
      };
  return scheduler_->Submit(MakeRuntimeJob(std::move(options), std::move(callback)));
}

runtime::RuntimeJobHandle BehaviorTreeRuntime::Submit(std::unique_ptr<BehaviorTree> tree,
                                                      BehaviorTreeJobOptions options) const {
  const auto shared_tree = std::shared_ptr<BehaviorTree>{std::move(tree)};
  const auto blackboard = blackboard_;
  const auto resource_manager = resource_manager_;
  const BehaviorTreeJobOptions execution_options = options;

  runtime::RuntimeJobCallback callback =
      [shared_tree, blackboard, resource_manager,
       execution_options](runtime::RuntimeJobContext& context) mutable {
        if (!shared_tree) {
          return runtime::RuntimeJobResult{runtime::ExecutionState::Failed,
                                           "Behavior tree is required"};
        }
        return ExecuteTree(context, shared_tree, execution_options, blackboard, resource_manager);
      };
  return scheduler_->Submit(MakeRuntimeJob(std::move(options), std::move(callback)));
}

} // namespace humanoid::bt
