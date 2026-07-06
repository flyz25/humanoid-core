#include <cstdlib>
#include <iostream>

#include <humanoid/ros2/messages/ROS2MessageTypes.h>
#include <humanoid/ros2/topics/TopicCatalog.h>

int main() {
  humanoid::ros2::messages::PlannerStatusMessage planner{};
  planner.plannerId = "rule-planner";
  planner.goalId = "goal-wave";
  planner.status = "Ready";

  humanoid::ros2::topics::TopicCatalog topics{};
  std::cout << "ROS2 Planner publishes " << planner.status << " on "
            << topics.publishers.plannerStatus << '\n';
  return EXIT_SUCCESS;
}
