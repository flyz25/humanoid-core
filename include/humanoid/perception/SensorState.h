#pragma once

/**
 * @file SensorState.h
 * @brief Defines generic sensor lifecycle and health state.
 */

#include <chrono>
#include <cstdint>
#include <string>

namespace humanoid::perception {

/**
 * @brief Lifecycle state reported by a sensor implementation.
 */
enum class SensorLifecycleState : std::uint8_t {
  Uninitialized, ///< Sensor has not allocated local resources.
  Initialized,   ///< Sensor resources are ready but capture is stopped.
  Running,       ///< Sensor capture or sampling is active.
  Stopped,       ///< Sensor capture or sampling has been stopped.
  Shutdown,      ///< Sensor resources have been released.
  Faulted        ///< Sensor detected a fault requiring external recovery.
};

/**
 * @brief Health classification reported by a sensor implementation.
 */
enum class SensorHealthStatus : std::uint8_t {
  Unknown,    ///< Health has not been reported.
  Healthy,    ///< Sensor is operating within expected limits.
  Degraded,   ///< Sensor is usable with reduced quality or reliability.
  Faulted,    ///< Sensor is not usable because of a fault.
  Unavailable ///< Sensor is absent or communication is unavailable.
};

/**
 * @brief Sensor health snapshot.
 */
struct SensorHealth final {
  /** @brief Current health classification. */
  SensorHealthStatus status{SensorHealthStatus::Unknown};

  /** @brief Vendor-independent diagnostic code; empty when no code is available. */
  std::string diagnosticCode;

  /** @brief Human-readable health message. */
  std::string message;

  /** @brief Monotonic time at which this health snapshot was produced. */
  std::chrono::steady_clock::time_point timestamp{};

  /** @brief Constructs an empty unknown-health snapshot. */
  SensorHealth() = default;

  /**
   * @brief Reports whether the sensor can be treated as usable.
   *
   * @return True when health is `Healthy` or `Degraded`.
   */
  [[nodiscard]] constexpr bool isUsable() const noexcept {
    return status == SensorHealthStatus::Healthy || status == SensorHealthStatus::Degraded;
  }
};

/**
 * @brief Sensor lifecycle snapshot.
 */
struct SensorState final {
  /** @brief Current lifecycle state. */
  SensorLifecycleState lifecycle{SensorLifecycleState::Uninitialized};

  /** @brief Current health state. */
  SensorHealth health;

  /** @brief Number of frames successfully produced by the sensor. */
  std::uint64_t framesProduced{0U};

  /** @brief Number of frame-read failures observed by the implementation. */
  std::uint64_t frameErrors{0U};

  /** @brief Monotonic time at which this state snapshot was produced. */
  std::chrono::steady_clock::time_point timestamp{};

  /** @brief Constructs a default uninitialized sensor state. */
  SensorState() = default;

  /**
   * @brief Reports whether the sensor is actively producing frames.
   *
   * @return True when lifecycle is `Running`.
   */
  [[nodiscard]] constexpr bool isRunning() const noexcept {
    return lifecycle == SensorLifecycleState::Running;
  }
};

} // namespace humanoid::perception
