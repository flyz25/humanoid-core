#pragma once

/**
 * @file SensorFactory.h
 * @brief Defines dependency-injected sensor creation utilities.
 */

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/Sensor.h>
#include <humanoid/perception/SensorCapabilities.h>

namespace humanoid::perception {

/**
 * @brief Callable that creates one sensor instance.
 */
using SensorCreator = std::function<std::unique_ptr<Sensor>()>;

/**
 * @brief Result returned by `SensorFactory::CreateSensor()`.
 */
struct SensorCreationResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Created sensor instance on success. */
  std::unique_ptr<Sensor> sensor;

  /** @brief Constructs an empty successful result. */
  SensorCreationResult() = default;

  /**
   * @brief Constructs a sensor creation result.
   *
   * @param result_status Operation status.
   * @param result_sensor Created sensor.
   */
  SensorCreationResult(humanoid::common::Status result_status,
                       std::unique_ptr<Sensor> result_sensor = {})
      : status(std::move(result_status)), sensor(std::move(result_sensor)) {}
};

/**
 * @brief Thread-safe registry of sensor creator callbacks.
 *
 * `SensorFactory` owns creator callbacks and capability metadata only. It does
 * not perform dynamic loading, hardware discovery, SDK initialization,
 * middleware communication, or sensor polling. Applications may instantiate and
 * inject as many factories as needed.
 */
class SensorFactory final {
public:
  /** @brief Constructs an empty sensor factory. */
  SensorFactory() = default;

  /** @brief Destroys registered creator callbacks. */
  ~SensorFactory() = default;

  SensorFactory(const SensorFactory&) = delete;
  SensorFactory& operator=(const SensorFactory&) = delete;
  SensorFactory(SensorFactory&&) = delete;
  SensorFactory& operator=(SensorFactory&&) = delete;

  /**
   * @brief Registers a sensor creator.
   *
   * @param capabilities Capability declaration for the sensor.
   * @param creator Creator callback.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterSensor(SensorCapabilities capabilities,
                                                        SensorCreator creator) {
    if (!capabilities.isValid()) {
      return humanoid::common::Status::error(
          humanoid::common::StatusCode::kInvalidArgument,
          "sensor capabilities require non-empty sensor id and name");
    }
    if (!creator) {
      return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                             "sensor creator is empty");
    }

    std::string sensor_id = capabilities.sensorId;
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto [iterator, inserted] =
        entries_.emplace(std::move(sensor_id), Entry{std::move(capabilities), std::move(creator)});
    static_cast<void>(iterator);
    if (!inserted) {
      return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                             "sensor id is already registered");
    }
    return humanoid::common::Status::ok();
  }

  /**
   * @brief Removes a registered sensor creator.
   *
   * @param sensor_id Sensor id to unregister.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterSensor(std::string_view sensor_id) {
    if (sensor_id.empty()) {
      return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                             "sensor id is empty");
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    if (entries_.erase(std::string{sensor_id}) == 0U) {
      return humanoid::common::Status::error(humanoid::common::StatusCode::kUnavailable,
                                             "sensor id is not registered");
    }
    return humanoid::common::Status::ok();
  }

  /**
   * @brief Creates a sensor instance by registered id.
   *
   * Creator exceptions are contained and reported as status failures.
   *
   * @param sensor_id Registered sensor id.
   * @return Creation result.
   */
  [[nodiscard]] SensorCreationResult CreateSensor(std::string_view sensor_id) const {
    if (sensor_id.empty()) {
      return SensorCreationResult{humanoid::common::Status::error(
          humanoid::common::StatusCode::kInvalidArgument, "sensor id is empty")};
    }

    SensorCreator creator;
    {
      std::shared_lock<std::shared_mutex> lock{mutex_};
      const auto iterator = entries_.find(std::string{sensor_id});
      if (iterator == entries_.end()) {
        return SensorCreationResult{humanoid::common::Status::error(
            humanoid::common::StatusCode::kUnavailable, "sensor id is not registered")};
      }
      creator = iterator->second.creator;
    }

    try {
      std::unique_ptr<Sensor> sensor = creator();
      if (!sensor) {
        return SensorCreationResult{humanoid::common::Status::error(
            humanoid::common::StatusCode::kInternalError, "sensor creator returned null")};
      }
      return SensorCreationResult{humanoid::common::Status::ok(), std::move(sensor)};
    } catch (...) {
      return SensorCreationResult{humanoid::common::Status::error(
          humanoid::common::StatusCode::kInternalError, "sensor creator threw an exception")};
    }
  }

  /**
   * @brief Enumerates registered sensor capabilities.
   *
   * @return Capability declarations ordered by sensor id.
   */
  [[nodiscard]] std::vector<SensorCapabilities> EnumerateSensors() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    std::vector<SensorCapabilities> capabilities;
    capabilities.reserve(entries_.size());
    for (const auto& [sensor_id, entry] : entries_) {
      static_cast<void>(sensor_id);
      capabilities.push_back(entry.capabilities);
    }
    return capabilities;
  }

  /**
   * @brief Reports whether a sensor id is registered.
   *
   * @param sensor_id Sensor id to inspect.
   * @return True when a creator exists.
   */
  [[nodiscard]] bool Contains(std::string_view sensor_id) const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    return entries_.find(std::string{sensor_id}) != entries_.end();
  }

  /**
   * @brief Returns the number of registered sensor creators.
   *
   * @return Registered sensor count.
   */
  [[nodiscard]] std::size_t Size() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    return entries_.size();
  }

private:
  struct Entry final {
    SensorCapabilities capabilities;
    SensorCreator creator;
  };

  mutable std::shared_mutex mutex_;
  std::map<std::string, Entry, std::less<>> entries_;
};

} // namespace humanoid::perception
