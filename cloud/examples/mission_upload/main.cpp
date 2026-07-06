#include <humanoid/cloud/fleet/FleetManager.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::fleet::FleetManager fleet;
  humanoid::cloud::fleet::RobotRecord robot{};
  robot.robotId = "robot-001";
  robot.vendor = "Simulator";
  robot.model = "SimRobot";

  if (!fleet.RegisterRobot(robot).ok() || !fleet.AddRobotToGroup("demo", robot.robotId).ok()) {
    return EXIT_FAILURE;
  }
  if (!fleet.DistributeMission("demo", "uploaded-mission-001").ok()) {
    return EXIT_FAILURE;
  }

  std::cout << "Mission distributions: " << fleet.MissionDistributions().size() << '\n';
  return EXIT_SUCCESS;
}
