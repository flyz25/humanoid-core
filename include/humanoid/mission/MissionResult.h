#pragma once

/**
 * @file MissionResult.h
 * @brief Defines the vendor-independent result of mission processing.
 */

#include <string>

#include <humanoid/mission/MissionStatus.h>

namespace humanoid::mission {

/**
 * @brief Describes the latest or final outcome of a mission.
 *
 * A result contains framework-owned types only. Diagnostic messages must not
 * expose vendor handles, SDK objects, credentials, or transport internals.
 */
struct MissionResult final {
  /**
   * @brief Lifecycle status represented by this result.
   */
  MissionStatus status{MissionStatus::Pending};

  /**
   * @brief Optional human-readable diagnostic information.
   */
  std::string message;

  /**
   * @brief Constructs a pending mission result with no diagnostic message.
   */
  MissionResult() = default;

  /**
   * @brief Reports whether the mission completed successfully.
   *
   * @return True only when status is `MissionStatus::Completed`.
   */
  [[nodiscard]] constexpr bool isSuccess() const noexcept {
    return status == MissionStatus::Completed;
  }

  /**
   * @brief Reports whether the result represents a terminal mission state.
   *
   * @return True when no further lifecycle transition is expected.
   */
  [[nodiscard]] constexpr bool isTerminal() const noexcept {
    return humanoid::mission::isTerminal(status);
  }
};

} // namespace humanoid::mission
