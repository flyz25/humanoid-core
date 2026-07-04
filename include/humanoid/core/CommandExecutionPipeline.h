#pragma once

/**
 * @file CommandExecutionPipeline.h
 * @brief Defines vendor-independent command execution lifecycle infrastructure.
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/CommandStatus.h>

namespace humanoid::logging {
class ILogger;
} // namespace humanoid::logging

namespace humanoid::core {

/**
 * @brief Unique identifier assigned by a command execution pipeline.
 */
using CommandExecutionId = std::uint64_t;

/**
 * @brief Subscription identifier assigned to execution lifecycle callbacks.
 */
using CommandExecutionCallbackId = std::uint64_t;

/**
 * @brief Monotonic timestamp used for execution lifecycle events.
 */
using CommandExecutionTimestamp = CommandTimestamp;

/**
 * @brief Construction options for `CommandExecutionPipeline`.
 */
struct CommandExecutionPipelineOptions final {
  /**
   * @brief Maximum number of commands waiting for worker execution.
   */
  std::size_t maximumQueueSize{1024U};

  /**
   * @brief Number of worker threads that may execute commands concurrently.
   */
  std::size_t workerCount{1U};

  /**
   * @brief Maximum completed execution records retained in history.
   *
   * Active executions are retained even if the total record count temporarily
   * exceeds this value.
   */
  std::size_t maximumHistorySize{1024U};
};

/**
 * @brief Lifecycle event emitted by the execution pipeline.
 */
struct CommandExecutionEvent final {
  /**
   * @brief Pipeline-assigned execution identifier.
   */
  CommandExecutionId executionId{0U};

  /**
   * @brief Producer-assigned command identifier.
   */
  CommandId commandId{0U};

  /**
   * @brief Command type associated with the execution.
   */
  CommandType commandType{CommandType::Custom};

  /**
   * @brief Lifecycle status represented by this event.
   */
  CommandStatus status{CommandStatus::Pending};

  /**
   * @brief Monotonic event timestamp.
   */
  CommandExecutionTimestamp timestamp{};

  /**
   * @brief Result snapshot associated with the event.
   */
  CommandResult result{};
};

/**
 * @brief Execution history record retained by the pipeline.
 */
struct CommandExecutionRecord final {
  /**
   * @brief Pipeline-assigned execution identifier.
   */
  CommandExecutionId executionId{0U};

  /**
   * @brief Producer-assigned command identifier.
   */
  CommandId commandId{0U};

  /**
   * @brief Command type submitted for execution.
   */
  CommandType commandType{CommandType::Custom};

  /**
   * @brief Command priority used for queue selection.
   */
  CommandPriority priority{CommandPriority::Normal};

  /**
   * @brief Latest known lifecycle status.
   */
  CommandStatus status{CommandStatus::Pending};

  /**
   * @brief Time at which the pipeline accepted or rejected the execution.
   */
  CommandExecutionTimestamp submittedAt{};

  /**
   * @brief Time at which the execution entered the queue.
   */
  std::optional<CommandExecutionTimestamp> queuedAt;

  /**
   * @brief Time at which a worker started executor invocation.
   */
  std::optional<CommandExecutionTimestamp> startedAt;

  /**
   * @brief Time at which the execution reached a terminal status.
   */
  std::optional<CommandExecutionTimestamp> completedAt;

  /**
   * @brief Latest or final command result.
   */
  CommandResult result{};
};

/**
 * @brief Thread-safe snapshot of execution pipeline metrics.
 */
struct CommandExecutionMetrics final {
  /**
   * @brief Commands currently waiting for a worker.
   */
  std::size_t currentQueued{0U};

  /**
   * @brief Commands currently running in executor callbacks.
   */
  std::size_t currentRunning{0U};

  /**
   * @brief Configured maximum number of waiting commands.
   */
  std::size_t maximumQueueSize{0U};

  /**
   * @brief Configured worker count.
   */
  std::size_t workerCount{0U};

  /**
   * @brief Configured completed-history retention limit.
   */
  std::size_t maximumHistorySize{0U};

  /**
   * @brief Current number of retained execution records.
   */
  std::size_t historySize{0U};

  /**
   * @brief Largest observed waiting-queue depth.
   */
  std::size_t highWatermark{0U};

  /**
   * @brief Largest observed concurrent running count.
   */
  std::size_t maximumConcurrentRunning{0U};

  /**
   * @brief Executions accepted into the queue.
   */
  std::uint64_t accepted{0U};

  /**
   * @brief Executions that reached `Running`.
   */
  std::uint64_t started{0U};

  /**
   * @brief Executions that completed successfully.
   */
  std::uint64_t completed{0U};

  /**
   * @brief Executions cancelled before worker start.
   */
  std::uint64_t cancelled{0U};

  /**
   * @brief Executions that timed out before or during execution.
   */
  std::uint64_t timedOut{0U};

  /**
   * @brief Executions that failed through executor result or exception.
   */
  std::uint64_t failed{0U};

  /**
   * @brief Executions rejected before entering the queue.
   */
  std::uint64_t rejected{0U};

