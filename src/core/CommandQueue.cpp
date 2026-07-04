#include <humanoid/core/CommandQueue.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace humanoid::core {
namespace {

[[nodiscard]] CommandResult Result(CommandStatus status, std::string message) {
  CommandResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] bool IsKnownPriority(CommandPriority priority) noexcept {
  switch (priority) {
  case CommandPriority::Low:
  case CommandPriority::Normal:
  case CommandPriority::High:
  case CommandPriority::Critical:
    return true;
  }

  return false;
}

[[nodiscard]] std::optional<CommandResult> ValidateForQueue(const Command& command) {
  if (command.id == 0U) {
    return Result(CommandStatus::Rejected, "Command identifier must be nonzero");
  }
  if (command.timeout < CommandTimeout::zero()) {
    return Result(CommandStatus::Rejected, "Command timeout must not be negative");
  }
  if (!IsKnownPriority(command.priority)) {
    return Result(CommandStatus::Rejected, "Command priority is invalid");
  }
  if (!command.hasTimeout()) {
    return std::nullopt;
  }

  const CommandTimestamp now =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  if (command.timestamp == CommandTimestamp{}) {
    return Result(CommandStatus::Rejected, "Timed commands require a monotonic creation timestamp");
  }
  if (command.timestamp > now) {
    return Result(CommandStatus::Rejected, "Command timestamp must not be in the future");
  }
  if (now - command.timestamp >= command.timeout) {
    return Result(CommandStatus::Timeout, "Command expired before queue acceptance");
  }

  return std::nullopt;
}

[[nodiscard]] bool HasExpired(const Command& command) noexcept {
  if (!command.hasTimeout() || command.timestamp == CommandTimestamp{}) {
    return false;
  }

  const CommandTimestamp now =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return now >= command.timestamp && now - command.timestamp >= command.timeout;
}

[[nodiscard]] bool IsTerminalStatus(CommandStatus status) noexcept { return isTerminal(status); }

[[nodiscard]] std::future<CommandResult> ReadyFuture(CommandResult result) {
  std::promise<CommandResult> promise;
  std::future<CommandResult> future = promise.get_future();
  promise.set_value(std::move(result));
  return future;
}

} // namespace

class CommandQueue::Impl final {
public:
  Impl(Executor executor, CommandQueueOptions options)
      : executor_(std::move(executor)), options_(options) {
    if (!executor_) {
      throw std::invalid_argument{"CommandQueue requires an executor"};
    }
    if (options_.maximumQueueSize == 0U) {
      throw std::invalid_argument{"CommandQueue maximum size must be greater than zero"};
    }
    if (options_.workerCount == 0U) {
      throw std::invalid_argument{"CommandQueue worker count must be greater than zero"};
    }

    try {
      worker_ids_.resize(options_.workerCount);
      workers_.reserve(options_.workerCount);
      for (std::size_t worker_index = 0U; worker_index < options_.workerCount; ++worker_index) {
        workers_.emplace_back([this, worker_index](std::stop_token stop_token) {
          Run(std::move(stop_token), worker_index);
        });
      }
    } catch (...) {
      {
        std::lock_guard<std::mutex> lock{mutex_};
        accepting_ = false;
      }
      for (std::jthread& worker : workers_) {
        worker.request_stop();
      }
      condition_.notify_all();
      for (std::jthread& worker : workers_) {
        if (worker.joinable()) {
          worker.join();
        }
      }
      throw;
    }
  }

  ~Impl() noexcept {
    try {
      static_cast<void>(Shutdown());
    } catch (...) {
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] std::future<CommandResult> Enqueue(Command command) {
    if (const std::optional<CommandResult> validation = ValidateForQueue(command)) {
      RecordImmediateResult(validation->status);
      return ReadyFuture(*validation);
    }

    auto task = std::make_shared<Task>(std::move(command));
    std::future<CommandResult> future = task->promise.get_future();

    CommandResult immediate_result;
    bool accepted = false;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (!accepting_) {
        immediate_result = Result(CommandStatus::Rejected, "Command queue is shut down");
        ++statistics_.rejected;
      } else if (tasks_.contains(task->command.id)) {
        immediate_result =
            Result(CommandStatus::Rejected, "Command identifier is already outstanding");
        ++statistics_.rejected;
      } else if (queue_.size() >= options_.maximumQueueSize) {
        immediate_result = Result(CommandStatus::Rejected, "Command queue is full");
        ++statistics_.rejected;
      } else {
        try {
          tasks_.emplace(task->command.id, task);
          queue_.push_back(task);
          ++statistics_.accepted;
          statistics_.highWatermark = std::max(statistics_.highWatermark, queue_.size());
          accepted = true;
        } catch (...) {
          tasks_.erase(task->command.id);
          ++statistics_.failed;
          throw;
        }
      }
    }

    if (!accepted) {
      task->promise.set_value(std::move(immediate_result));
    } else {
      condition_.notify_one();
    }
    return future;
  }

