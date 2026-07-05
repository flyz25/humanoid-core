/**
 * @file main.cpp
 * @brief Demonstrates timestamp alignment for sensor fusion.
 */

#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorFusion.h>
#include <humanoid/perception/SensorType.h>

namespace {

[[nodiscard]] humanoid::perception::SensorFrame
MakeFrame(humanoid::perception::SensorType type,
          humanoid::perception::SensorFrameTimestamp timestamp) {
  humanoid::perception::SensorFrame frame;
  frame.id = static_cast<humanoid::perception::SensorFrameId>(type);
  frame.sensorType = type;
  frame.timestamp = timestamp;
  frame.data = std::vector<std::byte>{std::byte{1}};
  return frame;
}

} // namespace

int main() {
  const auto timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());

  humanoid::perception::FrameSynchronizationPolicy policy;
  policy.maximumSkew = std::chrono::milliseconds{5};
  policy.requiredSensorTypes = {humanoid::perception::SensorType::Camera,
                                humanoid::perception::SensorType::Imu};

  humanoid::perception::FrameSynchronizer synchronizer{policy};
  std::vector<humanoid::perception::SensorFrame> frames;
  frames.push_back(MakeFrame(humanoid::perception::SensorType::Camera, timestamp));
  frames.push_back(
      MakeFrame(humanoid::perception::SensorType::Imu, timestamp + std::chrono::milliseconds{1}));

  const humanoid::perception::SynchronizedFrameSet synchronized_frames =
      synchronizer.Synchronize(std::move(frames));
  if (!synchronized_frames.succeeded()) {
    return 1;
  }

  std::cout << "Synchronized frames=" << synchronized_frames.frames.size() << '\n';
  return 0;
}
