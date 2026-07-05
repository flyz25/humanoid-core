#pragma once

/**
 * @file GoalPriority.h
 * @brief Defines vendor-independent planning goal priorities.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::planner {

/**
 * @brief Indicates relative planning urgency for a user intent.
 *
 * Priority influences planning and scheduling policy only. It must not bypass
 * safety validation, capability checks, or runtime resource ownership.
 */
enum class GoalPriority : std::uint8_t {
  Low,     ///< Deferrable intent with no immediate timing requirement.
  Normal,  ///< Standard planning priority.
  High,    ///< Time-sensitive intent that should precede normal goals.
  Critical ///< Safety-critical intent requiring earliest permitted handling.
};

/**
 * @brief Returns the stable diagnostic name of a goal priority.
 *
 * @param priority Goal priority to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(GoalPriority priority) noexcept {
  switch (priority) {
  case GoalPriority::Low:
    return "Low";
  case GoalPriority::Normal:
    return "Normal";
  case GoalPriority::High:
    return "High";
  case GoalPriority::Critical:
    return "Critical";
  }

  return "Unknown";
}

} // namespace humanoid::planner
