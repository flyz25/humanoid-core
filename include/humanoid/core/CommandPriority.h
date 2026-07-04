#pragma once

/**
 * @file CommandPriority.h
 * @brief Defines vendor-independent command priorities.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::core {

/**
 * @brief Indicates the relative scheduling importance of a command.
 *
 * Priority is descriptive metadata for a future command-processing component.
 * It does not itself authorize execution or bypass safety policy.
 */
enum class CommandPriority : std::uint8_t {
  Low,     ///< Deferrable work with no immediate timing requirement.
  Normal,  ///< Standard operational priority.
  High,    ///< Time-sensitive work that should precede normal work.
  Critical ///< Safety-critical work requiring the earliest permitted handling.
};

/**
 * @brief Returns the stable name of a command priority.
 *
 * @param priority Command priority to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(CommandPriority priority) noexcept {
  switch (priority) {
  case CommandPriority::Low:
    return "Low";
  case CommandPriority::Normal:
    return "Normal";
  case CommandPriority::High:
    return "High";
  case CommandPriority::Critical:
    return "Critical";
  }

  return "Unknown";
}

} // namespace humanoid::core
