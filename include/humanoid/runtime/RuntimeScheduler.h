#pragma once

/**
 * @file RuntimeScheduler.h
 * @brief Defines vendor-independent runtime execution scheduling.
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

#include <humanoid/runtime/Cancellation.h>
#include <humanoid/runtime/ExecutionContext.h>
#include <humanoid/runtime/ExecutionMetadata.h>
#include <humanoid/runtime/ExecutionScope.h>
#include <humanoid/runtime/ExecutionState.h>

namespace humanoid::runtime {

namespace detail {
class RuntimeJobControl;
} // namespace detail

/**
 * @brief Producer-assigned runtime job identifier.
 */
using RuntimeJobId = std::uint64_t;

/**
 * @brief Scheduler-assigned monotonically increasing sequence number.
 */
using RuntimeJobSequence = std::uint64_t;

/**
 * @brief Monotonic timestamp used for runtime scheduling events.
 */
using RuntimeSchedulerTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Runtime scheduling priority.
 */
enum class RuntimeJobPriority : std::uint8_t {
  /** @brief Lowest scheduling priority. */
  Low,

  /** @brief Default scheduling priority. */
  Normal,

  /** @brief High scheduling priority. */
  High,

  /** @brief Highest scheduling priority for urgent runtime work. */
  Critical
};

/**
 * @brief Runtime execution isolation mode.
 */
enum class RuntimeExecutionMode : std::uint8_t {
  /**
   * @brief Job may execute concurrently with other parallel jobs.
   */
  Parallel,

  /**
   * @brief Job requires exclusive scheduler execution.
   */
  Sequential
};

/**
 * @brief Result returned by a runtime job callback.
 */
struct RuntimeJobResult final {
  /** @brief Terminal execution state selected by the job or scheduler. */
  ExecutionState state{ExecutionState::Completed};

  /** @brief Human-readable diagnostic message. */
  std::string message;
};

/**
 * @brief Context passed to scheduler-owned runtime job callbacks.
 *
 * The context exposes only generic runtime state, cooperative cancellation, and
 * cooperative pause waiting. It contains no mission, behavior-tree, adapter, or
 * SDK dependencies.
 */
class RuntimeJobContext final {
public:
  /** @brief Constructs an empty invalid context. */
  RuntimeJobContext() = default;

  /**
   * @brief Returns the producer-assigned job identifier.
   *
   * @return Runtime job id, or zero for an invalid context.
   */
  [[nodiscard]] RuntimeJobId JobId() const noexcept;

  /**
   * @brief Returns the runtime execution context for this job.
   *
   * @return Shared execution context.
   * @throws std::logic_error if the context is invalid.
   */
  [[nodiscard]] ExecutionContext& Execution() const;

  /**
   * @brief Returns shared ownership of the runtime execution context.
   *
   * The returned aliasing pointer keeps the scheduler job control alive. It is
   * intended for execution engines whose contexts require shared ownership.
   *
   * @return Shared execution context.
   * @throws std::logic_error if the context is invalid.
   */
  [[nodiscard]] std::shared_ptr<ExecutionContext> SharedExecution() const;

  /**
   * @brief Returns a cooperative cancellation token for this job.
   *
   * @return Runtime cancellation token.
   */
  [[nodiscard]] CancellationToken Cancellation() const noexcept;

  /**
   * @brief Reports whether cooperative cancellation has been requested.
   *
   * @return True after the scheduler stops the job or the token is cancelled.
   */
  [[nodiscard]] bool IsCancellationRequested() const noexcept;

  /**
   * @brief Reports whether this job is currently paused.
   *
   * @return True while a pause request is active.
   */
  [[nodiscard]] bool IsPaused() const noexcept;

  /**
   * @brief Waits cooperatively while this job is paused.
   *
   * The wait returns when the job resumes or cancellation is requested.
   *
   * @return False when cancellation is requested, true otherwise.
   */
  bool WaitIfPaused() const;

private:
  friend class RuntimeScheduler;

