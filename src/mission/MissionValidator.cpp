#include <humanoid/mission/MissionValidator.h>

#include <cstddef>
#include <string>

#include <humanoid/core/CommandPriority.h>
#include <humanoid/core/CommandType.h>

namespace humanoid::mission {
namespace {

[[nodiscard]] MissionValidationResult Valid() {
  return MissionValidationResult{true, "Mission schema is valid"};
}

[[nodiscard]] MissionValidationResult Invalid(std::string message) {
  return MissionValidationResult{false, std::move(message)};
}

[[nodiscard]] bool IsKnown(humanoid::core::CommandType type) noexcept {
  switch (type) {
  case humanoid::core::CommandType::Stand:
  case humanoid::core::CommandType::Sit:
  case humanoid::core::CommandType::Walk:
  case humanoid::core::CommandType::Stop:
  case humanoid::core::CommandType::Move:
  case humanoid::core::CommandType::Rotate:
  case humanoid::core::CommandType::Velocity:
  case humanoid::core::CommandType::EmergencyStop:
  case humanoid::core::CommandType::HandOpen:
  case humanoid::core::CommandType::HandClose:
  case humanoid::core::CommandType::Gesture:
  case humanoid::core::CommandType::PlayAudio:
  case humanoid::core::CommandType::StopAudio:
  case humanoid::core::CommandType::SetVolume:
  case humanoid::core::CommandType::MuteAudio:
  case humanoid::core::CommandType::Custom:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsKnown(humanoid::core::CommandPriority priority) noexcept {
  switch (priority) {
  case humanoid::core::CommandPriority::Low:
  case humanoid::core::CommandPriority::Normal:
  case humanoid::core::CommandPriority::High:
  case humanoid::core::CommandPriority::Critical:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsKnown(MissionConditionType type) noexcept {
  switch (type) {
  case MissionConditionType::BatteryLevel:
  case MissionConditionType::Connection:
  case MissionConditionType::RobotStanding:
  case MissionConditionType::RobotWalking:
  case MissionConditionType::RobotSitting:
  case MissionConditionType::Capability:
  case MissionConditionType::FaultCode:
  case MissionConditionType::EmergencyStop:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsKnown(MissionConditionComparison comparison) noexcept {
  switch (comparison) {
  case MissionConditionComparison::Equal:
  case MissionConditionComparison::NotEqual:
  case MissionConditionComparison::LessThan:
  case MissionConditionComparison::LessThanOrEqual:
  case MissionConditionComparison::GreaterThan:
  case MissionConditionComparison::GreaterThanOrEqual:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsKnown(MissionConditionFailureAction action) noexcept {
  switch (action) {
  case MissionConditionFailureAction::Skip:
  case MissionConditionFailureAction::Abort:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsBooleanComparison(MissionConditionComparison comparison) noexcept {
  return comparison == MissionConditionComparison::Equal ||
         comparison == MissionConditionComparison::NotEqual;
}

[[nodiscard]] MissionValidationResult ValidateCondition(const MissionCondition& condition,
                                                        std::size_t step_index,
                                                        std::size_t condition_index) {
  const std::string prefix = "Mission step " + std::to_string(step_index) + " condition " +
                             std::to_string(condition_index) + ": ";
  if (condition.id == 0U) {
    return Invalid(prefix + "id must be nonzero");
  }
  if (condition.name.empty()) {
    return Invalid(prefix + "name is required");
  }
  if (!IsKnown(condition.type)) {
    return Invalid(prefix + "type is unknown");
  }
  if (!IsKnown(condition.comparison)) {
    return Invalid(prefix + "comparison is unknown");
  }
  if (!IsKnown(condition.failureAction)) {
    return Invalid(prefix + "on_failure is unknown");
  }
  if ((condition.type == MissionConditionType::Connection ||
       condition.type == MissionConditionType::RobotStanding ||
       condition.type == MissionConditionType::RobotWalking ||
       condition.type == MissionConditionType::RobotSitting ||
       condition.type == MissionConditionType::Capability ||
       condition.type == MissionConditionType::EmergencyStop) &&
      !IsBooleanComparison(condition.comparison)) {
    return Invalid(prefix + "boolean condition requires Equal or NotEqual comparison");
  }
  if (condition.type == MissionConditionType::Capability && !IsKnown(condition.commandType)) {
    return Invalid(prefix + "capability command_type is unknown");
  }

  return Valid();
}

[[nodiscard]] MissionValidationResult ValidateStep(const MissionStep& step,
                                                   std::size_t step_index) {
  const std::string prefix = "Mission step " + std::to_string(step_index) + ": ";
  if (!step.enabled) {
    return Valid();
  }
  if (step.id == 0U) {
    return Invalid(prefix + "id must be nonzero");
  }
  if (step.name.empty()) {
    return Invalid(prefix + "name is required");
  }
  if (step.timeout < MissionStepTimeout::zero()) {
    return Invalid(prefix + "timeout_ms must not be negative");
  }
  if (!step.retryPolicy.isValid()) {
    return Invalid(prefix + "retry_policy is not valid");
  }
  if (!step.loopPolicy.isValid()) {
    return Invalid(prefix + "loop_policy.iterations must be greater than zero");
  }
  if (!step.timeoutPolicy.isValid()) {
    return Invalid(prefix + "timeout_policy.timeout_ms must not be negative");
  }
  if (step.wait.has_value() && !step.wait->isValid()) {
    return Invalid(prefix + "wait.duration_ms must not be negative");
  }
  if (step.delay.has_value() && !step.delay->isValid()) {
    return Invalid(prefix + "delay.duration_ms must not be negative");
  }
  for (std::size_t index = 0U; index < step.conditions.size(); ++index) {
    const MissionValidationResult condition_result =
        ValidateCondition(step.conditions[index], step_index, index);
    if (!condition_result.Succeeded()) {
      return condition_result;
    }
  }
  if (step.skip || step.abort || step.wait.has_value() || step.delay.has_value()) {
    return Valid();
  }
  if (step.command.id == 0U) {
    return Invalid(prefix + "command.id must be nonzero");
  }
  if (!IsKnown(step.command.type)) {
    return Invalid(prefix + "command.type is unknown");
  }
  if (!IsKnown(step.command.priority)) {
    return Invalid(prefix + "command.priority is unknown");
  }
  if (step.command.timeout < humanoid::core::CommandTimeout::zero()) {
    return Invalid(prefix + "command.timeout_ms must not be negative");
  }
  if (!step.command.isValid()) {
    return Invalid(prefix + "command is not valid");
  }

  return Valid();
}

} // namespace

MissionValidationResult MissionValidator::Validate(const Mission& mission) const {
  if (mission.id == 0U) {
    return Invalid("Mission id must be nonzero");
  }
  if (mission.name.empty()) {
    return Invalid("Mission name is required");
  }
  if (mission.version.empty()) {
    return Invalid("Mission version is required");
  }
  if (mission.author.empty()) {
    return Invalid("Mission author is required");
  }
  if (mission.steps.empty()) {
    return Invalid("Mission requires at least one step");
  }

  bool has_enabled_step = false;
  for (std::size_t index = 0U; index < mission.steps.size(); ++index) {
    if (mission.steps[index].enabled) {
      has_enabled_step = true;
    }

    const MissionValidationResult result = ValidateStep(mission.steps[index], index);
    if (!result.Succeeded()) {
      return result;
    }
  }

  if (!has_enabled_step) {
    return Invalid("Mission requires at least one enabled step");
  }

  return Valid();
}

} // namespace humanoid::mission
