#pragma once

/**
 * @file GoalStatus.h
 * @brief Defines vendor-independent goal lifecycle states.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::planner {

/**
 * @brief Describes the lifecycle state of a high-level planning goal.
 *
 * Goal status is a model-level state. It does not imply that a mission,
 * behavior tree, command, adapter, or SDK operation has been executed.
 */
enum class GoalStatus : std::uint8_t {
  Pending,  ///< Goal has been created but not accepted for planning.
  Accepted, ///< Goal has been accepted by a future planner boundary.
  Planning, ///< Goal is being converted into an executable representation.
  Planned,  ///< Planning produced a usable result.
  Rejected, ///< Goal was rejected before planning.
  Failed,   ///< Planning failed after acceptance.
  Cancelled ///< Goal processing was cancelled cooperatively.
};

/**
 * @brief Returns the stable diagnostic name of a goal status.
 *
 * @param status Goal status to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(GoalStatus status) noexcept {
  switch (status) {
  case GoalStatus::Pending:
    return "Pending";
  case GoalStatus::Accepted:
    return "Accepted";
  case GoalStatus::Planning:
    return "Planning";
  case GoalStatus::Planned:
    return "Planned";
  case GoalStatus::Rejected:
    return "Rejected";
  case GoalStatus::Failed:
    return "Failed";
  case GoalStatus::Cancelled:
    return "Cancelled";
  }

  return "Unknown";
}

/**
 * @brief Reports whether a goal status is terminal.
 *
 * @param status Goal status to inspect.
 * @return True when no additional planning lifecycle transition is expected.
 */
[[nodiscard]] constexpr bool isTerminal(GoalStatus status) noexcept {
  return status == GoalStatus::Planned || status == GoalStatus::Rejected ||
         status == GoalStatus::Failed || status == GoalStatus::Cancelled;
}

} // namespace humanoid::planner
