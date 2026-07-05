#pragma once

/**
 * @file MissionStatus.h
 * @brief Defines vendor-independent mission lifecycle states.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::mission {

/**
 * @brief Represents the lifecycle state of a mission.
 */
enum class MissionStatus : std::uint8_t {
  Pending,   ///< The mission has been defined but has not started.
  Running,   ///< The mission is actively executing.
  Paused,    ///< The mission is temporarily suspended and may resume.
  Completed, ///< The mission completed successfully.
  Failed,    ///< The mission terminated because execution failed.
  Cancelled  ///< The mission was cancelled before successful completion.
};

/**
 * @brief Returns the stable name of a mission status.
 *
 * @param status Mission status to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(MissionStatus status) noexcept {
  switch (status) {
  case MissionStatus::Pending:
    return "Pending";
  case MissionStatus::Running:
    return "Running";
  case MissionStatus::Paused:
    return "Paused";
  case MissionStatus::Completed:
    return "Completed";
  case MissionStatus::Failed:
    return "Failed";
  case MissionStatus::Cancelled:
    return "Cancelled";
  }

  return "Unknown";
}

/**
 * @brief Reports whether a mission status is terminal.
 *
 * @param status Mission status to inspect.
 * @return True when no further lifecycle transition is expected.
 */
[[nodiscard]] constexpr bool isTerminal(MissionStatus status) noexcept {
  switch (status) {
  case MissionStatus::Completed:
  case MissionStatus::Failed:
  case MissionStatus::Cancelled:
    return true;
  case MissionStatus::Pending:
  case MissionStatus::Running:
  case MissionStatus::Paused:
    return false;
  }

  return false;
}

} // namespace humanoid::mission
