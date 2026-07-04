#pragma once

/**
 * @file RobotState.hpp
 * @brief Defines the vendor-independent robot state snapshot model.
 */

#include <chrono>
#include <cstdint>
#include <type_traits>

namespace humanoid::core {

/**
 * @brief Monotonic timestamp type used for robot state samples.
 *
 * The timestamp represents the local monotonic time at which a state sample was
 * observed or produced. It is intentionally not tied to wall-clock time so that
 * control and diagnostics code are not affected by system clock adjustments.
 */
using RobotStateTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Vendor-independent connection status.
 */
struct RobotConnectionState final {
  /**
   * @brief True when communication with the robot is established.
   */
  bool connected{false};

  /**
   * @brief Constructs a disconnected connection state.
   */
  constexpr RobotConnectionState() noexcept = default;
};

/**
 * @brief Vendor-independent robot power state.
 */
struct RobotPowerState final {
  /**
   * @brief Battery charge level in percent, in the range [0.0, 100.0].
   */
  float batteryLevel{0.0F};

  /**
   * @brief True when the robot reports an active charging state.
   */
  bool charging{false};

  /**
   * @brief Constructs an empty power state.
   */
  constexpr RobotPowerState() noexcept = default;
};

/**
 * @brief Vendor-independent high-level motion state.
 */
struct RobotMotionState final {
  /**
   * @brief True when the robot is in an upright standing posture.
   */
  bool standing{false};

  /**
   * @brief True when the robot is executing or reporting walking motion.
   */
  bool walking{false};

  /**
   * @brief True when the robot is in a seated or sitting posture.
   */
  bool sitting{false};

  /**
   * @brief Constructs an inactive motion state.
   */
  constexpr RobotMotionState() noexcept = default;
};

/**
 * @brief Vendor-independent base velocity state.
 */
struct RobotVelocity final {
  /**
   * @brief Linear velocity along the robot forward X axis, in meters per second.
   */
  float linearX{0.0F};

  /**
   * @brief Linear velocity along the robot lateral Y axis, in meters per second.
   */
  float linearY{0.0F};

  /**
   * @brief Angular velocity around the robot vertical Z axis, in radians per second.
   */
  float angularZ{0.0F};

  /**
   * @brief Constructs a zero velocity state.
   */
  constexpr RobotVelocity() noexcept = default;
};

/**
 * @brief Vendor-independent position state.
 */
struct RobotPose final {
  /**
   * @brief Position along the X axis in the active robot-local or world frame, in meters.
   */
  float positionX{0.0F};

  /**
   * @brief Position along the Y axis in the active robot-local or world frame, in meters.
   */
  float positionY{0.0F};

  /**
   * @brief Position along the Z axis in the active robot-local or world frame, in meters.
   */
  float positionZ{0.0F};

  /**
   * @brief Constructs a zero position state.
   */
  constexpr RobotPose() noexcept = default;
};

/**
 * @brief Vendor-independent orientation state.
 */
struct RobotOrientation final {
  /**
   * @brief Roll angle around the X axis, in radians.
   */
  float roll{0.0F};

  /**
   * @brief Pitch angle around the Y axis, in radians.
   */
  float pitch{0.0F};

  /**
   * @brief Yaw angle around the Z axis, in radians.
   */
  float yaw{0.0F};

  /**
   * @brief Constructs a zero orientation state.
   */
  constexpr RobotOrientation() noexcept = default;
};

/**
 * @brief Vendor-independent robot health state.
 */
struct RobotHealthState final {
  /**
   * @brief True when an emergency stop condition is active.
   */
  bool emergencyStop{false};

  /**
   * @brief Vendor-normalized fault code; zero means no reported fault.
   */
  std::int32_t faultCode{0};

  /**
   * @brief Constructs a healthy robot state.
   */
  constexpr RobotHealthState() noexcept = default;
};

/**
 * @brief Vendor-independent robot state snapshot.
 *
 * RobotState is a value type intended for adapter output, manager state caches,
 * diagnostics, and later serialization boundaries. It contains no vendor SDK
 * types, performs no dynamic allocation, and has deterministic default values.
 */
struct RobotState final {
  /**
   * @brief Connection state reported by the robot communication layer.
   */
  RobotConnectionState connection{};

  /**
   * @brief Power and battery state.
   */
  RobotPowerState power{};

  /**
   * @brief High-level posture and walking state.
   */
  RobotMotionState motion{};

  /**
   * @brief Base velocity state.
   */
  RobotVelocity velocity{};

  /**
   * @brief Position state.
   */
  RobotPose pose{};

  /**
   * @brief Orientation state.
   */
  RobotOrientation orientation{};

  /**
   * @brief Health, emergency-stop, and fault state.
   */
  RobotHealthState health{};

  /**
   * @brief Monotonic timestamp for this state sample.
   */
  RobotStateTimestamp timestamp{};

  /**
   * @brief Constructs an empty robot state sample.
   */
  constexpr RobotState() noexcept = default;
};

static_assert(std::is_standard_layout_v<RobotState>,
              "RobotState must remain a standard-layout value type.");
static_assert(std::is_trivially_copyable_v<RobotState>,
              "RobotState must remain trivially copyable and allocation-free.");

} // namespace humanoid::core
