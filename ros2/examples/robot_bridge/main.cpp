#include <cstdlib>
#include <iostream>
#include <memory>

#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/ros2/bridge/ROS2Bridge.h>

int main() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state{};
  state.connection.connected = true;
  state.power.batteryLevel = 88.0F;
  state.motion.standing = true;
  state_manager->UpdateState(state);

  humanoid::ros2::bridge::ROS2BridgeDependencies dependencies{};
  dependencies.robotStateManager = state_manager;

  humanoid::ros2::bridge::ROS2Bridge bridge{std::move(dependencies)};
  static_cast<void>(bridge.Start());
  const auto message = bridge.ReadRobotState();

  std::cout << "ROS2 Robot Bridge: connected=" << message.connected
            << " battery=" << message.batteryLevel << '\n';
  bridge.Stop();
  return EXIT_SUCCESS;
}
