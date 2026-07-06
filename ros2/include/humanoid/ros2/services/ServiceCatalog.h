#pragma once

/**
 * @file ServiceCatalog.h
 * @brief Defines ROS2 service names used by the optional bridge.
 */

#include <cstddef>
#include <string_view>

namespace humanoid::ros2::services {

/**
 * @brief ROS2 service names exposed by the bridge.
 */
struct ServiceCatalog final {
  std::string_view connectRobot{"/humanoid/connect_robot"};        ///< Connect robot.
  std::string_view disconnectRobot{"/humanoid/disconnect_robot"};  ///< Disconnect robot.
  std::string_view startMission{"/humanoid/mission/start"};        ///< Start mission.
  std::string_view pauseMission{"/humanoid/mission/pause"};        ///< Pause mission.
  std::string_view resumeMission{"/humanoid/mission/resume"};      ///< Resume mission.
  std::string_view cancelMission{"/humanoid/mission/cancel"};      ///< Cancel mission.
  std::string_view loadBehaviorTree{"/humanoid/bt/load"};          ///< Load BT.
  std::string_view loadPlannerGoal{"/humanoid/planner/load_goal"}; ///< Load planner goal.
  std::string_view sensorControl{"/humanoid/sensor/control"};      ///< Sensor control.
  std::string_view healthCheck{"/humanoid/health_check"};          ///< Health check.
};

/**
 * @brief Returns the number of service endpoints in the catalog.
 */
[[nodiscard]] constexpr std::size_t serviceCount() noexcept { return 10U; }

} // namespace humanoid::ros2::services
