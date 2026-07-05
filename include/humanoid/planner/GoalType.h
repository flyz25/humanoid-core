#pragma once

/**
 * @file GoalType.h
 * @brief Defines vendor-independent user-intent goal categories.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::planner {

/**
 * @brief Classifies the high-level intent represented by a planner goal.
 *
 * Goal type is descriptive planner input. It must not encode vendor SDK
 * commands, robot-specific modes, or concrete execution implementation types.
 */
enum class GoalType : std::uint8_t {
  Mission,      ///< Intent that can be satisfied by a mission.
  BehaviorTree, ///< Intent that can be satisfied by a behavior tree.
  Command,      ///< Intent that can be satisfied by one or more generic commands.
  Inspection,   ///< Intent to inspect or observe an environment or robot state.
  Interaction,  ///< Intent involving human-facing interaction.
  Custom        ///< Application-defined intent outside built-in categories.
};

/**
 * @brief Returns the stable diagnostic name of a goal type.
 *
 * @param type Goal type to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(GoalType type) noexcept {
  switch (type) {
  case GoalType::Mission:
    return "Mission";
  case GoalType::BehaviorTree:
    return "BehaviorTree";
  case GoalType::Command:
    return "Command";
  case GoalType::Inspection:
    return "Inspection";
  case GoalType::Interaction:
    return "Interaction";
  case GoalType::Custom:
    return "Custom";
  }

  return "Unknown";
}

} // namespace humanoid::planner
