/**
 * @file main.cpp
 * @brief Demonstrates generic camera sensor management.
 */

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <humanoid/perception/Sensor.h>
#include <humanoid/perception/SensorCapabilities.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorManager.h>
#include <humanoid/perception/SensorState.h>
#include <humanoid/perception/SensorType.h>

namespace {

class MemoryCamera final : public humanoid::perception::Sensor {
public:
  [[nodiscard]] humanoid::common::Status Initialize() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Initialized;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Shutdown() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Shutdown;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Start() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Running;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Stop() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Stopped;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::perception::SensorFrameResult ReadFrame() override {
    humanoid::perception::SensorFrame frame;
    frame.id = ++frame_id_;
    frame.sensorType = humanoid::perception::SensorType::Camera;
    frame.timestamp =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
    frame.width = 2U;
    frame.height = 2U;
    frame.channels = 3U;
    frame.bytesPerElement = 1U;
    frame.encoding = "rgb8";
    frame.data =
        std::vector<std::byte>{std::byte{255}, std::byte{0},   std::byte{0},   std::byte{0},
                               std::byte{255}, std::byte{0},   std::byte{0},   std::byte{0},
                               std::byte{255}, std::byte{255}, std::byte{255}, std::byte{255}};
    return humanoid::perception::SensorFrameResult{humanoid::common::Status::ok(),
                                                   std::move(frame)};
  }

  [[nodiscard]] humanoid::perception::SensorCapabilities Capabilities() const override {
    humanoid::perception::SensorCapabilities capabilities;
    capabilities.sensorId = "memory.camera";
    capabilities.name = "Memory Camera";
    capabilities.type = humanoid::perception::SensorType::Camera;
    capabilities.supportedEncodings = {"rgb8"};
    capabilities.maximumWidth = 2U;
    capabilities.maximumHeight = 2U;
    capabilities.maximumFrameRateHz = 30.0;
    return capabilities;
  }

  [[nodiscard]] humanoid::perception::SensorHealth Health() const override {
    humanoid::perception::SensorHealth health;
    health.status = humanoid::perception::SensorHealthStatus::Healthy;
    health.message = "memory camera healthy";
    return health;
  }

  [[nodiscard]] humanoid::perception::SensorConfiguration Configuration() const override {
    humanoid::perception::SensorConfiguration configuration;
    configuration.emplace("width", 2);
    configuration.emplace("height", 2);
    return configuration;
  }

private:
  humanoid::perception::SensorLifecycleState lifecycle_{
      humanoid::perception::SensorLifecycleState::Uninitialized};
  humanoid::perception::SensorFrameId frame_id_{0U};
};

} // namespace

int main() {
  humanoid::perception::SensorManager manager;
  if (!manager.RegisterSensor(std::make_unique<MemoryCamera>()).isOk()) {
    return 1;
  }
  if (!manager.InitializeSensor("memory.camera").isOk() ||
      !manager.StartSensor("memory.camera").isOk()) {
    return 1;
  }

  const humanoid::perception::SensorManagerFrameResult frame = manager.ReadFrame("memory.camera");
  if (!frame.succeeded()) {
    return 1;
  }

  std::cout << "Camera frame " << frame.frame.id << " bytes=" << frame.frame.sizeBytes() << '\n';
  return 0;
}
