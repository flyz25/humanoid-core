#pragma once

/**
 * @file PlanningResult.h
 * @brief Defines planner output using existing execution framework models.
 */

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/mission/Mission.h>
#include <humanoid/planner/GoalStatus.h>

namespace humanoid::planner {

/**
 * @brief Severity for planner diagnostics.
 */
enum class PlanningDiagnosticSeverity : std::uint8_t {
  Info,    ///< Informational diagnostic that does not affect success.
  Warning, ///< Non-fatal diagnostic that may affect plan quality.
  Error    ///< Fatal diagnostic explaining planning failure.
};

/**
 * @brief One vendor-independent diagnostic produced by a future planner.
 */
struct PlanningDiagnostic final {
  /**
   * @brief Diagnostic severity.
   */
  PlanningDiagnosticSeverity severity{PlanningDiagnosticSeverity::Info};

  /**
   * @brief Stable diagnostic code owned by the planner boundary.
   */
  std::string code;

  /**
   * @brief Human-readable diagnostic message.
   */
  std::string message;
};

/**
 * @brief Ordered collection of planner diagnostics.
 */
using PlanningDiagnostics = std::vector<PlanningDiagnostic>;

/**
 * @brief Vendor-independent output from a future planning boundary.
 *
 * A planner may produce a mission, a behavior tree, diagnostics, or any
 * combination of those outputs. `BehaviorTree` is owned through `std::unique_ptr`
 * because trees own node graphs and are intentionally non-copyable.
 */
struct PlanningResult final {
  /**
   * @brief Goal lifecycle state after planning.
   */
  GoalStatus status{GoalStatus::Pending};

  /**
   * @brief Optional mission produced by planning.
   */
  std::optional<humanoid::mission::Mission> mission;

  /**
   * @brief Optional behavior tree produced by planning.
   */
  std::unique_ptr<humanoid::bt::BehaviorTree> behaviorTree;

  /**
   * @brief Planner diagnostics, warnings, and failure explanations.
   */
  PlanningDiagnostics diagnostics;

  /**
   * @brief Constructs an empty planning result.
   */
  PlanningResult() = default;

  /** @brief Destroys owned planning output. */
  ~PlanningResult() = default;

  PlanningResult(const PlanningResult&) = delete;
  PlanningResult& operator=(const PlanningResult&) = delete;
  PlanningResult(PlanningResult&&) = default;
  PlanningResult& operator=(PlanningResult&&) = default;

  /**
   * @brief Reports whether a mission is present.
   *
   * @return True when planning produced a mission.
   */
  [[nodiscard]] bool hasMission() const noexcept { return mission.has_value(); }

  /**
   * @brief Reports whether a behavior tree is present.
   *
   * @return True when planning produced a behavior tree.
   */
  [[nodiscard]] bool hasBehaviorTree() const noexcept { return static_cast<bool>(behaviorTree); }

  /**
   * @brief Reports whether the planning result is successful.
   *
   * @return True when status is planned and at least one executable artifact is present.
   */
  [[nodiscard]] bool isSuccess() const noexcept {
    return status == GoalStatus::Planned && (hasMission() || hasBehaviorTree());
  }
};

} // namespace humanoid::planner
