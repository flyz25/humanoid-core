#pragma once

/**
 * @file SensorCapabilities.h
 * @brief Defines generic sensor capability and configuration metadata.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <humanoid/perception/SensorType.h>

namespace humanoid::perception {

/**
 * @brief Supported scalar value types for sensor configuration.
 */
using SensorConfigurationValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered sensor configuration map.
 *
 * Configuration contains framework-owned scalar values only. It must not store
 * SDK clients, middleware messages, pointers, file descriptors, or vendor
 * handles.
 */
using SensorConfiguration = std::map<std::string, SensorConfigurationValue, std::less<>>;

/**
 * @brief Sensor capability declaration.
 */
struct SensorCapabilities final {
  /** @brief Stable sensor identifier within the owning adapter or process. */
  std::string sensorId;

  /** @brief Human-readable sensor name. */
  std::string name;

  /** @brief Sensor category. */
  SensorType type{SensorType::Custom};

  /** @brief Supported encoding labels in preference order. */
  std::vector<std::string> supportedEncodings;

  /** @brief Maximum image-like frame width; zero when not applicable. */
  std::uint32_t maximumWidth{0U};

  /** @brief Maximum image-like frame height; zero when not applicable. */
  std::uint32_t maximumHeight{0U};

  /** @brief Maximum sample rate; zero means unspecified or event-driven. */
  double maximumFrameRateHz{0.0};

  /** @brief True when the sensor can provide hardware timestamps. */
  bool supportsHardwareTimestamp{false};

  /** @brief True when the sensor supports explicit start and stop lifecycle. */
  bool supportsStartStop{true};

  /** @brief True when `ReadFrame()` may be called repeatedly while running. */
  bool supportsStreaming{true};

  /** @brief True when `Configuration()` returns meaningful configuration. */
  bool supportsConfiguration{false};

  /** @brief Expected maximum blocking duration for one frame read. */
  std::chrono::milliseconds nominalReadTimeout{0};

  /** @brief Constructs an empty custom capability declaration. */
  SensorCapabilities() = default;

  /**
   * @brief Reports whether the declaration contains a usable identity.
   *
   * @return True when `sensorId` and `name` are not empty.
   */
  [[nodiscard]] bool isValid() const noexcept { return !sensorId.empty() && !name.empty(); }
};

} // namespace humanoid::perception
