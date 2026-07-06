#include "SdkConverter.h"

#include <cstdint>
#include <utility>

namespace humanoid::plugins::unitree::sdk {
namespace {

[[nodiscard]] humanoid::adapters::ErrorCode ToAdapterErrorCode(SdkErrorCode code) noexcept {
  switch (code) {
  case SdkErrorCode::kSuccess:
    return humanoid::adapters::ErrorCode::kSuccess;
  case SdkErrorCode::kSdkUnavailable:
    return humanoid::adapters::ErrorCode::kSDKUnavailable;
  case SdkErrorCode::kConnectionFailed:
    return humanoid::adapters::ErrorCode::kConnectionFailed;
  case SdkErrorCode::kTimeout:
    return humanoid::adapters::ErrorCode::kTimeout;
  case SdkErrorCode::kRobotFault:
    return humanoid::adapters::ErrorCode::kRobotFault;
  case SdkErrorCode::kUnknown:
    return humanoid::adapters::ErrorCode::kUnknown;
  }

  return humanoid::adapters::ErrorCode::kUnknown;
}

} // namespace

humanoid::adapters::Result ToAdapterResult(const SdkResult& result) {
  return humanoid::adapters::Result{ToAdapterErrorCode(result.code), result.message};
}

humanoid::core::RobotState ToCoreRobotState(const SdkRobotState& state) noexcept {
  humanoid::core::RobotState robot_state;
  robot_state.connection.connected = state.connection_state == SdkConnectionState::kConnected;
  robot_state.power.batteryLevel = state.battery_level;
  robot_state.power.charging = state.charging;
  robot_state.motion.standing = state.motion_mode == SdkMotionMode::kStanding;
  robot_state.motion.walking = state.motion_mode == SdkMotionMode::kWalking;
  robot_state.motion.sitting = state.motion_mode == SdkMotionMode::kSitting;
  robot_state.motion.robotMode = state.robot_mode;
  robot_state.motion.motionMode = state.motion_mode_id;
  robot_state.velocity.linearX = state.linear_x;
  robot_state.velocity.linearY = state.linear_y;
  robot_state.velocity.angularZ = state.angular_z;
  robot_state.pose.positionX = state.position_x;
  robot_state.pose.positionY = state.position_y;
  robot_state.pose.positionZ = state.position_z;
  robot_state.orientation.roll = state.roll;
  robot_state.orientation.pitch = state.pitch;
  robot_state.orientation.yaw = state.yaw;
  robot_state.imu.valid = state.low_state_available;
  robot_state.imu.quaternionX = state.imu_quaternion[0];
  robot_state.imu.quaternionY = state.imu_quaternion[1];
  robot_state.imu.quaternionZ = state.imu_quaternion[2];
  robot_state.imu.quaternionW = state.imu_quaternion[3];
  robot_state.imu.angularVelocityX = state.imu_angular_velocity[0];
  robot_state.imu.angularVelocityY = state.imu_angular_velocity[1];
  robot_state.imu.angularVelocityZ = state.imu_angular_velocity[2];
  robot_state.imu.linearAccelerationX = state.imu_linear_acceleration[0];
  robot_state.imu.linearAccelerationY = state.imu_linear_acceleration[1];
  robot_state.imu.linearAccelerationZ = state.imu_linear_acceleration[2];
  robot_state.imu.temperatureCelsius = state.imu_temperature_celsius;
  robot_state.diagnostics.stateFeedbackAvailable = state.low_state_available;
  robot_state.diagnostics.sequence = state.low_state_tick;
  robot_state.diagnostics.heartbeatCount = state.heartbeat_count;
  robot_state.diagnostics.reconnectAttemptCount = state.reconnect_attempt_count;
  robot_state.diagnostics.latencyMs = state.latency_ms;
  robot_state.diagnostics.jointCount = state.joint_count;
  robot_state.diagnostics.contactCount = state.contact_count;

  float max_temperature = 0.0F;
  const std::uint32_t joint_count =
      state.joint_count > humanoid::core::kMaxRobotJointStates
          ? static_cast<std::uint32_t>(humanoid::core::kMaxRobotJointStates)
          : state.joint_count;
  for (std::uint32_t index = 0U; index < joint_count; ++index) {
    robot_state.joints[index].valid = true;
    robot_state.joints[index].position = state.joint_position[index];
    robot_state.joints[index].velocity = state.joint_velocity[index];
    robot_state.joints[index].acceleration = state.joint_acceleration[index];
    robot_state.joints[index].torque = state.joint_torque[index];
    robot_state.joints[index].voltage = state.joint_voltage[index];
    robot_state.joints[index].temperatureCelsius = state.joint_temperature_celsius[index];
    robot_state.joints[index].mode = state.joint_mode[index];
    robot_state.joints[index].faultCode = state.joint_fault[index];
    if (state.joint_temperature_celsius[index] > max_temperature) {
      max_temperature = state.joint_temperature_celsius[index];
    }
  }
  robot_state.diagnostics.maxMotorTemperatureCelsius = max_temperature;

  const std::uint32_t contact_count =
      state.contact_count > humanoid::core::kMaxRobotContactStates
          ? static_cast<std::uint32_t>(humanoid::core::kMaxRobotContactStates)
          : state.contact_count;
  for (std::uint32_t index = 0U; index < contact_count; ++index) {
    robot_state.contacts[index].valid = true;
    robot_state.contacts[index].force = state.contact_force[index];
    robot_state.contacts[index].temperatureCelsius = state.contact_temperature_celsius[index];
    robot_state.contacts[index].contact = state.contact_force[index] > 0.0F;
  }
  robot_state.health.emergencyStop = state.emergency_stop;
  robot_state.health.faultCode = state.fault_code;
  robot_state.timestamp = state.timestamp;
  return robot_state;
}

std::string_view toString(SdkErrorCode code) noexcept {
  switch (code) {
  case SdkErrorCode::kSuccess:
    return "success";
  case SdkErrorCode::kSdkUnavailable:
    return "sdk_unavailable";
  case SdkErrorCode::kConnectionFailed:
    return "connection_failed";
  case SdkErrorCode::kTimeout:
    return "timeout";
  case SdkErrorCode::kRobotFault:
    return "robot_fault";
  case SdkErrorCode::kUnknown:
    return "unknown";
  }

  return "unknown";
}

std::string_view toString(SdkConnectionState state) noexcept {
  switch (state) {
  case SdkConnectionState::kUninitialized:
    return "uninitialized";
  case SdkConnectionState::kInitialized:
    return "initialized";
  case SdkConnectionState::kConnected:
    return "connected";
  case SdkConnectionState::kDisconnected:
    return "disconnected";
  case SdkConnectionState::kShutdown:
    return "shutdown";
  case SdkConnectionState::kFaulted:
    return "faulted";
  }

  return "faulted";
}

std::string_view toString(SdkMotionMode mode) noexcept {
  switch (mode) {
  case SdkMotionMode::kUnknown:
    return "unknown";
  case SdkMotionMode::kIdle:
    return "idle";
  case SdkMotionMode::kStanding:
    return "standing";
  case SdkMotionMode::kWalking:
    return "walking";
  case SdkMotionMode::kSitting:
    return "sitting";
  case SdkMotionMode::kFaulted:
    return "faulted";
  }

  return "unknown";
}

} // namespace humanoid::plugins::unitree::sdk
