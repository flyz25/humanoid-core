#pragma once

/**
 * @file Goal.h
 * @brief Defines the generic, vendor-independent user-intent goal model.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>

#include <humanoid/planner/GoalPriority.h>
#include <humanoid/planner/GoalStatus.h>
#include <humanoid/planner/GoalType.h>

namespace humanoid::planner {

/**
 * @brief Stable planning goal identifier type.
 *
 * A value of zero is reserved for an unassigned identifier. Applications,
 * operator interfaces, or future goal producers own identifier generation.
 */
using GoalId = std::uint64_t;

/**
 * @brief Invalid or unassigned goal identifier.
 */
inline constexpr GoalId kInvalidGoalId{0U};

/**
 * @brief Monotonic timestamp associated with goal creation or revision.
 */
using GoalTimestamp = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Supported scalar value types for goal constraints and context.
 *
 * Goal data must remain framework-owned. Vendor SDK objects, transport handles,
 * robot-specific enums, and pointers are not valid goal values.
 */
using GoalValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered collection of named goal constraints.
 *
 * Constraints describe requirements that a future planner should satisfy, such
 * as time budgets, allowed zones, interaction limits, or policy labels. They
 * are not executable commands.
 */
using GoalConstraints = std::map<std::string, GoalValue, std::less<>>;

/**
 * @brief Ordered collection of named situational goal context values.
 *
 * Context describes non-authoritative planning input supplied by an
 * application, operator interface, or future execution engine.
 */
using GoalContext = std::map<std::string, GoalValue, std::less<>>;

/**
 * @brief Ordered collection of non-operational goal annotations.
 *
 * Metadata may carry correlation IDs, source information, tracing labels, or
 * authoring hints. Runtime behavior must not silently depend on metadata.
 */
using GoalMetadata = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Generic user-intent model consumed by future planning components.
 *
 * `Goal` is a value type that describes intent without selecting a concrete
 * planner, mission executor, behavior tree, robot adapter, or vendor SDK. It
 * contains only framework-owned standard-library data and no global identifier
 * generator.
 */
struct Goal final {
  /**
   * @brief Unique, producer-assigned goal identifier; zero is unassigned.
   */
  GoalId id{kInvalidGoalId};

  /**
   * @brief Vendor-independent category of user intent.
   */
  GoalType type{GoalType::Custom};

  /**
   * @brief Human-readable intent description.
   */
  std::string description;

  /**
   * @brief Relative planning urgency.
   */
  GoalPriority priority{GoalPriority::Normal};

  /**
   * @brief Current model-level planning lifecycle state.
   */
  GoalStatus status{GoalStatus::Pending};

  /**
   * @brief Named requirements that a future planner should satisfy.
   */
  GoalConstraints constraints;

  /**
   * @brief Named situational input available to a future planner.
   */
  GoalContext context;

  /**
   * @brief Non-operational correlation and tracing annotations.
   */
  GoalMetadata metadata;

  /**
   * @brief Monotonic time at which the goal was created or revised.
   */
  GoalTimestamp timestamp{};

  /**
   * @brief Constructs an unassigned custom goal with normal priority.
   */
  Goal() = default;

  /**
   * @brief Reports whether the goal has the minimum usable identity fields.
   *
   * @return True when the identifier is assigned and description is not empty.
   */
  [[nodiscard]] bool isValid() const noexcept {
    return id != kInvalidGoalId && !description.empty();
  }

  /**
   * @brief Reports whether the goal lifecycle state is terminal.
   *
   * @return True when no additional goal planning transition is expected.
   */
  [[nodiscard]] constexpr bool isTerminal() const noexcept {
    return humanoid::planner::isTerminal(status);
  }
};

} // namespace humanoid::planner