  [[nodiscard]] CommandResult Cancel(CommandId command_id) {
    if (command_id == 0U) {
      return Result(CommandStatus::Rejected, "Command identifier must be nonzero");
    }

    std::shared_ptr<Task> cancelled_task;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      const auto task = tasks_.find(command_id);
      if (task == tasks_.end()) {
        return Result(CommandStatus::Rejected, "Command is not outstanding");
      }
      if (task->second->state == TaskState::Running) {
        return Result(CommandStatus::Rejected, "A running command cannot be cancelled");
      }

      cancelled_task = task->second;
      tasks_.erase(task);
      const auto queued = std::find(queue_.begin(), queue_.end(), cancelled_task);
      if (queued != queue_.end()) {
        queue_.erase(queued);
      }
      ++statistics_.cancelled;
    }

    CommandResult result = Result(CommandStatus::Cancelled, "Command cancelled before execution");
    cancelled_task->promise.set_value(result);
    condition_.notify_all();
    return result;
  }

  [[nodiscard]] CommandResult Shutdown() {
    std::deque<std::shared_ptr<Task>> cancelled_tasks;
    {
      std::unique_lock<std::mutex> lock{mutex_};
      if (shutdown_complete_) {
        return Result(CommandStatus::Completed, "Command queue is shut down");
      }
      if (IsWorkerThread()) {
        return Result(CommandStatus::Rejected,
                      "Command queue cannot be shut down from its executor");
      }
      if (shutdown_in_progress_) {
        condition_.wait(lock, [this]() { return shutdown_complete_; });
        return Result(CommandStatus::Completed, "Command queue is shut down");
      }

      shutdown_in_progress_ = true;
      accepting_ = false;
      cancelled_tasks.swap(queue_);
      for (const std::shared_ptr<Task>& task : cancelled_tasks) {
        tasks_.erase(task->command.id);
        ++statistics_.cancelled;
      }
    }

    for (const std::shared_ptr<Task>& task : cancelled_tasks) {
      try {
        task->promise.set_value(
            Result(CommandStatus::Cancelled, "Command cancelled during queue shutdown"));
      } catch (...) {
      }
    }

    for (std::jthread& worker : workers_) {
      worker.request_stop();
    }
    condition_.notify_all();
    for (std::jthread& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    {
      std::lock_guard<std::mutex> lock{mutex_};
      shutdown_complete_ = true;
      shutdown_in_progress_ = false;
    }
    condition_.notify_all();
    return Result(CommandStatus::Completed, "Command queue shut down");
  }

  [[nodiscard]] CommandQueueStatistics GetStatistics() const {
    std::lock_guard<std::mutex> lock{mutex_};
    CommandQueueStatistics statistics = statistics_;
    statistics.queued = queue_.size();
    statistics.active = active_count_;
    statistics.maximumQueueSize = options_.maximumQueueSize;
    statistics.workerCount = options_.workerCount;
    return statistics;
  }

  [[nodiscard]] std::size_t Size() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return queue_.size();
  }

  [[nodiscard]] bool Contains(CommandId command_id) const {
    std::lock_guard<std::mutex> lock{mutex_};
    return tasks_.contains(command_id);
  }

