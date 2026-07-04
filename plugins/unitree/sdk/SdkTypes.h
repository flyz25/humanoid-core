#pragma once

/**
 * @file SdkTypes.h
 * @brief Defines SDK-abstraction types for Unitree integrations.
 */

#include <chrono>
#include <cstdint>
#include <string>

namespace humanoid::plugins::unitree::sdk {

/**
 * @brief Normalized operation status used inside the Unitree SDK boundary.
 */
enum class SdkErrorCode {
  /**
   * @brief Operation completed successfully.
   */
  kSuccess,

  /**
   * @brief Unitree SDK2 is unavailable or could not be initialized.
   */
  kSdkUnavailable,

  /**
   * @brief Robot communication could not be established or verified.
   */
  kConnectionFailed,

  /**
   * @brief Operation timed out or timeout configuration is invalid.
   */
  kTimeout,

  /**
   * @brief Robot reported or implied a locomotion fault.
   */
  kRobotFault,

  /**
   * @brief Failure category is unknown.
   */
  kUnknown
};

/**
 * @brief Result returned by Unitree SDK abstraction operations.
 */
struct SdkResult final {
  /**
   * @brief Normalized outcome code.
   */
  SdkErrorCode code{SdkErrorCode::kSuccess};

  /**
   * @brief Human-readable diagnostic message.
   */
  std::string message;

  /**
   * @brief Reports whether the operation succeeded.
   *
   * @return True when the result code is `SdkErrorCode::kSuccess`.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return code == SdkErrorCode::kSuccess; }
};

/**
 * @brief Normalized Unitree SDK connection state.
 */
enum class SdkConnectionState {
  /**
   * @brief SDK wrapper has not been initialized.
   */
  kUninitialized,

  /**
   * @brief SDK transport and client are initialized.
   */
  kInitialized,

  /**
   * @brief Robot communication has been verified.
   */
  kConnected,

  /**
   * @brief Robot communication is disconnected while local SDK resources remain initialized.
   */
  kDisconnected,

  /**
   * @brief SDK resources have been shut down.
   */
  kShutdown,

  /**
   * @brief A fault occurred inside the SDK abstraction boundary.
   */
  kFaulted
};

/**
 * @brief Normalized high-level robot motion mode.
 */
enum class SdkMotionMode {
  /**
   * @brief Motion mode is unknown or unavailable.
   */
  kUnknown,

  /**
   * @brief Robot is idle or no locomotion mode is available.
   */
  kIdle,

  /**
   * @brief Robot is standing.
   */
  kStanding,

  /**
   * @brief Robot is walking or accepting velocity motion.
   */
  kWalking,

  /**
   * @brief Robot is sitting.
   */
  kSitting,

  /**
   * @brief Robot is faulted or emergency-stopped.
   */
  kFaulted
};

/**
 * @brief Configuration consumed by the Unitree SDK abstraction layer.
 */
struct SdkConfiguration final {
  /**
   * @brief Robot control IP address.
   */
  std::string robot_ip{"192.168.123.161"};

  /**
   * @brief Network interface used by Unitree SDK2 DDS transport.
   */
  std::string network_interface{"eth0"};

  /**
   * @brief Unitree SDK2 DDS domain identifier.
   */
  std::uint32_t domain_id{0};

  /**
   * @brief Command timeout.
   */
  std::chrono::milliseconds timeout{500};

  /**
   * @brief Optional robot serial number from configuration or discovery.
   */
  std::string serial_number;

  /**
   * @brief Optional firmware version from configuration or discovery.
   */
  std::string firmware_version;
};

/**
 * @brief Vendor-normalized robot descriptor returned by discovery.
 */
struct SdkRobotDescriptor final {
  /**
   * @brief Robot vendor name.
   */
  std::string vendor{"Unitree"};

  /**
   * @brief Robot model name.
   */
  std::string model{"G1"};

  /**
   * @brief Robot control IP address.
   */
  std::string robot_ip;

  /**
   * @brief DDS network interface used for communication.
   */
  std::string network_interface;

  /**
   * @brief DDS domain identifier.
   */
  std::uint32_t domain_id{0};

  /**
   * @brief Optional robot serial number.
   */
  std::string serial_number;

  /**
   * @brief Optional firmware version.
   */
  std::string firmware_version;

  /**
   * @brief True when the SDK abstraction verified that the locomotion service is reachable.
   */
  bool discovered{false};
};

/**
 * @brief Result and descriptor returned by discovery operations.
 */
struct SdkDiscoveryResult final {
  /**
   * @brief Discovery operation result.
   */
  SdkResult result{};

  /**
   * @brief Discovered or configured robot descriptor.
   */
  SdkRobotDescriptor descriptor{};
};

/**
 * @brief Normalized robot state sample captured at the SDK abstraction boundary.
 */
struct SdkRobotState final {
  /**
   * @brief SDK abstraction connection state.
   */
  SdkConnectionState connection_state{SdkConnectionState::kUninitialized};

  /**
   * @brief Normalized high-level motion mode.
   */
  SdkMotionMode motion_mode{SdkMotionMode::kUnknown};

  /**
   * @brief Battery charge level in percent.
   */
  float battery_level{0.0F};

  /**
   * @brief True when the robot reports charging.
   */
  bool charging{false};

  /**
   * @brief Linear velocity along the robot forward X axis, in meters per second.
   */
  float linear_x{0.0F};

  /**
   * @brief Linear velocity along the robot lateral Y axis, in meters per second.
   */
  float linear_y{0.0F};

  /**
   * @brief Angular velocity around the robot vertical Z axis, in radians per second.
   */
  float angular_z{0.0F};

  /**
   * @brief Position along the X axis, in meters.
   */
  float position_x{0.0F};

  /**
   * @brief Position along the Y axis, in meters.
   */
  float position_y{0.0F};

  /**
   * @brief Position along the Z axis, in meters.
   */
  float position_z{0.0F};

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
   * @brief True when an emergency stop condition is active.
   */
  bool emergency_stop{false};

  /**
   * @brief Vendor-normalized fault code; zero means no reported fault.
   */
  std::int32_t fault_code{0};

  /**
   * @brief Monotonic timestamp for this state sample.
   */
  std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> timestamp{};
};

} // namespace humanoid::plugins::unitree::sdk
