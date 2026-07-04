#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#include <humanoid/core/RobotState.hpp>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/services/TelemetryService.h>

int main() {
  try {
    auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();

    humanoid::core::RobotState state;
    state.connection.connected = true;
    state.power.batteryLevel = 82.5F;
    state.motion.standing = true;
    state.timestamp =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
    state_manager->UpdateState(state);

    std::atomic<int> samples{0};
    std::atomic<float> latest_battery{0.0F};
    humanoid::services::TelemetryService telemetry{state_manager, std::chrono::milliseconds{10}};
    const humanoid::services::TelemetryService::SubscriptionId subscription =
        telemetry.Subscribe([&samples, &latest_battery](const humanoid::core::RobotState& sample) {
          latest_battery.store(sample.power.batteryLevel, std::memory_order_relaxed);
          samples.fetch_add(1, std::memory_order_relaxed);
        });

    if (subscription == humanoid::services::TelemetryService::kInvalidSubscriptionId) {
      throw std::runtime_error("failed to subscribe telemetry listener");
    }

    if (!telemetry.Start()) {
      throw std::runtime_error("failed to start telemetry service");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{35});
    telemetry.Stop();
    static_cast<void>(telemetry.Unsubscribe(subscription));

    std::cout << "Telemetry example published " << samples.load(std::memory_order_relaxed)
              << " sample(s); latest_battery=" << latest_battery.load(std::memory_order_relaxed)
              << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
