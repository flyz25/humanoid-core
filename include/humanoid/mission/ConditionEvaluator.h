#pragma once

/**
 * @file ConditionEvaluator.h
 * @brief Defines mission condition evaluation against generic robot state.
 */

#include <memory>
#include <mutex>
#include <vector>

#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/core/SafetyValidator.h>
#include <humanoid/mission/MissionCondition.h>
#include <humanoid/mission/MissionEvent.h>

namespace humanoid::mission {

/**
 * @brief Result produced by evaluating one mission condition.
 */
struct ConditionEvaluationResult final {
  /**
   * @brief True when the condition source was available and evaluation ran.
   */
  bool evaluated{false};

  /**
   * @brief True when the condition was satisfied.
   */
  bool satisfied{false};

  /**
   * @brief Event describing the evaluation outcome.
   */
  MissionEvent event;

  /**
   * @brief Reports whether the condition was evaluated and satisfied.
   *
   * @return True when evaluation succeeded and the condition passed.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return evaluated && satisfied; }
};

/**
 * @brief Static context used for non-state condition sources.
 */
struct ConditionEvaluatorContext final {
  /**
   * @brief Generic command capabilities for capability conditions.
   */
  humanoid::core::CommandCapabilitySet capabilities{};

  /**
   * @brief True when `capabilities` came from a trusted framework source.
   */
  bool capabilitiesAvailable{false};
};

/**
 * @brief Evaluates mission conditions using RobotStateManager snapshots.
 *
 * The evaluator is vendor independent. Runtime robot state is read only through
 * the injected `humanoid::core::RobotStateManager`. Capability checks use an
 * injected generic capability snapshot and never call robot adapters or SDKs.
 */
class ConditionEvaluator final {
public:
  /**
   * @brief Constructs an evaluator with an optional capability context.
   *
   * @param state_manager Shared robot state manager used for condition reads.
   * @param context Static evaluator context.
   */
  explicit ConditionEvaluator(
      std::shared_ptr<const humanoid::core::RobotStateManager> state_manager,
      ConditionEvaluatorContext context = {});

  /**
   * @brief Replaces the generic capability context.
   *
   * @param capabilities New generic command capability set.
   */
  void UpdateCapabilities(humanoid::core::CommandCapabilitySet capabilities);

  /**
   * @brief Marks generic capabilities as unavailable.
   */
  void ClearCapabilities();

  /**
   * @brief Evaluates one condition.
   *
   * @param condition Mission condition to evaluate.
   * @return Evaluation result and event.
   */
  [[nodiscard]] ConditionEvaluationResult Evaluate(const MissionCondition& condition) const;

  /**
   * @brief Evaluates all supplied conditions in order.
   *
   * @param conditions Conditions to evaluate.
   * @return Ordered evaluation results.
   */
  [[nodiscard]] std::vector<ConditionEvaluationResult>
  EvaluateAll(const std::vector<MissionCondition>& conditions) const;

private:
  std::shared_ptr<const humanoid::core::RobotStateManager> state_manager_;
  mutable std::mutex context_mutex_;
  ConditionEvaluatorContext context_;
};

} // namespace humanoid::mission
