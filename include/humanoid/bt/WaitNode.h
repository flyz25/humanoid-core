#pragma once

/**
 * @file WaitNode.h
 * @brief Defines a behavior tree wait-until-condition leaf node.
 */

#include <mutex>
#include <string>
#include <string_view>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/ConditionNode.h>

namespace humanoid::bt {

/**
 * @brief Leaf node that remains running until a runtime predicate is true.
 */
class WaitNode final : public BTNode {
public:
  /**
   * @brief Constructs a wait node.
   *
   * @param name Stable diagnostic node name.
   * @param predicate Predicate evaluated during `Tick()`.
   */
  explicit WaitNode(std::string name = "Wait", ConditionPredicate predicate = {});

  /** @brief Destroys the node. */
  ~WaitNode() noexcept override = default;

  WaitNode(const WaitNode&) = delete;
  WaitNode& operator=(const WaitNode&) = delete;
  WaitNode(WaitNode&&) = delete;
  WaitNode& operator=(WaitNode&&) = delete;

  /**
   * @brief Returns the stable wait node name.
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
   * @brief Evaluates the predicate and waits cooperatively while false.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Success` once true, `Running` while false, or `Aborted` on
   * cancellation.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

  /**
   * @brief Clears satisfied state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Releases wait resources.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

private:
  mutable std::mutex mutex_;
  std::string name_;
  ConditionPredicate predicate_;
  bool satisfied_{false};
};

} // namespace humanoid::bt
