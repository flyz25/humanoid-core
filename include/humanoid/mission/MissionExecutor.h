#pragma once

/**
 * @file MissionExecutor.h
 * @brief Defines the vendor-independent mission executor.
 */

#include <cstddef>
#include <memory>
#include <optional>

#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionResult.h>
#include <humanoid/mission/MissionStatus.h>

namespace humanoid::core {
class CommandDispatcher;
} // namespace humanoid::core

namespace humanoid::mission {

/**
 * @brief Zero-based index of a mission step in a mission's ordered step list.
 */
using MissionStepIndex = std::size_t;

/**
 * @brief Executes missions by forwarding mission step commands to CommandDispatcher.
 *
 * `MissionExecutor` is vendor independent and owns no robot adapter or SDK
 * object. It does not bypass the command framework: every executable step is
 * translated into a `humanoid::core::Command` and submitted through the
 * injected `humanoid::core::CommandDispatcher`.
 *
 * Lifecycle operations are thread-safe. Pause, cancel, and stop requests do
 * not interrupt a command already executing inside the dispatcher because the
 * command and adapter contracts do not expose a cooperative interruption API.
 * Those requests are applied before the next step or retry starts.
 */
class MissionExecutor final {
public:
  /**
   * @brief Constructs an executor with an injected command dispatcher.
   *
   * A null dispatcher is accepted so composition failures can be reported as
   * failed mission results instead of dereferencing an invalid dependency.
   *
   * @param dispatcher Shared command dispatcher used for step execution.
   */
  explicit MissionExecutor(std::shared_ptr<core::CommandDispatcher> dispatcher);

  /**
   * @brief Stops active work and destroys executor resources.
   */
  ~MissionExecutor() noexcept;

  MissionExecutor(const MissionExecutor&) = delete;
  MissionExecutor& operator=(const MissionExecutor&) = delete;
  MissionExecutor(MissionExecutor&&) = delete;
  MissionExecutor& operator=(MissionExecutor&&) = delete;

  /**
   * @brief Starts asynchronous execution of a mission.
   *
   * @param mission Mission value to execute.
   * @return Running result when accepted, otherwise a failed mission result.
   */
  [[nodiscard]] MissionResult Start(Mission mission);

  /**
   * @brief Requests that execution pause before the next step or retry starts.
   *
   * @return Paused result when the request is accepted.
   */
  [[nodiscard]] MissionResult Pause();

  /**
   * @brief Resumes a paused mission.
   *
   * @return Running result when execution is resumed.
   */
  [[nodiscard]] MissionResult Resume();

  /**
   * @brief Requests mission cancellation.
   *
   * Active dispatcher work is allowed to finish. Queued dispatcher work is
   * cancelled when the dispatcher can still cancel it.
   *
   * @return Cancelled result when the request is accepted.
   */
  [[nodiscard]] MissionResult Cancel();

  /**
   * @brief Requests cancellation and waits for the worker thread to finish.
   *
   * This operation stops executor-owned work only. It does not call
   * `IRobotAdapter::Shutdown()` and does not send robot motion commands.
   *
   * @return Latest terminal mission result.
   */
  [[nodiscard]] MissionResult Stop();

  /**
   * @brief Executes one mission step synchronously through CommandDispatcher.
   *
   * @param step Step to execute.
   * @return Mission result translated from the command result.
   */
  [[nodiscard]] MissionResult ExecuteStep(const MissionStep& step);

  /**
   * @brief Returns the latest mission status.
   *
   * @return Current mission status snapshot.
   */
  [[nodiscard]] MissionStatus GetStatus() const;

  /**
   * @brief Returns the current step index when a step is active or most recent.
   *
   * @return Current zero-based step index, or empty when no step is selected.
   */
  [[nodiscard]] std::optional<MissionStepIndex> GetCurrentStepIndex() const;

  /**
   * @brief Returns the current step identifier when a step is active or recent.
   *
   * @return Current step identifier, or empty when no step is selected.
   */
  [[nodiscard]] std::optional<MissionStepId> GetCurrentStepId() const;

  /**
   * @brief Returns the latest mission result snapshot.
   *
   * @return Last mission result observed by the executor.
   */
  [[nodiscard]] MissionResult GetLastResult() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::mission
