#include <cstdlib>
#include <iostream>

#include <humanoid/ros2/actions/ActionCatalog.h>
#include <humanoid/ros2/topics/TopicCatalog.h>

int main() {
  humanoid::ros2::topics::TopicCatalog topics{};
  humanoid::ros2::actions::ActionCatalog actions{};
  std::cout << "ROS2 Behavior Tree topic: " << topics.publishers.behaviorTreeStatus
            << " action: " << actions.executeBehaviorTree << '\n';
  return EXIT_SUCCESS;
}
