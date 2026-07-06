#include <cstdlib>
#include <iostream>

#include <humanoid/ros2/topics/TopicCatalog.h>

int main() {
  humanoid::ros2::topics::TopicCatalog topics{};
  std::cout << "ROS2 Plugin discovery boundary uses topic " << topics.publishers.diagnostics
            << '\n';
  return EXIT_SUCCESS;
}
