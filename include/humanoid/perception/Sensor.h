#pragma once

/**
 * @file Sensor.h
 * @brief Defines the vendor-independent sensor interface.
 */

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/SensorCapabilities.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorState.h>

namespace humanoid::perception {

/**
 * @brief Pure abstract generic sensor boundary.
 *
 * `Sensor` exposes lifecycle, frame capture, capabilities, health, and
 * configuration without binding applications or core framework code to OpenCV,
 * PCL, ROS2, DDS, Unitree SDK, camera SDKs, LiDAR SDKs, audio APIs, or vendor
 * transport types.
 */
class Sensor {
public:
  /** @brief Destroys the sensor interface. */
  virtual ~Sensor() = default;

  /**
   * @brief Initializes local sensor resources without starting capture.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Initialize() = 0;

  /**
   * @brief Releases local sensor resources.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Shutdown() = 0;

  /**
   * @brief Starts capture, sampling, or stream acquisition.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Start() = 0;

  /**
   * @brief Stops capture, sampling, or stream acquisition.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Stop() = 0;

  /**
   * @brief Reads one generic frame from the sensor.
   *
   * Implementations must translate backend errors into `SensorFrameResult` and
   * must not allow vendor SDK exceptions to cross this boundary.
   *
   * @return Frame read result.
   */
  [[nodiscard]] virtual SensorFrameResult ReadFrame() = 0;

  /**
   * @brief Returns the static or negotiated sensor capabilities.
   *
   * @return Sensor capability declaration.
   */
  [[nodiscard]] virtual SensorCapabilities Capabilities() const = 0;

  /**
   * @brief Returns the current health snapshot.
   *
   * @return Sensor health.
   */
  [[nodiscard]] virtual SensorHealth Health() const = 0;

  /**
   * @brief Returns the current generic configuration snapshot.
   *
   * @return Sensor configuration values.
   */
  [[nodiscard]] virtual SensorConfiguration Configuration() const = 0;
};

} // namespace humanoid::perception
