#include "SdkConverter.h"

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

humanoid::adapters::RobotConnectionState
ToAdapterConnectionState(SdkConnectionState state) noexcept {
  switch (state) {
  case SdkConnectionState::kUninitialized:
    return humanoid::adapters::RobotConnectionState::kUninitialized;
  case SdkConnectionState::kInitialized:
    return humanoid::adapters::RobotConnectionState::kInitialized;
  case SdkConnectionState::kConnected:
    return humanoid::adapters::RobotConnectionState::kConnected;
  case SdkConnectionState::kDisconnected:
    return humanoid::adapters::RobotConnectionState::kDisconnected;
  case SdkConnectionState::kShutdown:
    return humanoid::adapters::RobotConnectionState::kShutdown;
  case SdkConnectionState::kFaulted:
    return humanoid::adapters::RobotConnectionState::kFaulted;
  }

  return humanoid::adapters::RobotConnectionState::kFaulted;
}

humanoid::core::RobotState ToCoreRobotState(const SdkRobotState& state) noexcept {
  humanoid::core::RobotState robot_state;
  robot_state.connection.connected = state.connection_state == SdkConnectionState::kConnected;
  robot_state.power.batteryLevel = state.battery_level;
  robot_state.power.charging = state.charging;
  robot_state.motion.standing = state.motion_mode == SdkMotionMode::kStanding;
  robot_state.motion.walking = state.motion_mode == SdkMotionMode::kWalking;
  robot_state.motion.sitting = state.motion_mode == SdkMotionMode::kSitting;
  robot_state.velocity.linearX = state.linear_x;
  robot_state.velocity.linearY = state.linear_y;
  robot_state.velocity.angularZ = state.angular_z;
  robot_state.pose.positionX = state.position_x;
  robot_state.pose.positionY = state.position_y;
  robot_state.pose.positionZ = state.position_z;
  robot_state.orientation.roll = state.roll;
  robot_state.orientation.pitch = state.pitch;
  robot_state.orientation.yaw = state.yaw;
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
