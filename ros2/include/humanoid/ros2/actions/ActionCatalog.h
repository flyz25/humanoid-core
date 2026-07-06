#pragma once

/**
 * @file ActionCatalog.h
 * @brief Defines ROS2 action names used by the optional bridge.
 */

#include <cstddef>
#include <string_view>

namespace humanoid::ros2::actions {

/**
 * @brief ROS2 action names exposed by the bridge.
 */
struct ActionCatalog final {
  std::string_view executeMission{"/humanoid/action/execute_mission"}; ///< Execute mission.
  std::string_view executeBehaviorTree{"/humanoid/action/execute_bt"}; ///< Execute BT.
  std::string_view navigate{"/humanoid/action/navigate"};              ///< Navigate.
  std::string_view wave{"/humanoid/action/wave"};                      ///< Wave.
  std::string_view greeting{"/humanoid/action/greeting"};              ///< Greeting.
  std::string_view inspection{"/humanoid/action/inspection"};          ///< Inspection.
  std::string_view custom{"/humanoid/action/custom"};                  ///< Custom action.
};

/**
 * @brief Returns the number of action endpoints in the catalog.
 */
[[nodiscard]] constexpr std::size_t actionCount() noexcept { return 7U; }

} // namespace humanoid::ros2::actions
