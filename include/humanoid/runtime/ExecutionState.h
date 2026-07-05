#pragma once

/**
 * @file ExecutionState.h
 * @brief Defines shared execution runtime lifecycle states.
 */

#include <cstdint>
#include <string_view>

namespace humanoid::runtime {

/**
 * @brief Represents the lifecycle state of a runtime execution.
 */
enum class ExecutionState : std::uint8_t {
  Created,   ///< The context exists but execution has not started.
  Starting,  ///< Execution startup is in progress.
  Running,   ///< Execution is actively progressing.
  Paused,    ///< Execution is suspended and may resume.
  Completed, ///< Execution completed successfully.
  Cancelled, ///< Execution ended after a cancellation request.
  Failed,    ///< Execution ended because an operation failed.
  Aborted    ///< Execution was terminated by policy or safety action.
};

/**
 * @brief Returns the stable name of an execution state.
 *
 * @param state State to inspect.
 * @return Non-owning stable state name.
 */
[[nodiscard]] constexpr std::string_view toString(ExecutionState state) noexcept {
  switch (state) {
  case ExecutionState::Created:
    return "Created";
  case ExecutionState::Starting:
    return "Starting";
  case ExecutionState::Running:
    return "Running";
  case ExecutionState::Paused:
    return "Paused";
  case ExecutionState::Completed:
    return "Completed";
  case ExecutionState::Cancelled:
    return "Cancelled";
  case ExecutionState::Failed:
    return "Failed";
  case ExecutionState::Aborted:
    return "Aborted";
  }

  return "Unknown";
}

/**
 * @brief Reports whether an execution state is terminal.
 *
 * @param state State to inspect.
 * @return True when execution cannot progress from the state.
 */
[[nodiscard]] constexpr bool isTerminal(ExecutionState state) noexcept {
  switch (state) {
  case ExecutionState::Completed:
  case ExecutionState::Cancelled:
  case ExecutionState::Failed:
  case ExecutionState::Aborted:
    return true;
  case ExecutionState::Created:
  case ExecutionState::Starting:
  case ExecutionState::Running:
  case ExecutionState::Paused:
    return false;
  }

  return false;
}

} // namespace humanoid::runtime
