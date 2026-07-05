/**
 * @file sensor_abstraction_unit_test.cpp
 * @brief Validates the generic sensor abstraction layer.
 */

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/Sensor.h>
#include <humanoid/perception/SensorCapabilities.h>
#include <humanoid/perception/SensorFactory.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorState.h>
#include <humanoid/perception/SensorType.h>

namespace {

void Require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

[[nodiscard]] humanoid::common::Status Ok() { return humanoid::common::Status::ok(); }

class TestSensor final : public humanoid::perception::Sensor {
public:
  explicit TestSensor(humanoid::perception::SensorType type) : type_(type) {}

  [[nodiscard]] humanoid::common::Status Initialize() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Initialized;
    return Ok();
  }

  [[nodiscard]] humanoid::common::Status Shutdown() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Shutdown;
    return Ok();
  }

  [[nodiscard]] humanoid::common::Status Start() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Running;
    return Ok();
  }

  [[nodiscard]] humanoid::common::Status Stop() override {
    lifecycle_ = humanoid::perception::SensorLifecycleState::Stopped;
    return Ok();
  }

  [[nodiscard]] humanoid::perception::SensorFrameResult ReadFrame() override {
    humanoid::perception::SensorFrame frame;
    frame.id = ++frame_id_;
    frame.sensorType = type_;
    frame.timestamp =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
    frame.width = 2U;
    frame.height = 2U;
    frame.channels = 1U;
    frame.bytesPerElement = 1U;
    frame.encoding = "mono8";
    frame.data = std::vector<std::byte>{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    frame.metadata.emplace("source", std::string{"unit-test"});
    return humanoid::perception::SensorFrameResult{Ok(), std::move(frame)};
  }

  [[nodiscard]] humanoid::perception::SensorCapabilities Capabilities() const override {
    humanoid::perception::SensorCapabilities capabilities;
    capabilities.sensorId = "test.camera";
    capabilities.name = "Test Camera";
    capabilities.type = type_;
    capabilities.supportedEncodings = {"mono8"};
    capabilities.maximumWidth = 2U;
    capabilities.maximumHeight = 2U;
    capabilities.maximumFrameRateHz = 30.0;
    capabilities.supportsConfiguration = true;
    capabilities.nominalReadTimeout = std::chrono::milliseconds{10};
    return capabilities;
  }

  [[nodiscard]] humanoid::perception::SensorHealth Health() const override {
    humanoid::perception::SensorHealth health;
    health.status = humanoid::perception::SensorHealthStatus::Healthy;
    health.message = "test sensor healthy";
    return health;
  }

  [[nodiscard]] humanoid::perception::SensorConfiguration Configuration() const override {
    humanoid::perception::SensorConfiguration configuration;
    configuration.emplace("exposure_ms", 5.0);
    configuration.emplace("auto_gain", true);
    return configuration;
  }

  [[nodiscard]] humanoid::perception::SensorLifecycleState lifecycle() const noexcept {
    return lifecycle_;
  }

private:
  humanoid::perception::SensorType type_;
  humanoid::perception::SensorLifecycleState lifecycle_{
      humanoid::perception::SensorLifecycleState::Uninitialized};
  humanoid::perception::SensorFrameId frame_id_{0U};
};

[[nodiscard]] humanoid::perception::SensorCapabilities MakeCapabilities() {
  humanoid::perception::SensorCapabilities capabilities;
  capabilities.sensorId = "test.camera";
  capabilities.name = "Test Camera";
  capabilities.type = humanoid::perception::SensorType::Camera;
  capabilities.supportedEncodings = {"mono8"};
  capabilities.maximumWidth = 640U;
  capabilities.maximumHeight = 480U;
  capabilities.maximumFrameRateHz = 30.0;
  capabilities.supportsHardwareTimestamp = true;
  capabilities.supportsConfiguration = true;
  capabilities.nominalReadTimeout = std::chrono::milliseconds{33};
  return capabilities;
}

