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
  case humanoid::core::CommandType::HandOpen:
  case humanoid::core::CommandType::HandClose:
  case humanoid::core::CommandType::PlayAudio:
  case humanoid::core::CommandType::StopAudio:
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
