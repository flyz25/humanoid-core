#pragma once

/**
 * @file BTContext.h
 * @brief Defines the runtime-backed behavior tree execution context.
 */

#include <memory>
#include <shared_mutex>

#include <humanoid/runtime/Blackboard.h>
#include <humanoid/runtime/ExecutionContext.h>

namespace humanoid::bt {

/**
 * @brief Thread-safe dependency container passed to behavior tree nodes.
 *
 * `BTContext` exposes shared runtime primitives to behavior tree nodes without
 * coupling the behavior tree layer to mission execution, robot adapters,
 * plugins, XML parsers, ROS2, or vendor SDKs.
 */
class BTContext final {
public:
  /**
   * @brief Constructs a context with default runtime-owned primitives.
   */
  BTContext();

  /**
   * @brief Constructs a context from injected runtime primitives.
   *
   * Null dependencies are replaced with default runtime-owned instances.
   *
   * @param execution_context Shared execution context.
   * @param blackboard Shared runtime blackboard.
   */
  BTContext(std::shared_ptr<runtime::ExecutionContext> execution_context,
            std::shared_ptr<runtime::Blackboard> blackboard);

  /** @brief Destroys the context. */
  ~BTContext() = default;

  /**
   * @brief Copies shared runtime dependencies into a new context lock.
   *
   * @param other Source context.
   */
  BTContext(const BTContext& other);

  /**
   * @brief Replaces this context with another context's shared dependencies.
   *
   * @param other Source context.
   * @return This context.
   */
  BTContext& operator=(const BTContext& other);

  /**
   * @brief Moves a context by copying shared runtime dependencies.
   *
   * @param other Source context.
   */
  BTContext(BTContext&& other);

  /**
   * @brief Replaces this context with another context's shared dependencies.
   *
   * @param other Source context.
   * @return This context.
   */
  BTContext& operator=(BTContext&& other);

  /**
   * @brief Returns the shared execution context.
   *
   * @return Non-null execution context.
   */
  [[nodiscard]] std::shared_ptr<runtime::ExecutionContext> Execution() const;

  /**
   * @brief Replaces the shared execution context.
   *
   * Null input is ignored to preserve a valid context.
   *
   * @param execution_context New execution context.
   * @return True when the dependency was replaced.
   */
  bool SetExecution(std::shared_ptr<runtime::ExecutionContext> execution_context);

  /**
   * @brief Returns the shared runtime blackboard.
   *
   * @return Non-null blackboard.
   */
  [[nodiscard]] std::shared_ptr<runtime::Blackboard> Blackboard() const;

  /**
   * @brief Replaces the shared runtime blackboard.
   *
   * Null input is ignored to preserve a valid context.
   *
   * @param blackboard New runtime blackboard.
   * @return True when the dependency was replaced.
   */
  bool SetBlackboard(std::shared_ptr<runtime::Blackboard> blackboard);

  /**
   * @brief Reports whether cooperative cancellation has been requested.
   *
   * @return True when the runtime execution context is cancelled.
   */
  [[nodiscard]] bool CancellationRequested() const;

private:
  mutable std::shared_mutex mutex_;
  std::shared_ptr<runtime::ExecutionContext> execution_context_;
  std::shared_ptr<runtime::Blackboard> blackboard_;
};

} // namespace humanoid::bt