void VerifySensorTypesAndCapabilities() {
  Require(humanoid::perception::toString(humanoid::perception::SensorType::Camera) == "Camera");
  Require(humanoid::perception::toString(humanoid::perception::SensorType::DepthCamera) ==
          "DepthCamera");
  Require(humanoid::perception::toString(humanoid::perception::SensorType::Lidar) == "Lidar");
  Require(humanoid::perception::toString(humanoid::perception::SensorType::Microphone) ==
          "Microphone");
  Require(humanoid::perception::toString(humanoid::perception::SensorType::Imu) == "Imu");
  Require(humanoid::perception::toString(humanoid::perception::SensorType::Radar) == "Radar");
  Require(humanoid::perception::toString(humanoid::perception::SensorType::Custom) == "Custom");

  const humanoid::perception::SensorCapabilities capabilities = MakeCapabilities();
  Require(capabilities.isValid());
  Require(capabilities.type == humanoid::perception::SensorType::Camera);
  Require(capabilities.supportedEncodings.size() == 1U);
}

void VerifySensorLifecycleFrameHealthAndConfiguration() {
  TestSensor sensor{humanoid::perception::SensorType::Camera};
  Require(sensor.Initialize().isOk());
  Require(sensor.lifecycle() == humanoid::perception::SensorLifecycleState::Initialized);
  Require(sensor.Start().isOk());
  Require(sensor.lifecycle() == humanoid::perception::SensorLifecycleState::Running);

  humanoid::perception::SensorFrameResult result = sensor.ReadFrame();
  Require(result.succeeded());
  Require(result.frame.hasIdentity());
  Require(result.frame.hasDimensions());
  Require(result.frame.sizeBytes() == 4U);
  Require(result.frame.sensorType == humanoid::perception::SensorType::Camera);

  const humanoid::perception::SensorHealth health = sensor.Health();
  Require(health.status == humanoid::perception::SensorHealthStatus::Healthy);
  Require(health.isUsable());

  const humanoid::perception::SensorConfiguration configuration = sensor.Configuration();
  Require(configuration.find("exposure_ms") != configuration.end());
  Require(configuration.find("auto_gain") != configuration.end());

  Require(sensor.Stop().isOk());
  Require(sensor.lifecycle() == humanoid::perception::SensorLifecycleState::Stopped);
  Require(sensor.Shutdown().isOk());
  Require(sensor.lifecycle() == humanoid::perception::SensorLifecycleState::Shutdown);
}

void VerifyFactoryRegistrationCreationAndRemoval() {
  humanoid::perception::SensorFactory factory;

  Require(factory
              .RegisterSensor(MakeCapabilities(),
                              []() {
                                return std::make_unique<TestSensor>(
                                    humanoid::perception::SensorType::Camera);
                              })
              .isOk());
  Require(factory.Size() == 1U);
  Require(factory.Contains("test.camera"));
  Require(!factory
               .RegisterSensor(MakeCapabilities(),
                               []() {
                                 return std::make_unique<TestSensor>(
                                     humanoid::perception::SensorType::Camera);
                               })
               .isOk());

  const std::vector<humanoid::perception::SensorCapabilities> sensors = factory.EnumerateSensors();
  Require(sensors.size() == 1U);
  Require(sensors.front().sensorId == "test.camera");

  humanoid::perception::SensorCreationResult created = factory.CreateSensor("test.camera");
  Require(created.status.isOk());
  Require(static_cast<bool>(created.sensor));
  Require(created.sensor->Capabilities().type == humanoid::perception::SensorType::Camera);

  Require(factory.UnregisterSensor("test.camera").isOk());
  Require(factory.Size() == 0U);
  Require(!factory.CreateSensor("test.camera").status.isOk());
}

} // namespace

int main() {
  VerifySensorTypesAndCapabilities();
  VerifySensorLifecycleFrameHealthAndConfiguration();
  VerifyFactoryRegistrationCreationAndRemoval();
  return EXIT_SUCCESS;
}
