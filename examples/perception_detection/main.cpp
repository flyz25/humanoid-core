/**
 * @file main.cpp
 * @brief Demonstrates generic detection result values.
 */

#include <chrono>
#include <iostream>

#include <humanoid/perception/DetectionResult.h>

int main() {
  humanoid::perception::DetectionResult result;
  result.id = "object-1";
  result.type = humanoid::perception::DetectionType::Object;
  result.confidence = 0.91;
  result.label = "inspection-target";
  result.boundingBox = humanoid::perception::BoundingBox2D{10.0, 20.0, 80.0, 60.0};
  result.timestamp = std::chrono::steady_clock::now();
  result.trackingId = "track-1";

  if (!result.hasIdentity() || !result.boundingBox.isValid()) {
    return 1;
  }

  std::cout << "Detection " << result.id << " label=" << result.label
            << " confidence=" << result.confidence << '\n';
  return 0;
}
