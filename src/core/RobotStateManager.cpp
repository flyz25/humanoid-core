#include "RobotStateManager.h"

#include <mutex>
#include <shared_mutex>

namespace humanoid::core {

void RobotStateManager::UpdateState(const RobotState& state) {
  std::unique_lock<std::shared_mutex> lock{mutex_};
  state_ = state;
}

RobotState RobotStateManager::GetState() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return state_;
}

void RobotStateManager::Reset() {
  std::unique_lock<std::shared_mutex> lock{mutex_};
  state_ = RobotState{};
}

bool RobotStateManager::IsConnected() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return state_.connection.connected;
}

float RobotStateManager::BatteryLevel() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return state_.power.batteryLevel;
}

bool RobotStateManager::EmergencyStop() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return state_.health.emergencyStop;
}

std::int32_t RobotStateManager::FaultCode() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return state_.health.faultCode;
}

} // namespace humanoid::core
