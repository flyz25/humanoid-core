#include <humanoid/cloud/fleet/FleetManager.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::fleet::FleetManager fleet;
  humanoid::cloud::fleet::RobotRecord robot{};
  robot.robotId = "robot-registration-example";
  robot.vendor = "Mock";
  robot.model = "MockRobot";

  if (!fleet.RegisterRobot(robot).ok()) {
    return EXIT_FAILURE;
  }

  const auto registered = fleet.FindRobot(robot.robotId);
  if (!registered.has_value()) {
    return EXIT_FAILURE;
  }

  std::cout << "Registered robot: " << registered->robotId << '\n';
  return EXIT_SUCCESS;
}
