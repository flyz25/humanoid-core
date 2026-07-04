#pragma once

/**
 * @file CommandQueue.h
 * @brief Defines the bounded asynchronous command queue.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>

#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>

namespace humanoid::core {

/**
 * @brief Construction options for `CommandQueue`.
 */
struct CommandQueueOptions final {
  /**
   * @brief Maximum number of commands waiting for a worker.
   */
  std::size_t maximumQueueSize{1024U};

  /**
   * @brief Number of command consumer workers.
   */
  std::size_t workerCount{1U};
};

/**
 * @brief Thread-safe snapshot of command queue activity.
 */
struct CommandQueueStatistics final {
  /**
   * @brief Commands currently waiting for a worker.
   */
  std::size_t queued{0U};

  /**
   * @brief Commands currently executing in workers.
   */
  std::size_t active{0U};

  /**
   * @brief Configured maximum number of waiting commands.
   */
  std::size_t maximumQueueSize{0U};

  /**
   * @brief Configured number of consumer workers.
   */
  std::size_t workerCount{0U};

  /**
   * @brief Largest observed number of waiting commands.
   */
  std::size_t highWatermark{0U};

  /**
   * @brief Commands successfully accepted into the queue.
   */
  std::uint64_t accepted{0U};

  /**
   * @brief Commands completed successfully by the executor.
   */
  std::uint64_t completed{0U};

  /**
   * @brief Commands cancelled before execution.
   */
  std::uint64_t cancelled{0U};

  /**
   * @brief Commands that completed with failure.
   */
  std::uint64_t failed{0U};

  /**
   * @brief Commands that expired before or during execution.
   */
  std::uint64_t timedOut{0U};

  /**
   * @brief Commands rejected before entering the queue or by the executor.
   */
  std::uint64_t rejected{0U};
};

/**
 * @brief Bounded, priority-aware asynchronous command execution queue.
 *
 * Commands are dequeued by descending `CommandPriority`. Commands with equal
 * priority are dequeued in FIFO order. The queue owns one or more cooperative
 * `std::jthread` workers and sleeps on a condition variable when no work is
 * available.
 *
 * The injected executor defines command-specific validation and forwarding.
 * The queue owns scheduling, timeout, cancellation, capacity, lifecycle, and
 * statistics only. It contains no adapter, SDK, mission, or business logic.
 *
 * All public operations are thread-safe. Cancellation is guaranteed only for
 * queued commands; an executor invocation that has started is not interrupted.
 */
class CommandQueue final {
public:
  /**
   * @brief Callable used by workers to execute one command.
   */
  using Executor = std::function<CommandResult(const Command&)>;

  /**
   * @brief Constructs and starts a command queue.
   *
   * @param executor Command executor invoked outside queue locks.
   * @param options Queue capacity and worker count.
   * @throws std::invalid_argument if executor, capacity, or worker count is invalid.
   * @throws std::system_error if a worker thread cannot be created.
   */
  explicit CommandQueue(Executor executor, CommandQueueOptions options = {});

  /**
   * @brief Cancels queued work and joins all worker threads.
   */
  ~CommandQueue() noexcept;

  CommandQueue(const CommandQueue&) = delete;
  CommandQueue& operator=(const CommandQueue&) = delete;
  CommandQueue(CommandQueue&&) = delete;
  CommandQueue& operator=(CommandQueue&&) = delete;

  /**
   * @brief Queues a command for asynchronous execution.
   *
   * Invalid, duplicate, expired, full-queue, or post-shutdown submissions
   * return a ready future containing a terminal result.
   *
   * @param command Command value to enqueue.
   * @return Future containing the terminal execution result.
   */
  [[nodiscard]] std::future<CommandResult> Enqueue(Command command);

  /**
   * @brief Cancels a command that is still waiting in the queue.
   *
   * @param command_id Identifier of the command to cancel.
   * @return `Cancelled` when removed, otherwise `Rejected`.
   */
  [[nodiscard]] CommandResult Cancel(CommandId command_id);

  /**
   * @brief Stops acceptance, cancels queued work, and joins workers.
   *
   * Active executor calls are allowed to finish. The operation is idempotent.
   * Calls made from the executor callback are rejected to prevent self-join.
   *
   * @return Completed result after all workers have stopped.
   */
  [[nodiscard]] CommandResult Shutdown();

  /**
   * @brief Returns a consistent queue statistics snapshot.
   *
   * @return Current and cumulative queue statistics.
   */
  [[nodiscard]] CommandQueueStatistics GetStatistics() const;

  /**
   * @brief Returns the number of commands waiting for workers.
   *
   * @return Current waiting command count.
   */
  [[nodiscard]] std::size_t Size() const;

  /**
   * @brief Reports whether an identifier is queued or executing.
   *
   * @param command_id Command identifier to find.
   * @return True when the command is outstanding.
   */
  [[nodiscard]] bool Contains(CommandId command_id) const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::core
