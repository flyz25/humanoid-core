#pragma once

/**
 * @file IPlanner.h
 * @brief Defines the vendor-independent planner interface.
 */

#include <string>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/planner/GoalType.h>
#include <humanoid/planner/PlanningRequest.h>
#include <humanoid/planner/PlanningResult.h>

namespace humanoid::planner {

/**
 * @brief Declares the implementation family of a planner.
 *
 * The type is descriptive metadata for discovery and selection. It does not
 * expose implementation classes, SDK objects, LLM clients, or planner-specific
 * handles.
 */
enum class PlannerKind {
  Rule,     ///< Deterministic rule-based planner.
  Llm,      ///< Future large-language-model-backed planner.
  Symbolic, ///< Future symbolic planner.
  Custom    ///< Application-defined planner family.
};

/**
 * @brief Stable planner capability declaration.
 *
 * Capabilities are used by applications and future planner hosts to discover
 * compatible planners without depending on concrete implementations.
 */
struct PlannerCapabilities final {
  /**
   * @brief Stable planner identifier within a process or plugin host.
   */
  std::string plannerId;

  /**
   * @brief Human-readable planner name.
   */
  std::string name;

  /**
   * @brief Human-readable planner description.
   */
  std::string description;

  /**
   * @brief Planner implementation family.
   */
  PlannerKind kind{PlannerKind::Custom};

  /**
   * @brief Goal categories accepted by this planner.
   */
  std::vector<GoalType> supportedGoalTypes;

  /**
   * @brief True when the planner may produce `Mission` output.
   */
  bool supportsMissionOutput{false};

  /**
   * @brief True when the planner may produce `BehaviorTree` output.
   */
  bool supportsBehaviorTreeOutput{false};

  /**
   * @brief True when `ValidatePlan()` is implemented by the planner.
   */
  bool supportsPlanValidation{false};

  /**
   * @brief True when `CancelPlan()` can request cooperative planning cancellation.
   */
  bool supportsCancellation{false};

  /**
   * @brief Reports whether the capability declaration is usable.
   *
   * @return True when identity fields are assigned and at least one output type is supported.
   */
  [[nodiscard]] bool isValid() const noexcept {
    return !plannerId.empty() && !name.empty() &&
           (supportsMissionOutput || supportsBehaviorTreeOutput);
  }
};

/**
 * @brief Pure abstract vendor-independent planner interface.
 *
 * `IPlanner` converts a generic `PlanningRequest` into a `PlanningResult`
 * without exposing LLM SDKs, robot SDKs, parser internals, robot adapters, or
 * concrete planning implementations. Applications should depend on this
 * interface, a `PlannerFactory`, or a `PlannerRegistry`, not concrete planners.
 */
class IPlanner {
public:
  /**
   * @brief Destroys the planner interface.
   */
  virtual ~IPlanner() = default;

  /**
   * @brief Produces an executable planning artifact for the supplied request.
   *
   * Implementations must contain their own exceptions and report planning
   * errors through `PlanningResult::status` and diagnostics.
   *
   * @param request Vendor-independent planning input.
   * @return Planning result containing mission, behavior tree, and diagnostics.
   */
  [[nodiscard]] virtual PlanningResult Plan(const PlanningRequest& request) = 0;

  /**
   * @brief Validates a previously produced planning result.
   *
   * @param result Planning result to validate.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  ValidatePlan(const PlanningResult& result) const = 0;

  /**
   * @brief Requests cooperative cancellation of in-progress planning work.
   *
   * The interface does not define preemptive interruption. Implementations that
   * support cancellation must observe this request at safe internal boundaries.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status CancelPlan() = 0;

  /**
   * @brief Returns the planner capability declaration.
   *
   * @return Vendor-independent planner capabilities.
   */
  [[nodiscard]] virtual PlannerCapabilities GetCapabilities() const = 0;
};

} // namespace humanoid::planner
