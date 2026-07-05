/**
 * @file mission_condition_unit_test.cpp
 * @brief Validates mission condition evaluation without robot hardware.
 */

#include <chrono>
#include <memory>
#include <stdexcept>

#include <humanoid/core.hpp>

namespace {

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

[[nodiscard]] humanoid::mission::MissionCondition BatteryCondition(float minimum) {
  humanoid::mission::MissionCondition condition;
  condition.id = 1U;
  condition.name = "Battery minimum";
  condition.type = humanoid::mission::MissionConditionType::BatteryLevel;
  condition.comparison = humanoid::mission::MissionConditionComparison::GreaterThanOrEqual;
  condition.numericValue = minimum;
  return condition;
}

void TestBatteryConditionTrue() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state;
  state.power.batteryLevel = 75.0F;
  state_manager->UpdateState(state);

  const humanoid::mission::ConditionEvaluator evaluator{state_manager};
  const humanoid::mission::ConditionEvaluationResult result =
      evaluator.Evaluate(BatteryCondition(50.0F));

  Check(result.Succeeded(), "Battery condition did not evaluate true");
  Check(result.event.type == humanoid::mission::MissionEventType::ConditionSatisfied,
        "Battery true condition produced wrong event");
}

void TestBatteryConditionFalse() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state;
  state.power.batteryLevel = 20.0F;
  state_manager->UpdateState(state);

  const humanoid::mission::ConditionEvaluator evaluator{state_manager};
  const humanoid::mission::ConditionEvaluationResult result =
      evaluator.Evaluate(BatteryCondition(50.0F));

  Check(result.evaluated, "Battery condition did not evaluate");
  Check(!result.satisfied, "Battery condition unexpectedly evaluated true");
  Check(result.event.type == humanoid::mission::MissionEventType::ConditionFailed,
        "Battery false condition produced wrong event");
}

void TestConnectionAndEmergencyStopConditions() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state;
  state.connection.connected = true;
  state.health.emergencyStop = false;
  state_manager->UpdateState(state);

  humanoid::mission::MissionCondition connected;
  connected.id = 2U;
  connected.name = "Connected";
  connected.type = humanoid::mission::MissionConditionType::Connection;
  connected.boolValue = true;

  humanoid::mission::MissionCondition emergency_stop_clear;
  emergency_stop_clear.id = 3U;
  emergency_stop_clear.name = "Emergency stop clear";
  emergency_stop_clear.type = humanoid::mission::MissionConditionType::EmergencyStop;
  emergency_stop_clear.boolValue = false;

  const humanoid::mission::ConditionEvaluator evaluator{state_manager};
  Check(evaluator.Evaluate(connected).Succeeded(), "Connection condition failed");
  Check(evaluator.Evaluate(emergency_stop_clear).Succeeded(), "Emergency stop condition failed");
}

void TestCapabilityCondition() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::mission::ConditionEvaluator evaluator{state_manager};

  humanoid::mission::MissionCondition move_capability;
  move_capability.id = 4U;
  move_capability.name = "Move supported";
  move_capability.type = humanoid::mission::MissionConditionType::Capability;
  move_capability.commandType = humanoid::core::CommandType::Move;
  move_capability.boolValue = true;

  Check(!evaluator.Evaluate(move_capability).evaluated,
        "Capability condition evaluated without capabilities");

  evaluator.UpdateCapabilities(humanoid::core::CommandCapabilitySet::LegacyAdapterDefaults());
  Check(evaluator.Evaluate(move_capability).Succeeded(), "Capability condition failed");
}

void TestFaultAndRobotStateConditions() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state;
  state.motion.standing = true;
  state.health.faultCode = 0;
  state_manager->UpdateState(state);

  humanoid::mission::MissionCondition standing;
  standing.id = 5U;
  standing.name = "Standing";
  standing.type = humanoid::mission::MissionConditionType::RobotStanding;
  standing.boolValue = true;

  humanoid::mission::MissionCondition no_fault;
  no_fault.id = 6U;
  no_fault.name = "No fault";
  no_fault.type = humanoid::mission::MissionConditionType::FaultCode;
  no_fault.comparison = humanoid::mission::MissionConditionComparison::Equal;
  no_fault.faultCode = 0;

  const humanoid::mission::ConditionEvaluator evaluator{state_manager};
  Check(evaluator.Evaluate(standing).Succeeded(), "Robot standing condition failed");
  Check(evaluator.Evaluate(no_fault).Succeeded(), "Fault condition failed");
}

} // namespace

int main() {
  try {
    TestBatteryConditionTrue();
    TestBatteryConditionFalse();
    TestConnectionAndEmergencyStopConditions();
    TestCapabilityCondition();
    TestFaultAndRobotStateConditions();
  } catch (...) {
    return 1;
  }

  return 0;
}
