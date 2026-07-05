#pragma once

/**
 * @file ActionNode.h
 * @brief Defines a reusable behavior tree action leaf node.
 */

#include <functional>
#include <mutex>
#include <string>
#include <string_view>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>

namespace humanoid::bt {

class BTContext;

/**
 * @brief Callback used by `ActionNode` to execute one behavior tree tick.
 */
using ActionCallback = std::function<BTStatus(BTContext&)>;

/**
 * @brief Leaf node that executes an injected framework action callback.
 *
 * `ActionNode` owns no robot adapter, mission executor, command dispatcher, SDK
 * object, parser, planner, or vendor type. It is intended for small
 * dependency-injected actions that already respect framework boundaries.
 */
class ActionNode final : public BTNode {
public:
  /**
   * @brief Constructs an action node.
   *
   * @param name Stable diagnostic node name.
   * @param callback Action callback executed during `Tick()`.
   */
  explicit ActionNode(std::string name = "Action", ActionCallback callback = {});

  /** @brief Destroys the node. */
  ~ActionNode() noexcept override = default;

  ActionNode(const ActionNode&) = delete;
  ActionNode& operator=(const ActionNode&) = delete;
  ActionNode(ActionNode&&) = delete;
  ActionNode& operator=(ActionNode&&) = delete;

  /**
   * @brief Returns the stable action node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Validates callback availability.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Idle` when a callback is available, otherwise `Failure`.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Executes one action callback tick.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Callback status, `Aborted` on cancellation, or `Failure` on error.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

  /**
   * @brief Resets transient action state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Releases action resources.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

private:
  mutable std::mutex mutex_;
  std::string name_;
  ActionCallback callback_;
};

} // namespace humanoid::bt
