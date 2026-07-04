#pragma once

/**
 * @file CommandStatus.h
 * @brief Defines vendor-independent command lifecycle states.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::core {

/**
 * @brief Represents the lifecycle state of a command.
 */
enum class CommandStatus : std::uint8_t {
  Pending,   ///< The command has been created but not submitted.
  Queued,    ///< The command has been accepted for later execution.
  Running,   ///< The command is currently executing.
  Completed, ///< The command completed successfully.
  Cancelled, ///< The command was cancelled before successful completion.
  Failed,    ///< The command terminated because execution failed.
  Timeout,   ///< The command exceeded its permitted execution duration.
  Rejected   ///< The command was not accepted for execution.
};

/**
 * @brief Returns the stable name of a command status.
 *
 * @param status Command status to inspect.
 * @return Non-owning string representation suitable for logs and diagnostics.
 */
[[nodiscard]] constexpr std::string_view toString(CommandStatus status) noexcept {
  switch (status) {
  case CommandStatus::Pending:
    return "Pending";
  case CommandStatus::Queued:
    return "Queued";
  case CommandStatus::Running:
    return "Running";
  case CommandStatus::Completed:
    return "Completed";
  case CommandStatus::Cancelled:
    return "Cancelled";
  case CommandStatus::Failed:
    return "Failed";
  case CommandStatus::Timeout:
    return "Timeout";
  case CommandStatus::Rejected:
    return "Rejected";
  }

  return "Unknown";
}

/**
 * @brief Reports whether a command status is terminal.
 *
 * @param status Command status to inspect.
 * @return True when no further lifecycle transition is expected.
 */
[[nodiscard]] constexpr bool isTerminal(CommandStatus status) noexcept {
  switch (status) {
  case CommandStatus::Completed:
  case CommandStatus::Cancelled:
  case CommandStatus::Failed:
  case CommandStatus::Timeout:
  case CommandStatus::Rejected:
    return true;
  case CommandStatus::Pending:
  case CommandStatus::Queued:
  case CommandStatus::Running:
    return false;
  }

  return false;
}

} // namespace humanoid::core
