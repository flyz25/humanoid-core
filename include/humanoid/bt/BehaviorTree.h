#pragma once

/**
 * @file BehaviorTree.h
 * @brief Defines the owning behavior tree runtime facade.
 */

#include <memory>
#include <mutex>

#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>

namespace humanoid::bt {

/**
 * @brief Owns and ticks one vendor-independent behavior tree.
 *
 * `BehaviorTree` serializes lifecycle operations, owns the root node, and
 * maps tree execution state onto the injected runtime execution context. It is
 * not a mission engine, command dispatcher, XML parser, robot adapter, or SDK
 * wrapper.
 */
class BehaviorTree final {
public:
  /**
   * @brief Constructs a behavior tree with an owned root node.
   *
   * @param root Root behavior tree node.
   * @param context Runtime-backed node context.
   */
  explicit BehaviorTree(std::unique_ptr<BTNode> root, BTContext context = {});

  /** @brief Shuts down the tree if needed and destroys owned resources. */
  ~BehaviorTree() noexcept;

  BehaviorTree(const BehaviorTree&) = delete;
  BehaviorTree& operator=(const BehaviorTree&) = delete;
  BehaviorTree(BehaviorTree&&) = delete;
  BehaviorTree& operator=(BehaviorTree&&) = delete;

  /**
   * @brief Initializes the tree and its root node.
   *
   * @return Current tree status after initialization.
   */
  [[nodiscard]] BTStatus Initialize();

  /**
   * @brief Executes one tick of the root node.
   *
   * A tree that has not been initialized is initialized automatically before
   * the first tick. Cancellation from the runtime context aborts the tick.
   *
   * @return Current tree status after the tick.
   */
  [[nodiscard]] BTStatus Tick();

  /**
   * @brief Resets root node state and returns the tree to idle.
   *
   * @return True when reset was applied.
   */
  bool Reset();

  /**
   * @brief Shuts down the tree and root node.
   *
   * @return True on the first shutdown call.
   */
  bool Shutdown() noexcept;

  /**
   * @brief Returns the latest tree status.
   *
   * @return Tree status snapshot.
   */
  [[nodiscard]] BTStatus Status() const;

  /**
   * @brief Reports whether the tree has a root node.
   *
   * @return True when a root node is available.
   */
  [[nodiscard]] bool HasRoot() const;

  /**
   * @brief Returns the runtime-backed behavior tree context.
   *
   * @return Context reference owned by the tree.
   */
  [[nodiscard]] BTContext& Context() noexcept;

  /**
   * @brief Returns the runtime-backed behavior tree context.
   *
   * @return Const context reference owned by the tree.
   */
  [[nodiscard]] const BTContext& Context() const noexcept;

private:
  [[nodiscard]] BTStatus InitializeLocked();
  [[nodiscard]] BTStatus AbortLocked() noexcept;
  void ApplyRuntimeStateLocked(BTStatus status) noexcept;

  mutable std::mutex mutex_;
  std::unique_ptr<BTNode> root_;
  BTContext context_;
  BTStatus status_{BTStatus::Idle};
  bool initialized_{false};
  bool shutdown_{false};
};

} // namespace humanoid::bt
