#include <humanoid/core/RobotState.hpp>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/services/TelemetryService.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

using humanoid::core::RobotState;
using humanoid::core::RobotStateManager;
using humanoid::services::TelemetryService;

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] bool TestRobotStateDefaults() {
  constexpr std::string_view kTestName{"RobotState defaults"};

  static_assert(std::is_standard_layout_v<RobotState>);
  static_assert(std::is_trivially_copyable_v<RobotState>);
  static_assert(noexcept(RobotState{}));

  constexpr RobotState state{};
  if (state.connection.connected || state.power.charging || state.motion.standing ||
      state.motion.walking || state.motion.sitting || state.health.emergencyStop) {
    return Fail(kTestName, "default boolean fields must be false");
  }

  if (state.power.batteryLevel != 0.0F || state.velocity.linearX != 0.0F ||
      state.velocity.linearY != 0.0F || state.velocity.angularZ != 0.0F ||
      state.pose.positionX != 0.0F || state.pose.positionY != 0.0F ||
      state.pose.positionZ != 0.0F || state.orientation.roll != 0.0F ||
      state.orientation.pitch != 0.0F || state.orientation.yaw != 0.0F) {
    return Fail(kTestName, "default numeric fields must be zero");
  }

  if (state.health.faultCode != 0) {
    return Fail(kTestName, "default fault code must be zero");
  }

  return true;
}

[[nodiscard]] bool TestRobotStateManagerUpdateAndReset() {
  constexpr std::string_view kTestName{"RobotStateManager update/reset"};

  RobotStateManager manager;
  RobotState state;
  state.connection.connected = true;
  state.power.batteryLevel = 87.0F;
  state.power.charging = true;
  state.motion.standing = true;
  state.velocity.linearX = 0.3F;
  state.velocity.linearY = -0.1F;
  state.velocity.angularZ = 0.2F;
  state.pose.positionX = 1.0F;
  state.pose.positionY = 2.0F;
  state.pose.positionZ = 0.5F;
  state.orientation.roll = 0.01F;
  state.orientation.pitch = -0.02F;
  state.orientation.yaw = 1.57F;
  state.health.emergencyStop = true;
  state.health.faultCode = 42;
  state.timestamp = humanoid::core::RobotStateTimestamp::clock::now();

  manager.UpdateState(state);

  const RobotState snapshot = manager.GetState();
  if (!snapshot.connection.connected || !snapshot.power.charging ||
      !snapshot.motion.standing || !snapshot.health.emergencyStop) {
    return Fail(kTestName, "snapshot did not preserve boolean fields");
  }

  if (manager.BatteryLevel() != 87.0F || manager.FaultCode() != 42 ||
      !manager.IsConnected() || !manager.EmergencyStop()) {
    return Fail(kTestName, "field accessors returned unexpected values");
  }

  if (snapshot.velocity.linearX != 0.3F || snapshot.velocity.linearY != -0.1F ||
      snapshot.velocity.angularZ != 0.2F || snapshot.pose.positionX != 1.0F ||
      snapshot.pose.positionY != 2.0F || snapshot.pose.positionZ != 0.5F ||
      snapshot.orientation.roll != 0.01F || snapshot.orientation.pitch != -0.02F ||
      snapshot.orientation.yaw != 1.57F) {
    return Fail(kTestName, "snapshot did not preserve motion, pose, or orientation fields");
  }

  manager.Reset();
  if (manager.IsConnected() || manager.EmergencyStop() || manager.BatteryLevel() != 0.0F ||
      manager.FaultCode() != 0) {
    return Fail(kTestName, "reset did not restore default state");
  }

  return true;
}

[[nodiscard]] bool TestRobotStateManagerThreadSafety() {
  constexpr std::string_view kTestName{"RobotStateManager thread safety"};
  constexpr std::size_t kWriterCount{4};
  constexpr std::size_t kReaderCount{4};
  constexpr std::size_t kIterations{4000};

  RobotStateManager manager;
  RobotState initial_state;
  initial_state.connection.connected = true;
  manager.UpdateState(initial_state);

  std::latch start_gate{1};
  std::atomic<std::size_t> read_count{0};
  std::atomic<std::size_t> connected_observations{0};
  std::vector<std::jthread> workers;
  workers.reserve(kWriterCount + kReaderCount);

  for (std::size_t writer_index = 0; writer_index < kWriterCount; ++writer_index) {
    workers.emplace_back([&manager, &start_gate, writer_index]() {
      start_gate.wait();

      RobotState state;
      for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
        state.connection.connected = ((iteration + writer_index) % 2U) == 0U;
        state.power.batteryLevel = static_cast<float>((iteration + writer_index) % 101U);
        state.health.faultCode = static_cast<std::int32_t>(writer_index);
        manager.UpdateState(state);
      }
    });
  }

  for (std::size_t reader_index = 0; reader_index < kReaderCount; ++reader_index) {
    (void)reader_index;
    workers.emplace_back([&manager, &start_gate, &read_count, &connected_observations]() {
      start_gate.wait();

      for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
        const RobotState snapshot = manager.GetState();
        if (snapshot.connection.connected) {
          connected_observations.fetch_add(1U, std::memory_order_relaxed);
        }
        read_count.fetch_add(1U, std::memory_order_relaxed);
      }
    });
  }

  start_gate.count_down();
  workers.clear();

  if (read_count.load(std::memory_order_relaxed) != kReaderCount * kIterations) {
    return Fail(kTestName, "concurrent readers did not complete all reads");
  }

  if (connected_observations.load(std::memory_order_relaxed) == 0U) {
    return Fail(kTestName, "concurrent readers did not observe updated state");
  }

  RobotState final_state;
  final_state.connection.connected = true;
  final_state.power.batteryLevel = 100.0F;
  final_state.health.faultCode = 0;
  manager.UpdateState(final_state);

  if (!manager.IsConnected() || manager.BatteryLevel() != 100.0F ||
      manager.FaultCode() != 0) {
    return Fail(kTestName, "manager state was inconsistent after concurrent access");
  }

  return true;
}

