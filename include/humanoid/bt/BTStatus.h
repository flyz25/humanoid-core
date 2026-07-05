#pragma once

/**
 * @file BTStatus.h
 * @brief Defines behavior tree node and tree execution statuses.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::bt {

/**
 * @brief Represents the lifecycle and tick result of a behavior tree node.
 */
enum class BTStatus : std::uint8_t {
  Idle,    ///< The node or tree is initialized but not currently executing.
  Running, ///< The node or tree needs another tick to finish.
  Success, ///< The node or tree completed successfully.
  Failure, ///< The node or tree completed with a handled failure.
  Aborted  ///< The node or tree was terminated by cancellation or policy.
};

/**
 * @brief Returns a stable display name for a behavior tree status.
 *
 * @param status Status to inspect.
 * @return Non-owning stable status name.
 */
[[nodiscard]] constexpr std::string_view toString(BTStatus status) noexcept {
  switch (status) {
  case BTStatus::Idle:
    return "Idle";
  case BTStatus::Running:
    return "Running";
  case BTStatus::Success:
    return "Success";
  case BTStatus::Failure:
    return "Failure";
  case BTStatus::Aborted:
    return "Aborted";
  }

  return "Unknown";
}

/**
 * @brief Reports whether a behavior tree status is terminal.
 *
 * @param status Status to inspect.
 * @return True when no further ticks should be issued without reset.
 */
[[nodiscard]] constexpr bool isTerminal(BTStatus status) noexcept {
  switch (status) {
  case BTStatus::Success:
  case BTStatus::Failure:
  case BTStatus::Aborted:
    return true;
  case BTStatus::Idle:
  case BTStatus::Running:
    return false;
  }

  return false;
}

} // namespace humanoid::bt
