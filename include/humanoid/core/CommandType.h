#pragma once

/**
 * @file CommandType.h
 * @brief Defines vendor-independent robot command categories.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::core {

/**
 * @brief Identifies the operation requested by a generic robot command.
 *
 * Command types describe framework-level intent only. Concrete adapters are
 * responsible for determining whether a command is supported and translating
 * it without exposing vendor-specific types to the core framework.
 */
enum class CommandType : std::uint8_t {
  Stand,         ///< Request an upright standing posture.
  Sit,           ///< Request a seated posture.
  Walk,          ///< Request walking using payload-defined parameters.
  Stop,          ///< Request that active robot motion stop.
  Move,          ///< Request translational motion using payload-defined parameters.
  Rotate,        ///< Request rotational motion using payload-defined parameters.
  Velocity,      ///< Request direct velocity control using payload-defined parameters.
  EmergencyStop, ///< Request emergency stop handling.
  HandOpen,      ///< Request that a supported hand open.
  HandClose,     ///< Request that a supported hand close.
  Gesture,       ///< Request a named hand or upper-body gesture.
  PlayAudio,     ///< Request audio playback using payload-defined parameters.
  StopAudio,     ///< Request that active audio playback stop.
  SetVolume,     ///< Request audio volume change using payload-defined parameters.
  MuteAudio,     ///< Request audio mute.
  Custom         ///< Request an extension command defined above the vendor boundary.
};

/**
 * @brief Returns the stable name of a command type.
 *
 * @param type Command type to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(CommandType type) noexcept {
  switch (type) {
  case CommandType::Stand:
    return "Stand";
  case CommandType::Sit:
    return "Sit";
  case CommandType::Walk:
    return "Walk";
  case CommandType::Stop:
    return "Stop";
  case CommandType::Move:
    return "Move";
  case CommandType::Rotate:
    return "Rotate";
  case CommandType::Velocity:
    return "Velocity";
  case CommandType::EmergencyStop:
    return "EmergencyStop";
  case CommandType::HandOpen:
    return "HandOpen";
  case CommandType::HandClose:
    return "HandClose";
  case CommandType::Gesture:
    return "Gesture";
  case CommandType::PlayAudio:
    return "PlayAudio";
  case CommandType::StopAudio:
    return "StopAudio";
  case CommandType::SetVolume:
    return "SetVolume";
  case CommandType::MuteAudio:
    return "MuteAudio";
  case CommandType::Custom:
    return "Custom";
  }

  return "Unknown";
}

} // namespace humanoid::core
