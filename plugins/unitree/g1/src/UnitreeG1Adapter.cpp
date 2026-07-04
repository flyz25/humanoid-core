#include <humanoid/plugins/unitree/g1/UnitreeG1Adapter.hpp>

#include <chrono>
#include <string>
#include <utility>

namespace humanoid::plugins::unitree::g1 {
namespace {

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status Unavailable(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kUnavailable,
                                         std::move(message));
}

} // namespace

humanoid::common::Status UnitreeG1Adapter::Initialize() {
  initialized_.store(true, std::memory_order_release);
  connected_.store(false, std::memory_order_release);
  return humanoid::common::Status::ok();
}

humanoid::common::Status UnitreeG1Adapter::Shutdown() {
  connected_.store(false, std::memory_order_release);
  initialized_.store(false, std::memory_order_release);
  return humanoid::common::Status::ok();
}

humanoid::common::Status UnitreeG1Adapter::Connect() {
  if (!initialized_.load(std::memory_order_acquire)) {
    return FailedPrecondition("Unitree G1 plugin adapter skeleton is not initialized");
  }

  connected_.store(false, std::memory_order_release);
  return Unavailable("Unitree G1 plugin skeleton does not implement SDK communication");
}

humanoid::common::Status UnitreeG1Adapter::Disconnect() {
  connected_.store(false, std::memory_order_release);
  return humanoid::common::Status::ok();
}

bool UnitreeG1Adapter::IsConnected() const noexcept {
  return connected_.load(std::memory_order_acquire);
}

humanoid::core::RobotState UnitreeG1Adapter::GetRobotState() const {
  humanoid::core::RobotState state;
  state.connection.connected = connected_.load(std::memory_order_acquire);
  state.power.batteryLevel = 0.0F;
  state.power.charging = false;
  state.motion.standing = false;
  state.motion.walking = false;
  state.motion.sitting = false;
  state.velocity.linearX = 0.0F;
  state.velocity.linearY = 0.0F;
  state.velocity.angularZ = 0.0F;
  state.pose.positionX = 0.0F;
  state.pose.positionY = 0.0F;
  state.pose.positionZ = 0.0F;
  state.orientation.roll = 0.0F;
  state.orientation.pitch = 0.0F;
  state.orientation.yaw = 0.0F;
  state.health.emergencyStop = false;
  state.health.faultCode = 0;
  state.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return state;
}

humanoid::core::RobotInformation UnitreeG1Adapter::GetRobotInformation() const {
  humanoid::core::RobotInformation information;
  information.vendor = "Unitree";
  information.model = "G1";
  information.adapterName = "UnitreeG1PluginSkeletonAdapter";
  return information;
}

humanoid::core::RobotCapabilities UnitreeG1Adapter::GetCapabilities() const {
  humanoid::core::RobotCapabilities capabilities;
  capabilities.supportsLifecycle = true;
  capabilities.supportsConnectionManagement = false;
  capabilities.supportsStateFeedback = true;
  capabilities.supportsRobotInformation = true;
  capabilities.supportsPeriodicUpdate = true;
  capabilities.supportsPowerState = false;
  capabilities.supportsPoseEstimation = false;
  capabilities.supportsHealthState = false;
  return capabilities;
}

humanoid::common::Status UnitreeG1Adapter::Update() {
  if (!initialized_.load(std::memory_order_acquire)) {
    return FailedPrecondition("Unitree G1 plugin adapter skeleton is not initialized");
  }

  return humanoid::common::Status::ok();
}

} // namespace humanoid::plugins::unitree::g1
