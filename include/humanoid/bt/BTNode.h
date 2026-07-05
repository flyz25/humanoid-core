#pragma once

/**
 * @file BTNode.h
 * @brief Defines the abstract behavior tree node contract.
 */

#include <string_view>

#include <humanoid/bt/BTStatus.h>

namespace humanoid::bt {

class BTContext;

/**
 * @brief Pure abstract interface for vendor-independent behavior tree nodes.
 *
 * Nodes are execution policy components owned by a `BehaviorTree`. They receive
 * all shared runtime state through `BTContext` and must not depend on robot
 * adapters, mission execution, XML parsers, ROS2, or vendor SDKs.
 */
class BTNode {
public:
  /** @brief Destroys a behavior tree node. */
  virtual ~BTNode() noexcept = default;

  BTNode(const BTNode&) = delete;
  BTNode& operator=(const BTNode&) = delete;
  BTNode(BTNode&&) = delete;
  BTNode& operator=(BTNode&&) = delete;

  /**
   * @brief Returns a stable node name for diagnostics.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] virtual std::string_view Name() const noexcept = 0;

  /**
   * @brief Initializes node-owned runtime resources.
   *
   * This method is called before the first tick after tree initialization.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Initial node status.
   */
  [[nodiscard]] virtual BTStatus Initialize(BTContext& context) = 0;

  /**
   * @brief Executes one behavior tree tick.
   *
   * Implementations should return quickly and report `BTStatus::Running` when
   * more work is required. Long-running work should observe cancellation through
   * the supplied context.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Node status after the tick.
   */
  [[nodiscard]] virtual BTStatus Tick(BTContext& context) = 0;

  /**
   * @brief Resets transient node state to `BTStatus::Idle`.
   *
   * @param context Runtime-backed behavior tree context.
   */
  virtual void Reset(BTContext& context) = 0;

  /**
   * @brief Releases node-owned runtime resources.
   *
   * @param context Runtime-backed behavior tree context.
   */
  virtual void Shutdown(BTContext& context) = 0;

protected:
  /** @brief Constructs a behavior tree node base. */
  BTNode() = default;
};

} // namespace humanoid::bt
