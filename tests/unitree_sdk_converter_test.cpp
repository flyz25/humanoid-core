#include <SdkConverter.h>
#include <SdkTypes.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

namespace unitree_sdk = humanoid::plugins::unitree::sdk;

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] bool TestResultConversion() {
  constexpr std::string_view kTestName{"Unitree SDK result conversion"};

  const humanoid::adapters::Result success = unitree_sdk::ToAdapterResult(
      unitree_sdk::SdkResult{unitree_sdk::SdkErrorCode::kSuccess, "success"});
  if (!success.Succeeded()) {
    return Fail(kTestName, "success result did not convert");
  }

  const humanoid::adapters::Result failure = unitree_sdk::ToAdapterResult(
      unitree_sdk::SdkResult{unitree_sdk::SdkErrorCode::kRobotFault, "fault"});
  if (failure.code != humanoid::adapters::ErrorCode::kRobotFault) {
    return Fail(kTestName, "robot fault result did not convert");
  }

  return true;
}

[[nodiscard]] bool TestConnectionStateConversion() {
  constexpr std::string_view kTestName{"Unitree SDK connection conversion"};

  if (unitree_sdk::ToAdapterConnectionState(unitree_sdk::SdkConnectionState::kConnected) !=
      humanoid::adapters::RobotConnectionState::kConnected) {
    return Fail(kTestName, "connected state did not convert");
  }

  if (unitree_sdk::ToAdapterConnectionState(unitree_sdk::SdkConnectionState::kFaulted) !=
      humanoid::adapters::RobotConnectionState::kFaulted) {
    return Fail(kTestName, "faulted state did not convert");
  }

  return true;
}

[[nodiscard]] bool TestRobotStateConversion() {
  constexpr std::string_view kTestName{"Unitree SDK robot state conversion"};

  unitree_sdk::SdkRobotState sdk_state;
  sdk_state.connection_state = unitree_sdk::SdkConnectionState::kConnected;
  sdk_state.motion_mode = unitree_sdk::SdkMotionMode::kWalking;
  sdk_state.battery_level = 88.0F;
  sdk_state.charging = true;
  sdk_state.linear_x = 0.25F;
  sdk_state.linear_y = -0.10F;
  sdk_state.angular_z = 0.50F;
  sdk_state.position_x = 1.0F;
  sdk_state.position_y = 2.0F;
  sdk_state.position_z = 3.0F;
  sdk_state.roll = 0.1F;
  sdk_state.pitch = 0.2F;
  sdk_state.yaw = 0.3F;
  sdk_state.emergency_stop = true;
  sdk_state.fault_code = 7;

  const humanoid::core::RobotState robot_state = unitree_sdk::ToCoreRobotState(sdk_state);
  if (!robot_state.connection.connected || !robot_state.motion.walking ||
      robot_state.motion.standing || robot_state.power.batteryLevel != 88.0F ||
      !robot_state.power.charging || robot_state.velocity.linearX != 0.25F ||
      robot_state.health.faultCode != 7 || !robot_state.health.emergencyStop) {
    return Fail(kTestName, "robot state fields did not convert");
  }

  return true;
}

[[nodiscard]] bool TestStringConversion() {
  constexpr std::string_view kTestName{"Unitree SDK string conversion"};

  if (unitree_sdk::toString(unitree_sdk::SdkErrorCode::kTimeout) != "timeout") {
    return Fail(kTestName, "error code string conversion failed");
  }

  if (unitree_sdk::toString(unitree_sdk::SdkConnectionState::kShutdown) != "shutdown") {
    return Fail(kTestName, "connection state string conversion failed");
  }

  if (unitree_sdk::toString(unitree_sdk::SdkMotionMode::kStanding) != "standing") {
    return Fail(kTestName, "motion mode string conversion failed");
  }

  return true;
}

[[nodiscard]] bool TestCommunicationTypeDefaults() {
  constexpr std::string_view kTestName{"Unitree SDK communication defaults"};

  const unitree_sdk::SdkCommunicationOptions options;
  if (options.heartbeat_interval <= std::chrono::milliseconds{0} ||
      options.reconnect_interval <= std::chrono::milliseconds{0} ||
      options.connection_timeout <= std::chrono::milliseconds{0}) {
    return Fail(kTestName, "communication timing defaults are not positive");
  }

  const unitree_sdk::SdkCommunicationStatus status;
  if (status.running || status.initialized || status.connected ||
      status.connection_state != unitree_sdk::SdkConnectionState::kUninitialized ||
      status.heartbeat_count != 0U || status.reconnect_attempt_count != 0U) {
    return Fail(kTestName, "communication status defaults are not conservative");
  }

  const unitree_sdk::SdkRobotState state;
  if (state.fsm_id != -1) {
    return Fail(kTestName, "robot state FSM id default is not unknown");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestResultConversion, TestConnectionStateConversion, TestRobotStateConversion,
      TestStringConversion, TestCommunicationTypeDefaults,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
