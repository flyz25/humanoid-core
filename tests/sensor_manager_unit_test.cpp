/**
 * @file sensor_manager_unit_test.cpp
 * @brief Validates the generic sensor manager.
 */

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/Sensor.h>
#include <humanoid/perception/SensorCapabilities.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorManager.h>
#include <humanoid/perception/SensorState.h>
#include <humanoid/perception/SensorType.h>

namespace {

void Require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

[[nodiscard]] humanoid::common::Status Ok() { return humanoid::common::Status::ok(); }

[[nodiscard]] humanoid::perception::SensorCapabilities
MakeCapabilities(std::string id, humanoid::perception::SensorType type) {
  humanoid::perception::SensorCapabilities capabilities;
  capabilities.sensorId = std::move(id);
  capabilities.name = capabilities.sensorId;
  capabilities.type = type;
  capabilities.supportedEncodings = {"bytes"};
  capabilities.maximumFrameRateHz = 1000.0;
  capabilities.nominalReadTimeout = std::chrono::milliseconds{5};
  return capabilities;
}

class CountingSensor final : public humanoid::perception::Sensor {
public:
  explicit CountingSensor(humanoid::perception::SensorCapabilities capabilities)
      : capabilities_(std::move(capabilities)) {}

  [[nodiscard]] humanoid::common::Status Initialize() override {
    std::lock_guard<std::mutex> lock{mutex_};
    lifecycle_ = humanoid::perception::SensorLifecycleState::Initialized;
    return Ok();
  }

  [[nodiscard]] humanoid::common::Status Shutdown() override {
    std::lock_guard<std::mutex> lock{mutex_};
    lifecycle_ = humanoid::perception::SensorLifecycleState::Shutdown;
    return Ok();
  }

  [[nodiscard]] humanoid::common::Status Start() override {
    std::lock_guard<std::mutex> lock{mutex_};
    lifecycle_ = humanoid::perception::SensorLifecycleState::Running;
    return Ok();
  }

  [[nodiscard]] humanoid::common::Status Stop() override {
    std::lock_guard<std::mutex> lock{mutex_};
    lifecycle_ = humanoid::perception::SensorLifecycleState::Stopped;
    return Ok();
  }

  [[nodiscard]] humanoid::perception::SensorFrameResult ReadFrame() override {
    std::lock_guard<std::mutex> lock{mutex_};
    humanoid::perception::SensorFrame frame;
    frame.id = ++frame_id_;
    frame.sensorType = capabilities_.type;
    frame.channels = 1U;
    frame.bytesPerElement = 1U;
    frame.encoding = "bytes";
    frame.data = std::vector<std::byte>{std::byte{static_cast<unsigned char>(frame.id % 255U)}};
    return humanoid::perception::SensorFrameResult{Ok(), std::move(frame)};
  }

  [[nodiscard]] humanoid::perception::SensorCapabilities Capabilities() const override {
    std::lock_guard<std::mutex> lock{mutex_};
    return capabilities_;
  }

  [[nodiscard]] humanoid::perception::SensorHealth Health() const override {
    std::lock_guard<std::mutex> lock{mutex_};
    humanoid::perception::SensorHealth health;
    health.status = humanoid::perception::SensorHealthStatus::Healthy;
    health.message = "healthy";
    return health;
  }

