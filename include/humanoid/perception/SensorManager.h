#pragma once

/**
 * @file SensorManager.h
 * @brief Defines the thread-safe generic sensor manager.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/Sensor.h>
#include <humanoid/perception/SensorCapabilities.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorState.h>

namespace humanoid::perception {

/**
 * @brief Stable identifier for a frame listener subscription.
 */
using SensorFrameSubscriptionId = std::uint64_t;

/**
 * @brief Callback invoked when the manager routes a successfully read frame.
 */
using SensorFrameListener = std::function<void(const std::string&, const SensorFrame&)>;

/**
 * @brief Result returned when registering a frame listener.
 */
struct SensorFrameSubscriptionResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Nonzero subscription id on success. */
  SensorFrameSubscriptionId subscriptionId{0U};

  /** @brief Constructs an empty successful result. */
  SensorFrameSubscriptionResult() = default;

  /**
   * @brief Constructs a subscription result.
   *
   * @param result_status Operation status.
   * @param result_subscription_id Subscription id.
   */
  SensorFrameSubscriptionResult(humanoid::common::Status result_status,
                                SensorFrameSubscriptionId result_subscription_id = 0U)
      : status(std::move(result_status)), subscriptionId(result_subscription_id) {}

  /**
   * @brief Reports whether subscription succeeded.
   *
   * @return True when status is OK and the subscription id is nonzero.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk() && subscriptionId != 0U; }
};

/**
 * @brief Result returned by manager frame reads.
 */
struct SensorManagerFrameResult final {
  /** @brief Operation status for the read attempt. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Sensor id associated with the read. */
  std::string sensorId;

  /** @brief Frame value populated when status is successful. */
  SensorFrame frame;

  /** @brief Constructs a successful empty result. */
  SensorManagerFrameResult() = default;

  /**
   * @brief Constructs a frame read result.
   *
   * @param result_status Operation status.
   * @param result_sensor_id Sensor id.
   * @param result_frame Frame value.
   */
  SensorManagerFrameResult(humanoid::common::Status result_status, std::string result_sensor_id,
                           SensorFrame result_frame = {})
      : status(std::move(result_status)), sensorId(std::move(result_sensor_id)),
        frame(std::move(result_frame)) {}

  /**
   * @brief Reports whether the read succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Result returned when querying one sensor health snapshot.
 */
struct SensorHealthResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Sensor id associated with the health snapshot. */
  std::string sensorId;

  /** @brief Health snapshot. */
  SensorHealth health;

  /** @brief Constructs a successful empty result. */
  SensorHealthResult() = default;

  /**
   * @brief Constructs a health result.
   *
   * @param result_status Operation status.
   * @param result_sensor_id Sensor id.
   * @param result_health Health snapshot.
   */
  SensorHealthResult(humanoid::common::Status result_status, std::string result_sensor_id,
                     SensorHealth result_health = {})
      : status(std::move(result_status)), sensorId(std::move(result_sensor_id)),
        health(std::move(result_health)) {}

  /**
   * @brief Reports whether the health query succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Result returned when querying one sensor state snapshot.
 */
struct SensorStateResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Sensor id associated with the state snapshot. */
  std::string sensorId;

  /** @brief Sensor lifecycle and health state. */
  SensorState state;

  /** @brief Constructs a successful empty result. */
  SensorStateResult() = default;

  /**
   * @brief Constructs a state result.
   *
   * @param result_status Operation status.
   * @param result_sensor_id Sensor id.
   * @param result_state State snapshot.
   */
  SensorStateResult(humanoid::common::Status result_status, std::string result_sensor_id,
                    SensorState result_state = {})
      : status(std::move(result_status)), sensorId(std::move(result_sensor_id)),
        state(std::move(result_state)) {}

  /**
   * @brief Reports whether the state query succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Thread-safe snapshot of sensor manager activity.
 */
struct SensorManagerStatistics final {
  /** @brief Number of currently registered sensors. */
  std::size_t registeredSensors{0U};

  /** @brief Number of registered frame listeners. */
  std::size_t frameListeners{0U};

  /** @brief Sensors registered over the manager lifetime. */
  std::uint64_t registrations{0U};

  /** @brief Sensors unregistered over the manager lifetime. */
  std::uint64_t unregistrations{0U};

  /** @brief Successful frame reads routed by the manager. */
  std::uint64_t routedFrames{0U};

  /** @brief Failed frame read attempts observed by the manager. */
  std::uint64_t readFailures{0U};

  /** @brief Listener callbacks invoked for routed frames. */
  std::uint64_t listenerCallbacks{0U};