[[nodiscard]] bool TestRobotStateManagerPerformanceSanity() {
  constexpr std::string_view kTestName{"RobotStateManager performance"};
  constexpr std::size_t kIterations{50000};

  RobotStateManager manager;
  RobotState state;

  const auto start_time = std::chrono::steady_clock::now();
  for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
    state.connection.connected = (iteration % 2U) == 0U;
    state.power.batteryLevel = static_cast<float>(iteration % 101U);
    manager.UpdateState(state);
    const RobotState snapshot = manager.GetState();
    if (snapshot.power.batteryLevel < 0.0F || snapshot.power.batteryLevel > 100.0F) {
      return Fail(kTestName, "snapshot battery value outside expected test range");
    }
  }

  const auto elapsed = std::chrono::steady_clock::now() - start_time;
  if (elapsed > std::chrono::seconds{5}) {
    return Fail(kTestName, "state update/read loop exceeded sanity threshold");
  }

  return true;
}

[[nodiscard]] bool TestTelemetrySubscriberCallback() {
  constexpr std::string_view kTestName{"TelemetryService subscriber callback"};

  auto manager = std::make_shared<RobotStateManager>();
  RobotState state;
  state.connection.connected = true;
  state.power.batteryLevel = 71.0F;
  state.motion.walking = true;
  manager->UpdateState(state);

  TelemetryService telemetry{manager, std::chrono::milliseconds{5}};
  std::mutex callback_mutex;
  std::condition_variable callback_condition;
  std::size_t first_count{0};
  std::size_t second_count{0};
  RobotState observed_state;

  const TelemetryService::SubscriptionId first_subscription =
      telemetry.Subscribe([&](const RobotState& published_state) {
        std::lock_guard<std::mutex> lock{callback_mutex};
        observed_state = published_state;
        ++first_count;
        callback_condition.notify_all();
      });
  const TelemetryService::SubscriptionId second_subscription =
      telemetry.Subscribe([&](const RobotState& published_state) {
        std::lock_guard<std::mutex> lock{callback_mutex};
        observed_state = published_state;
        ++second_count;
        callback_condition.notify_all();
      });

  if (first_subscription == TelemetryService::kInvalidSubscriptionId ||
      second_subscription == TelemetryService::kInvalidSubscriptionId) {
    return Fail(kTestName, "subscription registration failed");
  }

  if (!telemetry.Start()) {
    return Fail(kTestName, "telemetry service failed to start");
  }

  {
    std::unique_lock<std::mutex> lock{callback_mutex};
    const bool received = callback_condition.wait_for(lock, std::chrono::milliseconds{250}, [&]() {
      return first_count > 0U && second_count > 0U;
    });
    if (!received) {
      telemetry.Stop();
      return Fail(kTestName, "subscriber callbacks were not invoked");
    }
  }

  telemetry.Stop();

  if (!observed_state.connection.connected || observed_state.power.batteryLevel != 71.0F ||
      !observed_state.motion.walking) {
    return Fail(kTestName, "subscriber observed an unexpected state snapshot");
  }

  if (!telemetry.Unsubscribe(first_subscription) ||
      !telemetry.Unsubscribe(second_subscription)) {
    return Fail(kTestName, "unsubscribe failed for active subscriptions");
  }

  return true;
}

[[nodiscard]] bool TestTelemetryUnsubscribe() {
  constexpr std::string_view kTestName{"TelemetryService unsubscribe"};

  auto manager = std::make_shared<RobotStateManager>();
  TelemetryService telemetry{manager, std::chrono::milliseconds{5}};
  std::atomic<std::size_t> callback_count{0};

  const TelemetryService::SubscriptionId subscription =
      telemetry.Subscribe([&callback_count](const RobotState&) {
        callback_count.fetch_add(1U, std::memory_order_relaxed);
      });
  if (subscription == TelemetryService::kInvalidSubscriptionId) {
    return Fail(kTestName, "subscription registration failed");
  }

  if (!telemetry.Unsubscribe(subscription)) {
    return Fail(kTestName, "unsubscribe returned false for active subscription");
  }

  if (!telemetry.Start()) {
    return Fail(kTestName, "telemetry service failed to start");
  }

  std::this_thread::sleep_for(std::chrono::milliseconds{25});
  telemetry.Stop();

  if (callback_count.load(std::memory_order_relaxed) != 0U) {
    return Fail(kTestName, "unsubscribed callback was invoked");
  }

  return true;
}

[[nodiscard]] bool TestTelemetryStartValidation() {
  constexpr std::string_view kTestName{"TelemetryService start validation"};

  TelemetryService telemetry{nullptr, std::chrono::milliseconds{1}};
  if (telemetry.Start()) {
    telemetry.Stop();
    return Fail(kTestName, "telemetry service started without a state manager");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestRobotStateDefaults,
      TestRobotStateManagerUpdateAndReset,
      TestRobotStateManagerThreadSafety,
      TestRobotStateManagerPerformanceSanity,
      TestTelemetrySubscriberCallback,
      TestTelemetryUnsubscribe,
      TestTelemetryStartValidation,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
