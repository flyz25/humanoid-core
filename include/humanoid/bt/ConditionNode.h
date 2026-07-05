#pragma once

/**
 * @file ConditionNode.h
 * @brief Defines a reusable behavior tree condition leaf node.
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
 * @brief Predicate used by condition-oriented leaf nodes.
 */
using ConditionPredicate = std::function<bool(const BTContext&)>;

/**
 * @brief Leaf node that evaluates runtime context state once per tick.
 *
 * Conditions return `Success` when the injected predicate is true and `Failure`
 * when it is false. The predicate receives only `BTContext`, keeping condition
 * evaluation vendor independent and SDK-free.
 */
class ConditionNode final : public BTNode {
public:
  /**
   * @brief Constructs a condition node.
   *
   * @param name Stable diagnostic node name.
   * @param predicate Predicate evaluated during `Tick()`.
   */
  explicit ConditionNode(std::string name = "Condition", ConditionPredicate predicate = {});

  /** @brief Destroys the node. */
  ~ConditionNode() noexcept override = default;

  ConditionNode(const ConditionNode&) = delete;
  ConditionNode& operator=(const ConditionNode&) = delete;
  ConditionNode(ConditionNode&&) = delete;
  ConditionNode& operator=(ConditionNode&&) = delete;

  /**
   * @brief Returns the stable condition node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Validates predicate availability.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Idle` when a predicate is available, otherwise `Failure`.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Evaluates the injected predicate.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Success` for true, `Failure` for false, or `Aborted` on
   * cancellation.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

  /**
   * @brief Resets transient condition state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Releases condition resources.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

private:
  mutable std::mutex mutex_;
  std::string name_;
  ConditionPredicate predicate_;
};

} // namespace humanoid::bt
