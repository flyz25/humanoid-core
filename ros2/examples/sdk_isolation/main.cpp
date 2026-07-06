#include <cstdlib>
#include <iostream>

#include <humanoid/ros2/bridge/ROS2Bridge.h>

int main() {
  humanoid::ros2::bridge::ROS2Bridge bridge{{}};
  const auto status = bridge.Status();
  std::cout << "ROS2 SDK isolation verified for node " << status.nodeName
            << "; ROS2 runtime available=" << status.ros2RuntimeAvailable << '\n';
  return EXIT_SUCCESS;
}
