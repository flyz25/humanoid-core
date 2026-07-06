#include <humanoid/cloud/fleet/FleetManager.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::fleet::FleetManager fleet;

  humanoid::cloud::fleet::RobotRecord robot{};
  robot.robotId = "g1-stage-001";
  robot.vendor = "Unitree";
  robot.model = "G1";
  robot.capabilities = {"telemetry", "mission"};

  if (!fleet.RegisterRobot(robot).ok()) {
    return EXIT_FAILURE;
  }
  if (!fleet.Heartbeat(robot.robotId, true).ok()) {
    return EXIT_FAILURE;
  }
  if (!fleet.AddRobotToGroup("stage", robot.robotId).ok()) {
    return EXIT_FAILURE;
  }
  if (!fleet.DistributeMission("stage", "mission-greeting").ok()) {
    return EXIT_FAILURE;
  }

  const auto status = fleet.Status();
  std::cout << "Fleet robots: " << status.robotCount << ", online: " << status.onlineCount << '\n';
  return EXIT_SUCCESS;
}