  [[nodiscard]] humanoid::perception::SensorConfiguration Configuration() const override {
    humanoid::perception::SensorConfiguration configuration;
    configuration.emplace("enabled", true);
    return configuration;
  }

private:
  humanoid::perception::SensorCapabilities capabilities_;
  mutable std::mutex mutex_;
  humanoid::perception::SensorLifecycleState lifecycle_{
      humanoid::perception::SensorLifecycleState::Uninitialized};
  humanoid::perception::SensorFrameId frame_id_{0U};
};

std::unique_ptr<humanoid::perception::Sensor> MakeSensor(std::string id,
                                                         humanoid::perception::SensorType type) {
  return std::make_unique<CountingSensor>(MakeCapabilities(std::move(id), type));
}

void VerifyRegistrationDiscoveryHealthAndHotPlug() {
  humanoid::perception::SensorManager manager;

  Require(
      manager.RegisterSensor(MakeSensor("camera.front", humanoid::perception::SensorType::Camera))
          .isOk());
  Require(
      manager.RegisterSensor(MakeSensor("imu.body", humanoid::perception::SensorType::Imu)).isOk());
  Require(manager.Size() == 2U);
  Require(manager.Contains("camera.front"));

  const std::vector<humanoid::perception::SensorCapabilities> discovered =
      manager.DiscoverSensors();
  Require(discovered.size() == 2U);
  Require(discovered.front().sensorId == "camera.front");
  Require(discovered.back().sensorId == "imu.body");

  Require(manager.InitializeSensor("camera.front").isOk());
  Require(manager.StartSensor("camera.front").isOk());
  const humanoid::perception::SensorStateResult state = manager.State("camera.front");
  Require(state.succeeded());
  Require(state.state.lifecycle == humanoid::perception::SensorLifecycleState::Running);

  const humanoid::perception::SensorHealthResult health = manager.Health("camera.front");
  Require(health.succeeded());
  Require(health.health.status == humanoid::perception::SensorHealthStatus::Healthy);

  const std::vector<humanoid::perception::SensorHealthResult> report = manager.HealthReport();
  Require(report.size() == 2U);

  Require(manager.UnregisterSensor("imu.body").isOk());
  Require(!manager.Contains("imu.body"));
  Require(manager.Size() == 1U);
  Require(!manager.ReadFrame("imu.body").succeeded());
}

void VerifyFrameRoutingTimestampingAndLatestFrame() {
  humanoid::perception::SensorManager manager;
  Require(
      manager.RegisterSensor(MakeSensor("camera.front", humanoid::perception::SensorType::Camera))
          .isOk());

  std::atomic<std::uint64_t> callback_count{0U};
  const humanoid::perception::SensorFrameSubscriptionResult subscription =
      manager.Subscribe([&callback_count](const std::string& sensor_id,
                                          const humanoid::perception::SensorFrame& frame) {
        Require(sensor_id == "camera.front");
        Require(frame.hasIdentity());
        Require(frame.timestamp != humanoid::perception::SensorFrameTimestamp{});
        ++callback_count;
      });
  Require(subscription.succeeded());

  const humanoid::perception::SensorManagerFrameResult frame = manager.ReadFrame("camera.front");
  Require(frame.succeeded());
  Require(frame.frame.timestamp != humanoid::perception::SensorFrameTimestamp{});
  Require(callback_count.load() == 1U);

  const humanoid::perception::SensorManagerFrameResult latest = manager.LatestFrame("camera.front");
  Require(latest.succeeded());
  Require(latest.frame.id == frame.frame.id);

  const std::vector<humanoid::perception::SensorManagerFrameResult> frames =
      manager.ReadAllFrames();
  Require(frames.size() == 1U);
  Require(frames.front().succeeded());
  Require(callback_count.load() == 2U);

  Require(manager.Unsubscribe(subscription.subscriptionId).isOk());
  Require(manager.ReadFrame("camera.front").succeeded());
  Require(callback_count.load() == 2U);

  const humanoid::perception::SensorManagerStatistics statistics = manager.GetStatistics();
  Require(statistics.routedFrames == 3U);
  Require(statistics.listenerCallbacks == 2U);
}

void VerifyConcurrentReads() {
  humanoid::perception::SensorManager manager;
  Require(
      manager.RegisterSensor(MakeSensor("camera.front", humanoid::perception::SensorType::Camera))
          .isOk());
  Require(manager.RegisterSensor(MakeSensor("lidar.top", humanoid::perception::SensorType::Lidar))
              .isOk());

  constexpr std::size_t kThreadCount = 8U;
  constexpr std::size_t kReadsPerThread = 64U;
  std::atomic<std::uint64_t> successful_reads{0U};
  std::vector<std::jthread> threads;
  threads.reserve(kThreadCount);

  for (std::size_t thread_index = 0U; thread_index < kThreadCount; ++thread_index) {
    threads.emplace_back([&manager, &successful_reads, thread_index]() {
      const std::string sensor_id = (thread_index % 2U == 0U) ? "camera.front" : "lidar.top";
      for (std::size_t read_index = 0U; read_index < kReadsPerThread; ++read_index) {
        if (manager.ReadFrame(sensor_id).succeeded()) {
          ++successful_reads;
        }
      }
    });
  }

  threads.clear();

  Require(successful_reads.load() == kThreadCount * kReadsPerThread);
  const humanoid::perception::SensorManagerStatistics statistics = manager.GetStatistics();
  Require(statistics.routedFrames == kThreadCount * kReadsPerThread);
  Require(statistics.readFailures == 0U);

  const humanoid::perception::SensorStateResult camera_state = manager.State("camera.front");
  const humanoid::perception::SensorStateResult lidar_state = manager.State("lidar.top");
  Require(camera_state.succeeded());
  Require(lidar_state.succeeded());
  Require(camera_state.state.framesProduced + lidar_state.state.framesProduced ==
          kThreadCount * kReadsPerThread);
}

} // namespace

int main() {
  VerifyRegistrationDiscoveryHealthAndHotPlug();
  VerifyFrameRoutingTimestampingAndLatestFrame();
  VerifyConcurrentReads();
  return EXIT_SUCCESS;
}
