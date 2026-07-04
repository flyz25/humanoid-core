#pragma once

/**
 * @file CommandResult.h
 * @brief Defines the vendor-independent result of command processing.
 */

#include <string>

#include <humanoid/core/CommandStatus.h>

namespace humanoid::core {

/**
 * @brief Describes the latest or final outcome of a command.
 *
 * A result contains only framework-owned types. Diagnostic messages must not
 * expose vendor handles, SDK objects, credentials, or transport internals.
 */
struct CommandResult final {
  /**
   * @brief Lifecycle status represented by this result.
   */
  CommandStatus status{CommandStatus::Pending};

  /**
   * @brief Optional human-readable diagnostic information.
   */
  std::string message;

  /**
   * @brief Constructs a pending command result with no diagnostic message.
   */
  CommandResult() = default;

  /**
   * @brief Reports whether command execution completed successfully.
   *
   * @return True only when status is `CommandStatus::Completed`.
   */
  [[nodiscard]] constexpr bool isSuccess() const noexcept {
    return status == CommandStatus::Completed;
  }

  /**
   * @brief Reports whether the result represents a terminal command state.
   *
   * @return True when no further lifecycle transition is expected.
   */
  [[nodiscard]] constexpr bool isTerminal() const noexcept {
    return humanoid::core::isTerminal(status);
  }
};

} // namespace humanoid::core
