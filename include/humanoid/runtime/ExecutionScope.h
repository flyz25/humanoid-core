#pragma once

/**
 * @file ExecutionScope.h
 * @brief Defines framework-level execution scope categories.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::runtime {

/**
 * @brief Identifies the framework subsystem that owns an execution context.
 *
 * Scope values are descriptive only and introduce no dependency on an
 * execution engine or integration middleware.
 */
enum class ExecutionScope : std::uint8_t {
  Unknown,      ///< The execution owner has not been assigned.
  Mission,      ///< A mission framework execution.
  BehaviorTree, ///< A future behavior-tree execution.
  AIPlanner,    ///< A future AI planner execution.
  ROS2,         ///< A future ROS2 integration execution.
  Custom        ///< A downstream framework extension.
};

/**
 * @brief Returns the stable name of an execution scope.
 *
 * @param scope Scope to inspect.
 * @return Non-owning stable scope name.
 */
[[nodiscard]] constexpr std::string_view toString(ExecutionScope scope) noexcept {
  switch (scope) {
  case ExecutionScope::Unknown:
    return "Unknown";
  case ExecutionScope::Mission:
    return "Mission";
  case ExecutionScope::BehaviorTree:
    return "BehaviorTree";
  case ExecutionScope::AIPlanner:
    return "AIPlanner";
  case ExecutionScope::ROS2:
    return "ROS2";
  case ExecutionScope::Custom:
    return "Custom";
  }

  return "Unknown";
}

} // namespace humanoid::runtime
