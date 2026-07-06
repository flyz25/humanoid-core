#pragma once

/**
 * @file RobotState.hpp
 * @brief Defines the vendor-independent robot state snapshot model.
 */

#include <array>
#include <chrono>
#include <cstddef>
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
 * @brief Maximum number of joint samples retained in a generic robot state.
 *
 * The value covers the Unitree G1 low-state motor array while keeping
 * RobotState allocation-free and vendor independent.
 */
inline constexpr std::size_t kMaxRobotJointStates = 35U;

/**
 * @brief Maximum number of foot/contact samples retained in a generic robot state.
 */
inline constexpr std::size_t kMaxRobotContactStates = 4U;

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
   * @brief Vendor-normalized robot mode identifier when available.
   */
  std::int32_t robotMode{0};

  /**
   * @brief Vendor-normalized motion mode identifier when available.
   */
  std::int32_t motionMode{0};

  /**
   * @brief Constructs an inactive motion state.
   */
  constexpr RobotMotionState() noexcept = default;
};

/**
 * @brief Vendor-independent IMU sample.
 */
struct RobotImuState final {
  /**
   * @brief True when the IMU sample was populated by the adapter.
   */
  bool valid{false};

  /**
   * @brief Quaternion X component.
   */
  float quaternionX{0.0F};

  /**
   * @brief Quaternion Y component.
   */
  float quaternionY{0.0F};

  /**
   * @brief Quaternion Z component.
   */
  float quaternionZ{0.0F};

  /**
   * @brief Quaternion W component.
   */
  float quaternionW{1.0F};

  /**
   * @brief Angular velocity around X, in radians per second.
   */
  float angularVelocityX{0.0F};

  /**
   * @brief Angular velocity around Y, in radians per second.
   */
  float angularVelocityY{0.0F};

  /**
   * @brief Angular velocity around Z, in radians per second.
   */
  float angularVelocityZ{0.0F};

  /**
   * @brief Linear acceleration along X, in meters per second squared.
   */
  float linearAccelerationX{0.0F};

  /**
   * @brief Linear acceleration along Y, in meters per second squared.
   */
  float linearAccelerationY{0.0F};

  /**
   * @brief Linear acceleration along Z, in meters per second squared.
   */
  float linearAccelerationZ{0.0F};

  /**
   * @brief IMU temperature in degrees Celsius when available.
   */
  float temperatureCelsius{0.0F};

  /**
   * @brief Constructs an empty IMU sample.
   */
  constexpr RobotImuState() noexcept = default;
};

/**
 * @brief Vendor-independent joint/motor state sample.
 */
struct RobotJointState final {
  /**
   * @brief True when this joint entry contains adapter-provided data.
   */
  bool valid{false};

  /**
   * @brief Joint position in radians.
   */
  float position{0.0F};

  /**
   * @brief Joint velocity in radians per second.
   */
  float velocity{0.0F};

  /**
   * @brief Joint acceleration in radians per second squared when available.
   */
  float acceleration{0.0F};

  /**
   * @brief Estimated torque in Newton meters when available.
   */
  float torque{0.0F};

  /**
   * @brief Motor voltage when available.
   */
  float voltage{0.0F};

  /**
   * @brief Primary motor temperature in degrees Celsius when available.
   */
  float temperatureCelsius{0.0F};

  /**
   * @brief Vendor-normalized motor mode.
   */
  std::uint32_t mode{0U};

  /**
   * @brief Vendor-normalized motor fault code; zero indicates no reported fault.
   */
  std::uint32_t faultCode{0U};

  /**
   * @brief Constructs an empty joint state sample.
   */
  constexpr RobotJointState() noexcept = default;
};

/**
 * @brief Vendor-independent foot/contact state sample.
 */
struct RobotContactState final {
  /**
   * @brief True when this contact entry contains adapter-provided data.
   */
  bool valid{false};

  /**
   * @brief True when contact is currently detected.
   */
  bool contact{false};

  /**
   * @brief Contact force in Newtons when available.
   */
  float force{0.0F};

  /**
   * @brief Contact sensor temperature in degrees Celsius when available.
   */
  float temperatureCelsius{0.0F};

  /**
   * @brief Constructs an empty contact state sample.
   */
  constexpr RobotContactState() noexcept = default;
};

/**
 * @brief Vendor-independent diagnostics and synchronization state.
 */
struct RobotDiagnosticsState final {
  /**
   * @brief True when low-level state feedback has been received.
   */
  bool stateFeedbackAvailable{false};

  /**
   * @brief Latest sequence or tick value reported by the robot when available.
   */
  std::uint64_t sequence{0U};

  /**
   * @brief Latest heartbeat count observed by the communication layer.
   */
  std::uint64_t heartbeatCount{0U};

  /**
   * @brief Reconnect attempts observed by the communication layer.
   */
  std::uint64_t reconnectAttemptCount{0U};

  /**
   * @brief Estimated state latency in milliseconds when available.
   */
  float latencyMs{0.0F};

  /**
   * @brief Highest motor temperature observed in the current state sample.
   */
  float maxMotorTemperatureCelsius{0.0F};

  /**
   * @brief Number of joint entries populated by the adapter.
   */
  std::uint32_t jointCount{0U};

  /**
   * @brief Number of contact entries populated by the adapter.
   */
  std::uint32_t contactCount{0U};

  /**
   * @brief Constructs empty diagnostics.
   */
  constexpr RobotDiagnosticsState() noexcept = default;
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
   * @brief IMU sample when available from the active adapter.
   */
  RobotImuState imu{};

  /**
   * @brief Fixed-capacity joint/motor state samples.
   */
  std::array<RobotJointState, kMaxRobotJointStates> joints{};

  /**
   * @brief Fixed-capacity foot/contact state samples.
   */
  std::array<RobotContactState, kMaxRobotContactStates> contacts{};

  /**
   * @brief Diagnostic and synchronization metadata.
   */
  RobotDiagnosticsState diagnostics{};

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