  /** @brief Listener callbacks that threw and were contained. */
  std::uint64_t listenerFailures{0U};
};

/**
 * @brief Thread-safe owner and router for active sensor instances.
 *
 * `SensorManager` owns generic `Sensor` implementations, provides active
 * sensor discovery, serializes lifecycle and read calls per sensor, timestamps
 * frames that arrive without a capture timestamp, stores latest frame/state
 * snapshots, and routes successful frames to subscribed listeners.
 *
 * The manager contains no driver, SDK, middleware, robot, mission, behavior
 * tree, planning, or vendor-specific logic. It is non-singleton and intended
 * for dependency injection.
 */
class SensorManager final {
public:
  /** @brief Constructs an empty sensor manager. */
  SensorManager();

  /** @brief Stops and releases all registered sensors. */
  ~SensorManager() noexcept;

  SensorManager(const SensorManager&) = delete;
  SensorManager& operator=(const SensorManager&) = delete;
  SensorManager(SensorManager&&) = delete;
  SensorManager& operator=(SensorManager&&) = delete;

  /**
   * @brief Registers an active sensor instance.
   *
   * The sensor id is taken from `Sensor::Capabilities()`. Duplicate ids are
   * rejected. The manager assumes ownership only when registration succeeds.
   *
   * @param sensor Sensor instance to register.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterSensor(std::unique_ptr<Sensor> sensor);

  /**
   * @brief Unregisters a sensor and releases it after cooperative shutdown.
   *
   * @param sensor_id Registered sensor id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterSensor(std::string_view sensor_id);

  /**
   * @brief Initializes a registered sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status InitializeSensor(std::string_view sensor_id);

  /**
   * @brief Starts a registered sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status StartSensor(std::string_view sensor_id);

  /**
   * @brief Stops a registered sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status StopSensor(std::string_view sensor_id);

  /**
   * @brief Shuts down a registered sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status ShutdownSensor(std::string_view sensor_id);

  /**
   * @brief Stops, shuts down, and unregisters all sensors.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Shutdown();

  /**
   * @brief Discovers currently registered active sensors.
   *
   * @return Capability declarations ordered by sensor id.
   */
  [[nodiscard]] std::vector<SensorCapabilities> DiscoverSensors() const;

  /**
   * @brief Returns whether a sensor id is currently registered.
   *
   * @param sensor_id Sensor id to inspect.
   * @return True when a sensor with the id is registered.
   */
  [[nodiscard]] bool Contains(std::string_view sensor_id) const;

  /**
   * @brief Returns the number of currently registered sensors.
   *
   * @return Sensor count.
   */
  [[nodiscard]] std::size_t Size() const;

  /**
   * @brief Reads one frame from a registered sensor and routes it to listeners.
   *
   * @param sensor_id Registered sensor id.
   * @return Frame read result.
   */
  [[nodiscard]] SensorManagerFrameResult ReadFrame(std::string_view sensor_id);

  /**
   * @brief Reads one frame from every registered sensor.
   *
   * Each sensor read is serialized per sensor but different callers may read
   * different sensors concurrently.
   *
   * @return Frame results ordered by sensor id.
   */
  [[nodiscard]] std::vector<SensorManagerFrameResult> ReadAllFrames();

  /**
   * @brief Returns the latest successfully routed frame for one sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return Latest frame result or unavailable status.
   */
  [[nodiscard]] SensorManagerFrameResult LatestFrame(std::string_view sensor_id) const;

  /**
   * @brief Returns the current health snapshot for one sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return Health result.
   */
  [[nodiscard]] SensorHealthResult Health(std::string_view sensor_id) const;

  /**
   * @brief Returns health snapshots for all registered sensors.
   *
   * @return Health results ordered by sensor id.
   */
  [[nodiscard]] std::vector<SensorHealthResult> HealthReport() const;

  /**
   * @brief Returns the manager-maintained state snapshot for one sensor.
   *
   * @param sensor_id Registered sensor id.
   * @return State result.
   */
  [[nodiscard]] SensorStateResult State(std::string_view sensor_id) const;

  /**
   * @brief Subscribes to routed frames.
   *
   * Listener callbacks are invoked outside manager and sensor locks.
   * Exceptions from listeners are contained and counted.
   *
   * @param listener Listener callback.
   * @return Subscription result.
   */
  [[nodiscard]] SensorFrameSubscriptionResult Subscribe(SensorFrameListener listener);

  /**
   * @brief Removes a frame listener subscription.
   *
   * @param subscription_id Subscription id returned by `Subscribe()`.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Unsubscribe(SensorFrameSubscriptionId subscription_id);

  /**
   * @brief Returns a consistent manager statistics snapshot.
   *
   * @return Sensor manager statistics.
   */
  [[nodiscard]] SensorManagerStatistics GetStatistics() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::perception
