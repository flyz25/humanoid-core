#include <cstdlib>
#include <iostream>

#include <humanoid/ros2/bridge/ROS2Bridge.h>

int main() {
  humanoid::ros2::bridge::ROS2Bridge bridge{{}};
  const auto status = bridge.Status();
  std::cout << "ROS2 Mission services prepared: " << status.serviceCount
            << " services configured\n";
  return EXIT_SUCCESS;
}
