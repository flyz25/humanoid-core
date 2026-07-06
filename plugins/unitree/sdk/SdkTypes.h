#pragma once

/**
 * @file SdkTypes.h
 * @brief Defines SDK-abstraction types for Unitree integrations.
 */

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace humanoid::plugins::unitree::sdk {

inline constexpr std::size_t kSdkMaxJointStates = 35U;
inline constexpr std::size_t kSdkMaxContactStates = 4U;

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
 * @brief Generic velocity command normalized before entering Unitree SDK2.
 */
struct SdkVelocityCommand final {
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
};

/**
 * @brief Vendor-normalized hand and upper-body gesture command.
 */
enum class SdkHandGesture {
  /**
   * @brief Raise both hands.
   */
  kHandsUp,

  /**
   * @brief Clap hands.
   */
  kClap,

  /**
   * @brief High-five gesture.
   */
  kHighFive,

  /**
   * @brief Hug gesture.
   */
  kHug,

  /**
   * @brief Heart gesture.
   */
  kHeart,

  /**
   * @brief Reject gesture.
   */
  kReject,

  /**
   * @brief Wave gesture.
   */
  kWave,

  /**
   * @brief Shake-hand gesture.
   */
  kShakeHand
};

/**
 * @brief Audio playback payload for the Unitree SDK audio stream API.
 */
struct SdkAudioPlayback final {
  /**
   * @brief Application name used by the Unitree audio service.
   */
  std::string app_name;

  /**
   * @brief Stream identifier used by the Unitree audio service.
   */
  std::string stream_id;

  /**
   * @brief PCM audio bytes passed to Unitree SDK2.
   */
  std::vector<std::uint8_t> pcm_data;
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
 * @brief Runtime options for read-only SDK communication monitoring.
 */
struct SdkCommunicationOptions final {
  /**
   * @brief Period between read-only heartbeat/state synchronization attempts.
   */
  std::chrono::milliseconds heartbeat_interval{500};

  /**
   * @brief Minimum delay between automatic reconnection attempts.
   */
  std::chrono::milliseconds reconnect_interval{1000};

  /**
   * @brief Maximum age of the last successful heartbeat before the link is marked disconnected.
   */
  std::chrono::milliseconds connection_timeout{1500};
};

/**
 * @brief Snapshot of the SDK communication worker status.
 */
struct SdkCommunicationStatus final {
  /**
   * @brief True when the communication worker thread is active.
   */
  bool running{false};

  /**
   * @brief True when SDK transport and the locomotion client have been initialized.
   */
  bool initialized{false};

  /**
   * @brief True when the latest read-only heartbeat verified robot communication.
   */
  bool connected{false};

  /**
   * @brief Latest normalized connection state.
   */
  SdkConnectionState connection_state{SdkConnectionState::kUninitialized};

  /**
   * @brief Number of heartbeat synchronization attempts performed by the worker.
   */
  std::uint64_t heartbeat_count{0};

  /**
   * @brief Number of automatic reconnect attempts performed after heartbeat loss.
   */
  std::uint64_t reconnect_attempt_count{0};

  /**
   * @brief Monotonic time of the last successful heartbeat.
   */
  std::chrono::steady_clock::time_point last_successful_heartbeat{};

  /**
   * @brief Monotonic time of the last reconnect attempt.
   */
  std::chrono::steady_clock::time_point last_reconnect_attempt{};
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
   * @brief Vendor robot mode from low-state feedback when available.
   */
  std::int32_t robot_mode{0};

  /**
   * @brief Vendor motion mode from low-state feedback when available.
   */
  std::int32_t motion_mode_id{0};

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
   * @brief Last read-only Unitree FSM identifier observed by the communication heartbeat.
   */
  std::int32_t fsm_id{-1};

  /**
   * @brief True when low-state DDS feedback has populated this sample.
   */
  bool low_state_available{false};

  /**
   * @brief Latest low-state tick value.
   */
  std::uint64_t low_state_tick{0U};

  /**
   * @brief Latest heartbeat count from the communication worker.
   */
  std::uint64_t heartbeat_count{0U};

  /**
   * @brief Latest reconnect attempt count from the communication worker.
   */
  std::uint64_t reconnect_attempt_count{0U};

  /**
   * @brief State feedback latency estimate in milliseconds.
   */
  float latency_ms{0.0F};

  /**
   * @brief IMU quaternion in x, y, z, w order.
   */
  std::array<float, 4> imu_quaternion{0.0F, 0.0F, 0.0F, 1.0F};

  /**
   * @brief IMU angular velocity in radians per second.
   */
  std::array<float, 3> imu_angular_velocity{0.0F, 0.0F, 0.0F};

  /**
   * @brief IMU linear acceleration in meters per second squared.
   */
  std::array<float, 3> imu_linear_acceleration{0.0F, 0.0F, 0.0F};

  /**
   * @brief IMU temperature in degrees Celsius.
   */
  float imu_temperature_celsius{0.0F};

  /**
   * @brief Number of populated joint entries.
   */
  std::uint32_t joint_count{0U};

  /**
   * @brief Joint position samples.
   */
  std::array<float, kSdkMaxJointStates> joint_position{};

  /**
   * @brief Joint velocity samples.
   */
  std::array<float, kSdkMaxJointStates> joint_velocity{};

  /**
   * @brief Joint acceleration samples.
   */
  std::array<float, kSdkMaxJointStates> joint_acceleration{};

  /**
   * @brief Joint torque samples.
   */
  std::array<float, kSdkMaxJointStates> joint_torque{};

  /**
   * @brief Joint voltage samples.
   */
  std::array<float, kSdkMaxJointStates> joint_voltage{};

  /**
   * @brief Primary joint temperature samples.
   */
  std::array<float, kSdkMaxJointStates> joint_temperature_celsius{};

  /**
   * @brief Vendor-normalized joint mode samples.
   */
  std::array<std::uint32_t, kSdkMaxJointStates> joint_mode{};

  /**
   * @brief Vendor-normalized joint fault samples.
   */
  std::array<std::uint32_t, kSdkMaxJointStates> joint_fault{};

  /**
   * @brief Number of populated contact entries.
   */
  std::uint32_t contact_count{0U};

  /**
   * @brief Foot/contact force samples when available.
   */
  std::array<float, kSdkMaxContactStates> contact_force{};

  /**
   * @brief Foot/contact temperature samples when available.
   */
  std::array<float, kSdkMaxContactStates> contact_temperature_celsius{};

  /**
   * @brief Monotonic timestamp for this state sample.
   */
  std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> timestamp{};
};

} // namespace humanoid::plugins::unitree::sdk