  explicit RuntimeJobContext(std::shared_ptr<detail::RuntimeJobControl> control);

  std::shared_ptr<detail::RuntimeJobControl> control_;
};

/**
 * @brief Callable executed by scheduler workers.
 */
using RuntimeJobCallback = std::function<RuntimeJobResult(RuntimeJobContext&)>;

/**
 * @brief Runtime job submitted to the scheduler.
 */
struct RuntimeJob final {
  /** @brief Producer-assigned nonzero job identifier. */
  RuntimeJobId id{0U};

  /** @brief Scheduling priority. */
  RuntimeJobPriority priority{RuntimeJobPriority::Normal};

  /** @brief Parallel or sequential execution mode. */
  RuntimeExecutionMode executionMode{RuntimeExecutionMode::Parallel};

  /** @brief Framework subsystem represented by this execution. */
  ExecutionScope scope{ExecutionScope::Unknown};

  /** @brief Optional non-operational runtime metadata. */
  ExecutionMetadata metadata;

  /** @brief Callback invoked by a scheduler worker. */
  RuntimeJobCallback callback;
};

/**
 * @brief Handle returned when a runtime job is submitted.
 */
struct RuntimeJobHandle final {
  /** @brief Producer-assigned runtime job identifier. */
  RuntimeJobId jobId{0U};

  /** @brief Future containing the terminal job result. */
  std::shared_future<RuntimeJobResult> result;
};

/**
 * @brief Scheduler construction options.
 */
struct RuntimeSchedulerOptions final {
  /** @brief Maximum number of jobs waiting for execution. */
  std::size_t maximumQueueSize{1024U};

  /** @brief Number of worker threads allowed to execute runtime jobs. */
  std::size_t workerCount{1U};

  /** @brief Maximum retained completed job snapshots. */
  std::size_t maximumCompletedHistorySize{1024U};
};

/**
 * @brief Scheduler metrics snapshot.
 */
struct RuntimeSchedulerStatistics final {
  /** @brief Jobs currently waiting for workers. */
  std::size_t queued{0U};

  /** @brief Jobs currently running in worker callbacks. */
  std::size_t running{0U};

  /** @brief Jobs currently paused while queued or running. */
  std::size_t paused{0U};

  /** @brief Configured maximum waiting queue size. */
  std::size_t maximumQueueSize{0U};

  /** @brief Configured worker count. */
  std::size_t workerCount{0U};

  /** @brief Largest observed waiting queue size. */
  std::size_t highWatermark{0U};

  /** @brief Largest observed concurrent running count. */
  std::size_t maximumConcurrentRunning{0U};

  /** @brief Jobs accepted by the scheduler. */
  std::uint64_t accepted{0U};

  /** @brief Jobs started by workers. */
  std::uint64_t started{0U};

  /** @brief Jobs completed successfully. */
  std::uint64_t completed{0U};

  /** @brief Jobs cancelled before or during execution. */
  std::uint64_t cancelled{0U};

  /** @brief Jobs stopped or aborted by scheduler request. */
  std::uint64_t stopped{0U};

  /** @brief Jobs failed or whose callbacks threw. */
  std::uint64_t failed{0U};

  /** @brief Jobs rejected before queue acceptance. */
  std::uint64_t rejected{0U};
};

/**
 * @brief Snapshot of a runtime job lifecycle.
 */
struct RuntimeJobSnapshot final {
  /** @brief Producer-assigned runtime job identifier. */
  RuntimeJobId jobId{0U};

  /** @brief Scheduler-assigned sequence number. */
  RuntimeJobSequence sequence{0U};

  /** @brief Job priority used for scheduling. */
  RuntimeJobPriority priority{RuntimeJobPriority::Normal};

  /** @brief Parallel or sequential execution mode. */
  RuntimeExecutionMode executionMode{RuntimeExecutionMode::Parallel};

  /** @brief Latest lifecycle state. */
  ExecutionState state{ExecutionState::Created};

  /** @brief Timestamp when the job was accepted. */
  RuntimeSchedulerTimestamp queuedAt{};