  /**
   * @brief Callback invocations that threw and were contained.
   */
  std::uint64_t callbackFailures{0U};
};

/**
 * @brief Handle returned when submitting a command for execution.
 */
struct CommandExecutionHandle final {
  /**
   * @brief Pipeline-assigned execution identifier.
   */
  CommandExecutionId executionId{0U};

  /**
   * @brief Producer-assigned command identifier.
   */
  CommandId commandId{0U};

  /**
   * @brief Future containing the terminal execution result.
   */
  std::future<CommandResult> result;
};

/**
 * @brief Vendor-independent command execution lifecycle pipeline.
 *
 * The pipeline owns scheduling, lifecycle transitions, callback publication,
 * optional logging, metrics, and bounded history. It delegates command-specific
 * work to an injected executor and contains no SDK, robot adapter, mission, or
 * business logic.
 *
 * All public operations are thread-safe. Callback invocations are performed
 * outside internal locks. Running executor callbacks are not interrupted by
 * cancellation or shutdown; cancellation applies only before worker start.
 */
class CommandExecutionPipeline final {
public:
  /**
   * @brief Callable used by workers to execute one command.
   */
  using Executor = std::function<CommandResult(const Command&)>;

  /**
   * @brief Callback type for lifecycle events.
   */
  using Callback = std::function<void(const CommandExecutionEvent&)>;

  /**
   * @brief Invalid callback identifier value.
   */
  static constexpr CommandExecutionCallbackId kInvalidCallbackId{0U};

  /**
   * @brief Constructs and starts an execution pipeline.
   *
   * @param executor Command executor invoked outside pipeline locks.
   * @param options Queue, worker, and history limits.
   * @param logger Optional logger used for lifecycle events.
   * @throws std::invalid_argument when executor, queue size, worker count, or
   * history size is invalid.
   * @throws std::system_error if worker creation fails.
   */
  explicit CommandExecutionPipeline(Executor executor, CommandExecutionPipelineOptions options = {},
                                    std::shared_ptr<logging::ILogger> logger = nullptr);

  /**
   * @brief Cancels queued work and joins worker threads.
   */
  ~CommandExecutionPipeline() noexcept;

  CommandExecutionPipeline(const CommandExecutionPipeline&) = delete;
  CommandExecutionPipeline& operator=(const CommandExecutionPipeline&) = delete;
  CommandExecutionPipeline(CommandExecutionPipeline&&) = delete;
  CommandExecutionPipeline& operator=(CommandExecutionPipeline&&) = delete;

  /**
   * @brief Submits a command for lifecycle-managed asynchronous execution.
   *
   * Invalid, duplicate, expired, full-queue, or post-shutdown submissions return
   * a handle with a ready future containing a terminal result.
   *
   * @param command Command value to execute.
   * @return Execution handle containing the execution ID and final-result future.
   */
  [[nodiscard]] CommandExecutionHandle Submit(Command command);

  /**
   * @brief Cancels an execution that is still queued.
   *
   * @param execution_id Execution identifier returned by `Submit()`.
   * @return `Cancelled` when queued work was cancelled, otherwise `Rejected`.
   */
  [[nodiscard]] CommandResult Cancel(CommandExecutionId execution_id);

  /**
   * @brief Stops accepting work, cancels queued executions, and joins workers.
   *
   * Active executor callbacks are allowed to finish. The operation is
   * idempotent. Calls made from a pipeline worker are rejected to prevent a
   * worker joining itself.
   *
   * @return Completed result after shutdown is complete.
   */
  [[nodiscard]] CommandResult Shutdown();

  /**
   * @brief Registers a lifecycle callback.
   *
   * @param callback Callback invoked outside pipeline locks.
   * @return Subscription ID, or `kInvalidCallbackId` when callback is empty.
   */
  [[nodiscard]] CommandExecutionCallbackId Subscribe(Callback callback);

  /**
   * @brief Removes a lifecycle callback subscription.
   *
   * @param callback_id Subscription ID returned by `Subscribe()`.
   * @return True when a callback was removed.
   */
  [[nodiscard]] bool Unsubscribe(CommandExecutionCallbackId callback_id);

  /**
   * @brief Returns a consistent metrics snapshot.
   *
   * @return Current and cumulative execution metrics.
   */
  [[nodiscard]] CommandExecutionMetrics GetMetrics() const;

  /**
   * @brief Returns retained execution records in insertion order.
   *
   * @return Snapshot of bounded execution history.
   */
  [[nodiscard]] std::vector<CommandExecutionRecord> GetHistory() const;

  /**
   * @brief Looks up one retained execution record.
   *
   * @param execution_id Execution identifier to inspect.
   * @return Execution record when retained.
   */
  [[nodiscard]] std::optional<CommandExecutionRecord>
  GetRecord(CommandExecutionId execution_id) const;

  /**
   * @brief Reports whether an execution is queued or running.
   *
   * @param execution_id Execution identifier to inspect.
   * @return True when execution has not reached a terminal status.
   */
  [[nodiscard]] bool Contains(CommandExecutionId execution_id) const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::core
