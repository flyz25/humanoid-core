#pragma once

/**
 * @file TelemetryService.h
 * @brief Defines a thread-safe robot state telemetry publisher.
 */

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>

#include <humanoid/core/RobotState.hpp>
#include <humanoid/core/RobotStateManager.hpp>

namespace humanoid::services {

/**
 * @brief Periodically publishes snapshots from a RobotStateManager.
 *
 * TelemetryService owns a background `std::jthread` while running. The worker
 * sleeps on a condition variable between samples and wakes promptly when
 * `Stop()` is requested. Listener registration is synchronized separately from
 * service lifecycle state.
 *
 * Listener callbacks are invoked outside internal locks. A callback may receive
 * one final copied snapshot after `Unsubscribe()` if publication was already in
 * progress when unsubscription occurred.
 */
class TelemetryService final {
public:
  /**
   * @brief Listener callback invoked with each published robot state snapshot.
   */
  using Listener = std::function<void(const core::RobotState&)>;

  /**
   * @brief Stable identifier returned by Subscribe().
   */
  using SubscriptionId = std::uint64_t;

  /**
   * @brief Sentinel value used when subscription registration fails.
   */
  static constexpr SubscriptionId kInvalidSubscriptionId{0};

  /**
   * @brief Default telemetry polling interval.
   */
  static constexpr std::chrono::milliseconds kDefaultPollingInterval{100};

  /**
   * @brief Constructs a telemetry service.
   *
   * @param state_manager Shared robot state source. Start() fails when null.
   * @param polling_interval Desired publish interval. Non-positive values use
   * the default interval.
   */
  explicit TelemetryService(
      std::shared_ptr<const core::RobotStateManager> state_manager,
      std::chrono::milliseconds polling_interval = kDefaultPollingInterval) noexcept;

  /**
   * @brief Stops the worker thread before destruction.
   */
  ~TelemetryService();

  TelemetryService(const TelemetryService&) = delete;
  TelemetryService& operator=(const TelemetryService&) = delete;
  TelemetryService(TelemetryService&&) = delete;
  TelemetryService& operator=(TelemetryService&&) = delete;

  /**
   * @brief Starts periodic state publication.
   *
   * @return True when the worker thread was started by this call.
   */
  [[nodiscard]] bool Start();

  /**
   * @brief Stops periodic state publication.
   */
  void Stop();

  /**
   * @brief Subscribes a listener for state snapshots.
   *
   * @param listener Callback to invoke for each state snapshot.
   * @return Subscription identifier, or kInvalidSubscriptionId on failure.
   */
  [[nodiscard]] SubscriptionId Subscribe(Listener listener);

  /**
   * @brief Removes a previously subscribed listener.
   *
   * @param subscription_id Identifier returned by Subscribe().
   * @return True when a listener was removed.
   */
  [[nodiscard]] bool Unsubscribe(SubscriptionId subscription_id);

private:
  /**
   * @brief Runs the telemetry publication loop.
   *
   * @param stop_token Cooperative stop token owned by the worker thread.
   */
  void Run(std::stop_token stop_token);

  /**
   * @brief Publishes the latest state to a copied listener list.
   */
  void PublishCurrentState() const;

  /**
   * @brief Allocates a subscription identifier while listeners_mutex_ is held.
   *
   * @return Subscription identifier, or kInvalidSubscriptionId when exhausted.
   */
  [[nodiscard]] SubscriptionId AllocateSubscriptionIdLocked();

  /**
   * @brief Normalizes a polling interval.
   *
   * @param polling_interval Candidate interval.
   * @return Candidate interval when positive, otherwise kDefaultPollingInterval.
   */
  [[nodiscard]] static constexpr std::chrono::milliseconds
  NormalizePollingInterval(std::chrono::milliseconds polling_interval) noexcept {
    return polling_interval.count() > 0 ? polling_interval : kDefaultPollingInterval;
  }

  std::shared_ptr<const core::RobotStateManager> state_manager_;
  std::chrono::milliseconds polling_interval_;

  mutable std::shared_mutex listeners_mutex_;
  std::unordered_map<SubscriptionId, Listener> listeners_;
  SubscriptionId next_subscription_id_{1};

  std::mutex lifecycle_mutex_;
  std::condition_variable lifecycle_condition_;
  bool stop_requested_{false};
  std::jthread worker_;
};

} // namespace humanoid::services
