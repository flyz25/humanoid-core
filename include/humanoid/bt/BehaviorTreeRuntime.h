#pragma once

/**
 * @file BehaviorTreeRuntime.h
 * @brief Defines behavior tree integration with the shared execution runtime.
 */

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/runtime/ResourceManager.h>
#include <humanoid/runtime/RuntimeScheduler.h>

namespace humanoid::bt {

class BehaviorTreeFactory;

/**
 * @brief Runtime scheduling and resource policy for one behavior tree job.
 */
struct BehaviorTreeJobOptions final {
  /** @brief Nonzero identifier used by the shared runtime scheduler. */
  runtime::RuntimeJobId executionId{0U};

  /** @brief Runtime scheduling priority. */
  runtime::RuntimeJobPriority priority{runtime::RuntimeJobPriority::Normal};

  /** @brief Parallel or sequential scheduler execution mode. */
  runtime::RuntimeExecutionMode executionMode{runtime::RuntimeExecutionMode::Parallel};

  /** @brief Optional logical resource acquired for the tree lifetime. */
  std::string resourceId;

  /** @brief Ownership mode used when a resource identifier is present. */
  runtime::ResourceLockMode resourceMode{runtime::ResourceLockMode::Exclusive};

  /** @brief Maximum resource acquisition wait; zero performs a nonblocking attempt. */
  std::chrono::milliseconds resourceTimeout{0};

  /** @brief Positive delay between nonterminal tree ticks. */
  std::chrono::milliseconds tickInterval{10};

  /** @brief Non-operational metadata copied into the runtime job. */
  runtime::ExecutionMetadata metadata;
};

/**
 * @brief Executes behavior trees through injected runtime infrastructure.
 *
 * This integration layer creates or accepts a behavior tree, binds it to the
 * scheduler-owned `ExecutionContext` and shared `Blackboard`, acquires an
 * optional `ResourceManager` lease, and cooperates with scheduler pause and
 * cancellation. It owns no worker threads, queue, blackboard, or resource
 * registry of its own.
 */
class BehaviorTreeRuntime final {
public:
  /**
   * @brief Constructs a runtime integration from shared framework services.
   *
   * @param scheduler Shared execution scheduler.
   * @param blackboard Shared runtime blackboard.
   * @param resource_manager Shared runtime resource manager.
   * @param factory Shared behavior tree node factory.
   * @throws std::invalid_argument if any dependency is null.
   */
  BehaviorTreeRuntime(std::shared_ptr<runtime::RuntimeScheduler> scheduler,
                      std::shared_ptr<runtime::Blackboard> blackboard,
                      std::shared_ptr<runtime::ResourceManager> resource_manager,
                      std::shared_ptr<const BehaviorTreeFactory> factory);

  /** @brief Releases shared runtime service ownership. */
  ~BehaviorTreeRuntime() = default;

  BehaviorTreeRuntime(const BehaviorTreeRuntime&) = delete;
  BehaviorTreeRuntime& operator=(const BehaviorTreeRuntime&) = delete;
  BehaviorTreeRuntime(BehaviorTreeRuntime&&) = delete;
  BehaviorTreeRuntime& operator=(BehaviorTreeRuntime&&) = delete;

  /**
   * @brief Creates a tree root through the factory and submits it to runtime.
   *
   * @param root_node_type Registered root node type.
   * @param options Runtime job options.
   * @return Scheduler job handle.
   */
  [[nodiscard]] runtime::RuntimeJobHandle Submit(std::string_view root_node_type,
                                                 BehaviorTreeJobOptions options) const;

  /**
   * @brief Submits an already constructed behavior tree to runtime.
   *
   * This overload supports trees produced by `TreeLoader`. Ownership transfers
   * to the submitted runtime callback.
   *
   * @param tree Owned behavior tree.
   * @param options Runtime job options.
   * @return Scheduler job handle.
   */
  [[nodiscard]] runtime::RuntimeJobHandle Submit(std::unique_ptr<BehaviorTree> tree,
                                                 BehaviorTreeJobOptions options) const;

private:
  std::shared_ptr<runtime::RuntimeScheduler> scheduler_;
  std::shared_ptr<runtime::Blackboard> blackboard_;
  std::shared_ptr<runtime::ResourceManager> resource_manager_;
  std::shared_ptr<const BehaviorTreeFactory> factory_;
};

} // namespace humanoid::bt
