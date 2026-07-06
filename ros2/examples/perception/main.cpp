#include <cstdlib>
#include <iostream>

#include <humanoid/perception/DetectionResult.h>
#include <humanoid/ros2/messages/ROS2MessageTypes.h>

int main() {
  humanoid::perception::DetectionResult detection{};
  detection.id = "qr-001";
  detection.type = humanoid::perception::DetectionType::QrCode;
  detection.label = "station-a";
  detection.confidence = 0.99;
  detection.boundingBox = humanoid::perception::BoundingBox2D{12.0, 16.0, 80.0, 80.0};

  const auto message = humanoid::ros2::messages::FromDetectionResult(detection);
  std::cout << "ROS2 Perception detection: " << message.type << " label=" << message.label << '\n';
  return EXIT_SUCCESS;
}
