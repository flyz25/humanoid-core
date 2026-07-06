#include <cstdlib>
#include <iostream>
#include <memory>

#include <humanoid/ros2/bridge/ROS2Bridge.h>
#include <humanoid/runtime/ExecutionContext.h>
#include <humanoid/runtime/ExecutionScope.h>

int main() {
  humanoid::ros2::bridge::ROS2BridgeDependencies dependencies{};
  dependencies.executionContext = std::make_shared<humanoid::runtime::ExecutionContext>(
      7U, humanoid::runtime::ExecutionScope::ROS2);

  humanoid::ros2::bridge::ROS2Bridge bridge{std::move(dependencies)};
  const auto status = bridge.Status();
  std::cout << "ROS2 Runtime bridge node: " << status.nodeName << '\n';
  return EXIT_SUCCESS;
}
