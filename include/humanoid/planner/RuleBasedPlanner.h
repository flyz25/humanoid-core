#pragma once

/**
 * @file RuleBasedPlanner.h
 * @brief Defines a deterministic static rule-based planner.
 */

#include <atomic>
#include <cstddef>
#include <shared_mutex>
#include <string>
#include <vector>

#include <humanoid/core/CommandType.h>
#include <humanoid/planner/IPlanner.h>

namespace humanoid::planner {

/**
 * @brief One deterministic rule used by `RuleBasedPlanner`.
 *
 * Rules match by goal type and minimum goal priority. Matching rules produce a
 * fixed command sequence that is converted into both a `Mission` and a
 * behavior tree representation. A fallback rule is used only when no normal
 * rule matches.
 */
struct RuleBasedPlannerRule final {
  /**
   * @brief Stable rule identifier used in diagnostics and generated metadata.
   */
  std::string ruleId;

  /**
   * @brief Goal type accepted by this rule.
   */
  GoalType goalType{GoalType::Custom};

  /**
   * @brief Minimum goal priority required for this rule.
   */
  GoalPriority minimumPriority{GoalPriority::Low};

  /**
   * @brief Deterministic command sequence generated when the rule matches.
   */
  std::vector<humanoid::core::CommandType> commandSequence;

  /**
   * @brief Human-readable mission name prefix for generated missions.
   */
  std::string missionName{"Rule-Based Plan"};

  /**
   * @brief True when this rule is used only as a fallback.
   */
  bool fallback{false};

  /**
   * @brief Reports whether the rule contains enough data to generate a plan.
   *
   * @return True when rule identifier and command sequence are assigned.
   */
  [[nodiscard]] bool isValid() const noexcept {
    return !ruleId.empty() && !commandSequence.empty();
  }
};

/**
 * @brief Deterministic planner that maps goals to static framework plans.
 *
 * `RuleBasedPlanner` contains no AI, LLM integration, SDK calls, adapter calls,
 * mission execution, or behavior tree runtime execution. It converts a
 * `PlanningRequest` into framework-owned `Mission` and `BehaviorTree` values
 * using an ordered static rule set.
 */
class RuleBasedPlanner final : public IPlanner {
public:
  /**
   * @brief Ordered rule set used for deterministic planning.
   */
  using RuleSet = std::vector<RuleBasedPlannerRule>;

  /**
   * @brief Constructs a planner with the default static rules.
   */
  RuleBasedPlanner();

  /**
   * @brief Constructs a planner with an injected rule set.
   *
   * Invalid rules are ignored. If no valid fallback rule is supplied, a default
   * fallback rule is appended.
   *
   * @param rules Ordered deterministic rules.
   */
  explicit RuleBasedPlanner(RuleSet rules);

  /** @brief Destroys the rule-based planner. */
  ~RuleBasedPlanner() override = default;

  RuleBasedPlanner(const RuleBasedPlanner&) = delete;
  RuleBasedPlanner& operator=(const RuleBasedPlanner&) = delete;
  RuleBasedPlanner(RuleBasedPlanner&&) = delete;
  RuleBasedPlanner& operator=(RuleBasedPlanner&&) = delete;

  /**
   * @brief Generates a deterministic mission and behavior tree for a goal.
   *
   * @param request Vendor-independent planning input.
   * @return Planning result with generated artifacts and diagnostics.
   */
  [[nodiscard]] PlanningResult Plan(const PlanningRequest& request) override;

  /**
   * @brief Validates a generated plan for minimum executable artifacts.
   *
   * @param result Planning result to validate.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status ValidatePlan(const PlanningResult& result) const override;

  /**
   * @brief Requests cooperative cancellation for current or next planning call.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status CancelPlan() override;

  /**
   * @brief Returns this planner's static capability declaration.
   *
   * @return Planner capabilities.
   */
  [[nodiscard]] PlannerCapabilities GetCapabilities() const override;

  /**
   * @brief Returns the number of valid rules owned by this planner.
   *
   * @return Rule count.
   */
  [[nodiscard]] std::size_t RuleCount() const;

  /**
   * @brief Returns the default deterministic rule set.
   *
   * @return Ordered default rules.
   */
  [[nodiscard]] static RuleSet DefaultRules();

private:
  [[nodiscard]] static RuleBasedPlannerRule DefaultFallbackRule();
  [[nodiscard]] static PlannerCapabilities DefaultCapabilities();

  RuleSet rules_;
  PlannerCapabilities capabilities_;
  mutable std::shared_mutex mutex_;
  std::atomic<bool> cancellationRequested_{false};
};

} // namespace humanoid::planner