private:
  enum class TaskState : std::uint8_t { Queued, Running };

  struct Task final {
    explicit Task(Command command_value) : command(std::move(command_value)) {}

    Command command;
    std::promise<CommandResult> promise;
    TaskState state{TaskState::Queued};
  };

  void RecordImmediateResult(CommandStatus status) {
    std::lock_guard<std::mutex> lock{mutex_};
    RecordResultLocked(status);
  }

  [[nodiscard]] bool IsWorkerThread() const noexcept {
    const std::thread::id current_thread = std::this_thread::get_id();
    return std::find(worker_ids_.begin(), worker_ids_.end(), current_thread) != worker_ids_.end();
  }

  void RecordResultLocked(CommandStatus status) noexcept {
    switch (status) {
    case CommandStatus::Completed:
      ++statistics_.completed;
      break;
    case CommandStatus::Cancelled:
      ++statistics_.cancelled;
      break;
    case CommandStatus::Failed:
      ++statistics_.failed;
      break;
    case CommandStatus::Timeout:
      ++statistics_.timedOut;
      break;
    case CommandStatus::Rejected:
      ++statistics_.rejected;
      break;
    case CommandStatus::Pending:
    case CommandStatus::Queued:
    case CommandStatus::Running:
      ++statistics_.failed;
      break;
    }
  }

  void Run(std::stop_token stop_token, std::size_t worker_index) noexcept {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      worker_ids_[worker_index] = std::this_thread::get_id();
    }

    while (true) {
      std::shared_ptr<Task> task;
      {
        std::unique_lock<std::mutex> lock{mutex_};
        condition_.wait(lock, [this, &stop_token]() {
          return stop_token.stop_requested() || !queue_.empty() || !accepting_;
        });

        if (queue_.empty()) {
          if (stop_token.stop_requested() || !accepting_) {
            worker_ids_[worker_index] = std::thread::id{};
            return;
          }
          continue;
        }

        const auto highest_priority =
            std::max_element(queue_.begin(), queue_.end(), [](const auto& left, const auto& right) {
              return static_cast<std::uint8_t>(left->command.priority) <
                     static_cast<std::uint8_t>(right->command.priority);
            });
        task = *highest_priority;
        queue_.erase(highest_priority);
        task->state = TaskState::Running;
        ++active_count_;
      }

      CommandResult result;
      if (HasExpired(task->command)) {
        result = Result(CommandStatus::Timeout, "Command expired while waiting in queue");
      } else {
        try {
          result = executor_(task->command);
        } catch (const std::exception& exception) {
          result.status = CommandStatus::Failed;
          try {
            result.message = std::string{"Command executor failed: "} + exception.what();
          } catch (...) {
          }
        } catch (...) {
          result.status = CommandStatus::Failed;
          try {
            result.message = "Command executor failed with an unknown error";
          } catch (...) {
          }
        }

        if (result.status == CommandStatus::Completed && HasExpired(task->command)) {
          result = Result(CommandStatus::Timeout, "Command completed after its timeout");
        } else if (!IsTerminalStatus(result.status)) {
          result = Result(CommandStatus::Failed, "Command executor returned a nonterminal status");
        }
      }

      {
        std::lock_guard<std::mutex> lock{mutex_};
        tasks_.erase(task->command.id);
        --active_count_;
        RecordResultLocked(result.status);
      }
      condition_.notify_all();

      try {
        task->promise.set_value(std::move(result));
      } catch (...) {
      }
    }
  }

  Executor executor_;
  CommandQueueOptions options_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<std::shared_ptr<Task>> queue_;
  std::unordered_map<CommandId, std::shared_ptr<Task>> tasks_;
  CommandQueueStatistics statistics_;
  std::size_t active_count_{0U};
  bool accepting_{true};
  bool shutdown_in_progress_{false};
  bool shutdown_complete_{false};
  std::vector<std::thread::id> worker_ids_;
  std::vector<std::jthread> workers_;
};

CommandQueue::CommandQueue(Executor executor, CommandQueueOptions options)
    : impl_(std::make_unique<Impl>(std::move(executor), options)) {}

CommandQueue::~CommandQueue() noexcept = default;

std::future<CommandResult> CommandQueue::Enqueue(Command command) {
  return impl_->Enqueue(std::move(command));
}

CommandResult CommandQueue::Cancel(CommandId command_id) { return impl_->Cancel(command_id); }

CommandResult CommandQueue::Shutdown() { return impl_->Shutdown(); }

CommandQueueStatistics CommandQueue::GetStatistics() const { return impl_->GetStatistics(); }

std::size_t CommandQueue::Size() const { return impl_->Size(); }

bool CommandQueue::Contains(CommandId command_id) const { return impl_->Contains(command_id); }

} // namespace humanoid::core
