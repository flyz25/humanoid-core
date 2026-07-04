#pragma once

/**
 * @file SafetyValidator.h
 * @brief Defines vendor-independent command safety validation.
 */

#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/RobotState.hpp>

namespace humanoid::core {

/**
 * @brief Vendor-independent command capability declaration.
 *
 * The capability set describes which generic command categories are permitted
 * by the currently selected robot integration. It contains no SDK types and no
 * vendor-specific model assumptions.
 */
struct CommandCapabilitySet final {
  /**
   * @brief True when an upright standing command is supported.
   */
  bool stand{false};

  /**
   * @brief True when a seated posture command is supported.
   */
  bool sit{false};

  /**
   * @brief True when walking commands are supported.
   */
  bool walk{false};

  /**
   * @brief True when active motion can be stopped.
   */
  bool stop{false};

  /**
   * @brief True when translational velocity commands are supported.
   */
  bool move{false};

  /**
   * @brief True when rotational velocity commands are supported.
   */
  bool rotate{false};

  /**
   * @brief True when hand-open commands are supported.
   */
  bool handOpen{false};

  /**
   * @brief True when hand-close commands are supported.
   */
  bool handClose{false};

  /**
   * @brief True when audio playback commands are supported.
   */
  bool playAudio{false};

  /**
   * @brief True when audio stop commands are supported.
   */
  bool stopAudio{false};

  /**
   * @brief True when application-defined custom commands are supported.
   */
  bool custom{false};

  /**
   * @brief Returns the capabilities provided by the legacy `IRobotAdapter`.
   *
   * @return Capability set matching the currently dispatchable adapter methods.
   */
  [[nodiscard]] static constexpr CommandCapabilitySet LegacyAdapterDefaults() noexcept {
    CommandCapabilitySet capabilities;
    capabilities.stand = true;
    capabilities.walk = true;
    capabilities.stop = true;
    capabilities.move = true;
    capabilities.rotate = true;
    return capabilities;
  }

  /**
   * @brief Reports whether a command type is supported.
   *
   * @param type Command type to inspect.
   * @return True when the command type is enabled in this capability set.
   */
  [[nodiscard]] constexpr bool Supports(CommandType type) const noexcept {
    switch (type) {
    case CommandType::Stand:
      return stand;
    case CommandType::Sit:
      return sit;
    case CommandType::Walk:
      return walk;
    case CommandType::Stop:
      return stop;
    case CommandType::Move:
      return move;
    case CommandType::Rotate:
      return rotate;
    case CommandType::HandOpen:
      return handOpen;
    case CommandType::HandClose:
      return handClose;
    case CommandType::PlayAudio:
      return playAudio;
    case CommandType::StopAudio:
      return stopAudio;
    case CommandType::Custom:
      return custom;
    }

    return false;
  }
};

/**
 * @brief Runtime context used to evaluate command safety.
 */
struct SafetyValidationContext final {
  /**
   * @brief Latest vendor-independent robot state snapshot.
   */
  RobotState robotState{};

  /**
   * @brief Generic capabilities for the active robot integration.
   */
  CommandCapabilitySet capabilities{};

  /**
   * @brief True when `robotState` came from a valid state source.
   */
  bool robotStateAvailable{true};

  /**
   * @brief True when battery level in `robotState.power` is trusted.
   */
  bool batteryStateAvailable{true};

  /**
   * @brief True when `capabilities` came from a valid capability source.
   */
  bool capabilitiesAvailable{true};
};

/**
 * @brief Policy options for command safety validation.
 */
struct SafetyValidatorOptions final {
  /**
   * @brief Minimum battery percentage required for actuator commands.
   */
  float minimumBatteryLevelPercent{15.0F};

  /**
   * @brief True when commands require an active robot connection.
   */
  bool requireConnection{true};

  /**
   * @brief True when actuator commands require trusted battery state.
   */
  bool requireBatteryStateForActuatorCommands{true};

  /**
   * @brief True when battery percentages outside [0, 100] are rejected.
   */
  bool enforceBatteryRange{true};

  /**
   * @brief True when active emergency stop rejects unsafe commands.
   */
  bool rejectEmergencyStop{true};

  /**
   * @brief True when `Stop` remains permitted during emergency stop.
   */
  bool allowStopDuringEmergencyStop{true};

  /**
   * @brief True when nonzero robot fault codes reject unsafe commands.
   */
  bool rejectRobotFaults{true};

  /**
   * @brief True when contradictory posture flags reject commands.
   */
  bool enforceMotionStateConsistency{true};

  /**
   * @brief True when base motion requires a standing, non-sitting state.
   */
  bool requireStandingForBaseMotion{true};
};

/**
 * @brief Validates whether a command is safe to forward to a robot adapter.
 *
 * `SafetyValidator` is a policy object. It owns no robot connection, starts no
 * threads, performs no I/O, and contains no vendor-specific logic. Callers
 * provide the current command, latest generic robot state, and generic
 * capability set through dependency injection.
 */
class SafetyValidator final {
public:
  /**
   * @brief Constructs a validator with explicit policy options.
   *
   * @param options Safety policy options.
   */
  explicit constexpr SafetyValidator(SafetyValidatorOptions options = {}) noexcept
      : options_(options) {}

  /**
   * @brief Validates a command against state, capability, and safety policy.
   *
   * @param command Command to validate.
   * @param context Latest state and capability context.
   * @return `Completed` when the command is safe to execute, otherwise a
   * terminal rejection, timeout, or failure result.
   */
  [[nodiscard]] CommandResult Validate(const Command& command,
                                       const SafetyValidationContext& context) const;

  /**
   * @brief Returns the immutable validator options.
   *
   * @return Active safety policy options.
   */
  [[nodiscard]] constexpr const SafetyValidatorOptions& options() const noexcept {
    return options_;
  }

  /**
   * @brief Reports whether a command type drives robot actuators.
   *
   * @param type Command type to inspect.
   * @return True for posture, locomotion, and hand commands.
   */
  [[nodiscard]] static constexpr bool IsActuatorCommand(CommandType type) noexcept {
    switch (type) {
    case CommandType::Stand:
    case CommandType::Sit:
    case CommandType::Walk:
    case CommandType::Move:
    case CommandType::Rotate:
    case CommandType::HandOpen:
    case CommandType::HandClose:
      return true;
    case CommandType::Stop:
    case CommandType::PlayAudio:
    case CommandType::StopAudio:
    case CommandType::Custom:
      return false;
    }

    return false;
  }

  /**
   * @brief Reports whether a command type requests base motion.
   *
   * @param type Command type to inspect.
   * @return True for walking, move, and rotate commands.
   */
  [[nodiscard]] static constexpr bool IsBaseMotionCommand(CommandType type) noexcept {
    switch (type) {
    case CommandType::Walk:
    case CommandType::Move:
    case CommandType::Rotate:
      return true;
    case CommandType::Stand:
    case CommandType::Sit:
    case CommandType::Stop:
    case CommandType::HandOpen:
    case CommandType::HandClose:
    case CommandType::PlayAudio:
    case CommandType::StopAudio:
    case CommandType::Custom:
      return false;
    }

    return false;
  }

private:
  SafetyValidatorOptions options_{};
};

} // namespace humanoid::core
