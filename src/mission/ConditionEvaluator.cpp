#include <humanoid/mission/ConditionEvaluator.h>

#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <utility>

namespace humanoid::mission {
namespace {

[[nodiscard]] MissionEventTimestamp Now() noexcept {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
}

[[nodiscard]] bool CompareFloat(float actual, MissionConditionComparison comparison,
                                float expected) noexcept {
  switch (comparison) {
  case MissionConditionComparison::Equal:
    return actual == expected;
  case MissionConditionComparison::NotEqual:
    return actual != expected;
  case MissionConditionComparison::LessThan:
    return actual < expected;
  case MissionConditionComparison::LessThanOrEqual:
    return actual <= expected;
  case MissionConditionComparison::GreaterThan:
    return actual > expected;
  case MissionConditionComparison::GreaterThanOrEqual:
    return actual >= expected;
  }
  return false;
}

[[nodiscard]] bool CompareInteger(std::int32_t actual, MissionConditionComparison comparison,
                                  std::int32_t expected) noexcept {
  switch (comparison) {
  case MissionConditionComparison::Equal:
    return actual == expected;
  case MissionConditionComparison::NotEqual:
    return actual != expected;
  case MissionConditionComparison::LessThan:
    return actual < expected;
  case MissionConditionComparison::LessThanOrEqual:
    return actual <= expected;
  case MissionConditionComparison::GreaterThan:
    return actual > expected;
  case MissionConditionComparison::GreaterThanOrEqual:
    return actual >= expected;
  }
  return false;
}

[[nodiscard]] bool CompareBool(bool actual, MissionConditionComparison comparison,
                               bool expected) noexcept {
  switch (comparison) {
  case MissionConditionComparison::Equal:
    return actual == expected;
  case MissionConditionComparison::NotEqual:
    return actual != expected;
  case MissionConditionComparison::LessThan:
  case MissionConditionComparison::LessThanOrEqual:
  case MissionConditionComparison::GreaterThan:
  case MissionConditionComparison::GreaterThanOrEqual:
    return false;
  }
  return false;
}

[[nodiscard]] MissionEvent MakeEvent(const MissionCondition& condition, MissionEventType type,
                                     std::string message) {
  MissionEvent event;
  event.conditionId = condition.id;
  event.type = type;
  event.timestamp = Now();
  event.message = std::move(message);
  return event;
}

[[nodiscard]] ConditionEvaluationResult Unavailable(const MissionCondition& condition,
                                                    std::string message) {
  ConditionEvaluationResult result;
  result.evaluated = false;
  result.satisfied = false;
  result.event = MakeEvent(condition, MissionEventType::ConditionUnavailable, std::move(message));
  return result;
}

[[nodiscard]] ConditionEvaluationResult Evaluated(const MissionCondition& condition,
                                                  bool satisfied) {
  ConditionEvaluationResult result;
  result.evaluated = true;
  result.satisfied = satisfied;
  result.event = MakeEvent(condition,
                           satisfied ? MissionEventType::ConditionSatisfied
                                     : MissionEventType::ConditionFailed,
                           std::string{toString(condition.type)} +
                               (satisfied ? " condition satisfied" : " condition failed"));
  return result;
}

} // namespace

ConditionEvaluator::ConditionEvaluator(
    std::shared_ptr<const humanoid::core::RobotStateManager> state_manager,
    ConditionEvaluatorContext context)
    : state_manager_(std::move(state_manager)), context_(context) {}

void ConditionEvaluator::UpdateCapabilities(humanoid::core::CommandCapabilitySet capabilities) {
  std::lock_guard<std::mutex> lock{context_mutex_};
  context_.capabilities = capabilities;
  context_.capabilitiesAvailable = true;
}

void ConditionEvaluator::ClearCapabilities() {
  std::lock_guard<std::mutex> lock{context_mutex_};
  context_.capabilities = humanoid::core::CommandCapabilitySet{};
  context_.capabilitiesAvailable = false;
}

ConditionEvaluationResult ConditionEvaluator::Evaluate(const MissionCondition& condition) const {
  if (!condition.isValid()) {
    return Unavailable(condition, "Mission condition is not valid");
  }

  if (condition.type == MissionConditionType::Capability) {
    ConditionEvaluatorContext context;
    {
      std::lock_guard<std::mutex> lock{context_mutex_};
      context = context_;
    }
    if (!context.capabilitiesAvailable) {
      return Unavailable(condition, "Robot capabilities are unavailable");
    }
    return Evaluated(condition, CompareBool(context.capabilities.Supports(condition.commandType),
                                            condition.comparison, condition.boolValue));
  }

  if (!state_manager_) {
    return Unavailable(condition, "Robot state manager is unavailable");
  }

  const humanoid::core::RobotState state = state_manager_->GetState();
  switch (condition.type) {
  case MissionConditionType::BatteryLevel:
    if (!std::isfinite(state.power.batteryLevel)) {
      return Unavailable(condition, "Battery level is not finite");
    }
    return Evaluated(condition, CompareFloat(state.power.batteryLevel, condition.comparison,
                                             condition.numericValue));
  case MissionConditionType::Connection:
    return Evaluated(condition, CompareBool(state.connection.connected, condition.comparison,
                                            condition.boolValue));
  case MissionConditionType::RobotStanding:
    return Evaluated(condition,
                     CompareBool(state.motion.standing, condition.comparison, condition.boolValue));
  case MissionConditionType::RobotWalking:
    return Evaluated(condition,
                     CompareBool(state.motion.walking, condition.comparison, condition.boolValue));
  case MissionConditionType::RobotSitting:
    return Evaluated(condition,
                     CompareBool(state.motion.sitting, condition.comparison, condition.boolValue));
  case MissionConditionType::FaultCode:
    return Evaluated(condition, CompareInteger(state.health.faultCode, condition.comparison,
                                               condition.faultCode));
  case MissionConditionType::EmergencyStop:
    return Evaluated(condition, CompareBool(state.health.emergencyStop, condition.comparison,
                                            condition.boolValue));
  case MissionConditionType::Capability:
    break;
  }

  return Unavailable(condition, "Unknown mission condition type");
}

std::vector<ConditionEvaluationResult>
ConditionEvaluator::EvaluateAll(const std::vector<MissionCondition>& conditions) const {
  std::vector<ConditionEvaluationResult> results;
  results.reserve(conditions.size());
  for (const MissionCondition& condition : conditions) {
    results.push_back(Evaluate(condition));
  }
  return results;
}

} // namespace humanoid::mission
