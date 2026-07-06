#include <humanoid/core/SafetyValidator.h>

#include <chrono>
#include <cmath>
#include <string>
#include <utility>

namespace humanoid::core {
namespace {

[[nodiscard]] CommandResult Result(CommandStatus status, std::string message) {
  CommandResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] bool IsKnown(CommandType type) noexcept {
  switch (type) {
  case CommandType::Stand:
  case CommandType::Sit:
  case CommandType::Walk:
  case CommandType::Stop:
  case CommandType::Move:
  case CommandType::Rotate:
  case CommandType::Velocity:
  case CommandType::EmergencyStop:
  case CommandType::HandOpen:
  case CommandType::HandClose:
  case CommandType::Gesture:
  case CommandType::PlayAudio:
  case CommandType::StopAudio:
  case CommandType::SetVolume:
  case CommandType::MuteAudio:
  case CommandType::Custom:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsKnown(CommandPriority priority) noexcept {
  switch (priority) {
  case CommandPriority::Low:
  case CommandPriority::Normal:
  case CommandPriority::High:
  case CommandPriority::Critical:
    return true;
  }

  return false;
}

[[nodiscard]] bool HasExpired(const Command& command) noexcept {
  if (!command.hasTimeout() || command.timestamp == CommandTimestamp{}) {
    return false;
  }

  const CommandTimestamp now =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return now >= command.timestamp && now - command.timestamp >= command.timeout;
}

[[nodiscard]] bool IsStopCommand(CommandType type) noexcept {
  return type == CommandType::Stop || type == CommandType::EmergencyStop;
}

[[nodiscard]] bool IsBatteryPercent(float value) noexcept {
  constexpr float kMinimumBatteryPercent{0.0F};
  constexpr float kMaximumBatteryPercent{100.0F};
  return std::isfinite(value) && value >= kMinimumBatteryPercent && value <= kMaximumBatteryPercent;
}

[[nodiscard]] int ActivePostureCount(const RobotMotionState& motion) noexcept {
  int count{0};
  if (motion.standing) {
    ++count;
  }
  if (motion.sitting) {
    ++count;
  }
  return count;
}

[[nodiscard]] bool HasContradictoryMotionState(const RobotMotionState& motion) noexcept {
  return ActivePostureCount(motion) > 1 || (motion.walking && motion.sitting);
}

[[nodiscard]] bool ShouldCheckBattery(CommandType type) noexcept {
  return SafetyValidator::IsActuatorCommand(type);
}

} // namespace

CommandResult SafetyValidator::Validate(const Command& command,
                                        const SafetyValidationContext& context) const {
  if (command.id == 0U) {
    return Result(CommandStatus::Rejected, "Command identifier must be nonzero");
  }
  if (command.timeout < CommandTimeout::zero()) {
    return Result(CommandStatus::Rejected, "Command timeout must not be negative");
  }
  if (!IsKnown(command.type)) {
    return Result(CommandStatus::Rejected, "Command type is invalid");
  }
  if (!IsKnown(command.priority)) {
    return Result(CommandStatus::Rejected, "Command priority is invalid");
  }
  if (command.hasTimeout()) {
    const CommandTimestamp now =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
    if (command.timestamp == CommandTimestamp{}) {
      return Result(CommandStatus::Rejected,
                    "Timed commands require a monotonic creation timestamp");
    }
    if (command.timestamp > now) {
      return Result(CommandStatus::Rejected, "Command timestamp must not be in the future");
    }
    if (HasExpired(command)) {
      return Result(CommandStatus::Timeout, "Command expired before safety validation");
    }
  }

  if (!context.robotStateAvailable) {
    return Result(CommandStatus::Rejected, "Robot state is unavailable");
  }
  if (!context.capabilitiesAvailable) {
    return Result(CommandStatus::Rejected, "Robot capabilities are unavailable");
  }
  if (!context.capabilities.Supports(command.type)) {
    return Result(CommandStatus::Rejected, "Command capability is not supported");
  }

  const RobotState& state = context.robotState;
  if (options_.requireConnection && !state.connection.connected) {
    return Result(CommandStatus::Rejected, "Robot is not connected");
  }

  const bool stop_command = IsStopCommand(command.type);
  if (options_.rejectEmergencyStop && state.health.emergencyStop &&
      !(options_.allowStopDuringEmergencyStop && stop_command)) {
    return Result(CommandStatus::Rejected, "Emergency stop is active");
  }
  if (options_.rejectRobotFaults && state.health.faultCode != 0 && !stop_command) {
    return Result(CommandStatus::Rejected, "Robot fault is active");
  }

  if (options_.enforceMotionStateConsistency && !stop_command &&
      HasContradictoryMotionState(state.motion)) {
    return Result(CommandStatus::Rejected, "Robot motion state is inconsistent");
  }

  if (options_.requireBatteryStateForActuatorCommands && ShouldCheckBattery(command.type)) {
    if (!context.batteryStateAvailable) {
      return Result(CommandStatus::Rejected, "Robot battery state is unavailable");
    }
    if (options_.enforceBatteryRange && !IsBatteryPercent(state.power.batteryLevel)) {
      return Result(CommandStatus::Rejected, "Robot battery level is invalid");
    }
    if (state.power.batteryLevel < options_.minimumBatteryLevelPercent) {
      return Result(CommandStatus::Rejected, "Robot battery level is below safety threshold");
    }
  }

  if (options_.requireStandingForBaseMotion && IsBaseMotionCommand(command.type)) {
    if (state.motion.sitting) {
      return Result(CommandStatus::Rejected, "Base motion is rejected while robot is sitting");
    }
    if (!state.motion.standing) {
      return Result(CommandStatus::Rejected, "Base motion requires a standing robot state");
    }
  }
  if (state.motion.walking &&
      (command.type == CommandType::Stand || command.type == CommandType::Sit)) {
    return Result(CommandStatus::Rejected, "Posture command requires stopped base motion");
  }

  return Result(CommandStatus::Completed, "Command passed safety validation");
}

} // namespace humanoid::core