  /** @brief Timestamp when a worker started the job. */
  std::optional<RuntimeSchedulerTimestamp> startedAt;

  /** @brief Timestamp when the job reached a terminal state. */
  std::optional<RuntimeSchedulerTimestamp> completedAt;

  /** @brief Latest or terminal job result. */
  RuntimeJobResult result{};
};

/**
 * @brief Thread-safe vendor-independent scheduler for runtime executions.
 *
 * The scheduler owns queueing, priority selection, sequential-vs-parallel
 * dispatch, lifecycle tracking, cooperative pause/resume, cooperative stop, and
 * worker shutdown. It executes injected callbacks only and contains no mission,
 * behavior-tree, robot adapter, plugin, SDK, ROS2, planner, or AI logic.
 */
class RuntimeScheduler final {
public:
  /**
   * @brief Constructs and starts scheduler worker threads.
   *
   * @param options Queue capacity, workers, and history retention.
   * @throws std::invalid_argument if options are invalid.
   * @throws std::system_error if workers cannot be created.
   */
  explicit RuntimeScheduler(RuntimeSchedulerOptions options = {});

  /** @brief Stops the scheduler and joins workers. */
  ~RuntimeScheduler() noexcept;

  RuntimeScheduler(const RuntimeScheduler&) = delete;
  RuntimeScheduler& operator=(const RuntimeScheduler&) = delete;
  RuntimeScheduler(RuntimeScheduler&&) = delete;
  RuntimeScheduler& operator=(RuntimeScheduler&&) = delete;

  /**
   * @brief Queues a runtime job for execution.
   *
   * Invalid, duplicate, full-queue, or post-shutdown submissions return a ready
   * failed result.
   *
   * @param job Runtime job to enqueue.
   * @return Handle containing the job id and terminal result future.
   */
  [[nodiscard]] RuntimeJobHandle Submit(RuntimeJob job);

  /**
   * @brief Requests cooperative pause for a queued or running job.
   *
   * Running job callbacks must call `RuntimeJobContext::WaitIfPaused()` or
   * inspect `IsPaused()` to cooperate.
   *
   * @param job_id Runtime job identifier.
   * @return True when a non-terminal job was paused.
   */
  bool Pause(RuntimeJobId job_id);

  /**
   * @brief Resumes a paused queued or running job.
   *
   * @param job_id Runtime job identifier.
   * @return True when a paused non-terminal job was resumed.
   */
  bool Resume(RuntimeJobId job_id);

  /**
   * @brief Requests cooperative stop for a queued or running job.
   *
   * Queued jobs complete immediately as cancelled. Running jobs receive a
   * cancellation request and complete when their callback returns.
   *
   * @param job_id Runtime job identifier.
   * @return True when an outstanding job was stopped or cancellation requested.
   */
  bool Stop(RuntimeJobId job_id);

  /**
   * @brief Stops accepting jobs, cancels queued jobs, and joins workers.
   *
   * Running callbacks receive cancellation requests and are allowed to return.
   * The operation is idempotent.
   */
  void Shutdown() noexcept;

  /**
   * @brief Returns a job snapshot by id.
   *
   * @param job_id Runtime job identifier.
   * @return Snapshot when retained.
   */
  [[nodiscard]] std::optional<RuntimeJobSnapshot> GetSnapshot(RuntimeJobId job_id) const;

  /**
   * @brief Returns retained job snapshots.
   *
   * @return Snapshot vector ordered by internal map iteration.
   */
  [[nodiscard]] std::vector<RuntimeJobSnapshot> GetSnapshots() const;

  /**
   * @brief Returns scheduler metrics.
   *
   * @return Consistent scheduler statistics snapshot.
   */
  [[nodiscard]] RuntimeSchedulerStatistics GetStatistics() const;

  /**
   * @brief Returns the number of jobs waiting for workers.
   *
   * @return Current queue size.
   */
  [[nodiscard]] std::size_t Size() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::runtime
