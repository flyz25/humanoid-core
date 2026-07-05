#pragma once

/**
 * @file SensorType.h
 * @brief Defines vendor-independent sensor categories.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::perception {

/**
 * @brief Identifies a generic sensor class supported by the perception boundary.
 *
 * Sensor type values are framework-owned metadata. They must not encode vendor
 * SDK enums, transport handles, middleware message types, or implementation
 * class names.
 */
enum class SensorType : std::uint8_t {
  Camera,      ///< Monocular or color camera.
  DepthCamera, ///< Depth-producing camera or RGB-D sensor.
  Lidar,       ///< Laser scanner or LiDAR sensor.
  Microphone,  ///< Audio capture sensor.
  Imu,         ///< Inertial measurement unit.
  Radar,       ///< Radar sensor.
  Custom       ///< Application-defined sensor category.
};

/**
 * @brief Returns a stable diagnostic string for a sensor type.
 *
 * @param type Sensor type to convert.
 * @return Non-owning string representation.
 */
[[nodiscard]] constexpr std::string_view toString(SensorType type) noexcept {
  switch (type) {
  case SensorType::Camera:
    return "Camera";
  case SensorType::DepthCamera:
    return "DepthCamera";
  case SensorType::Lidar:
    return "Lidar";
  case SensorType::Microphone:
    return "Microphone";
  case SensorType::Imu:
    return "Imu";
  case SensorType::Radar:
    return "Radar";
  case SensorType::Custom:
    return "Custom";
  }

  return "Unknown";
}

} // namespace humanoid::perception
