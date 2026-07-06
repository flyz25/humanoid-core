#include <humanoid/core/CommandExecutionPipeline.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
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

#include <humanoid/logging/LogLevel.hpp>
#include <humanoid/logging/LogMessage.hpp>
#include <humanoid/logging/Logger.hpp>

namespace humanoid::core {
namespace {

[[nodiscard]] CommandExecutionTimestamp Now() {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
}

[[nodiscard]] CommandResult Result(CommandStatus status, std::string message) {
  CommandResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] bool IsKnownCommandType(CommandType type) noexcept {
  switch (type) {
  case CommandType::Stand:
  case CommandType::Sit:
  case CommandType::Walk:
  case CommandType::Stop:
  case CommandType::Move:
  case CommandType::Rotate:
  case CommandType::Velocity:
  case CommandType::EmergencyStop:
  case CommandType::HandOpen:
  case CommandType::HandClose:
  case CommandType::Gesture:
  case CommandType::PlayAudio:
  case CommandType::StopAudio:
  case CommandType::SetVolume:
  case CommandType::MuteAudio:
  case CommandType::Custom:
    return true;
  }

  return false;
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

[[nodiscard]] bool HasExpired(const Command& command) noexcept {
  if (!command.hasTimeout() || command.timestamp == CommandTimestamp{}) {
    return false;
  }

  const CommandTimestamp now = Now();
  return now >= command.timestamp && now - command.timestamp >= command.timeout;
}

[[nodiscard]] std::optional<CommandResult> ValidateForSubmission(const Command& command) {
  if (command.id == 0U) {
    return Result(CommandStatus::Rejected, "Command identifier must be nonzero");
  }
  if (command.timeout < CommandTimeout::zero()) {
    return Result(CommandStatus::Rejected, "Command timeout must not be negative");
  }
  if (!IsKnownCommandType(command.type)) {
    return Result(CommandStatus::Rejected, "Command type is invalid");
  }
  if (!IsKnownPriority(command.priority)) {
    return Result(CommandStatus::Rejected, "Command priority is invalid");
  }
  if (!command.hasTimeout()) {
    return std::nullopt;
  }

  const CommandTimestamp now = Now();
  if (command.timestamp == CommandTimestamp{}) {
    return Result(CommandStatus::Rejected, "Timed commands require a monotonic creation timestamp");
  }
  if (command.timestamp > now) {
    return Result(CommandStatus::Rejected, "Command timestamp must not be in the future");
  }
  if (now - command.timestamp >= command.timeout) {
    return Result(CommandStatus::Timeout, "Command expired before execution submission");
  }

  return std::nullopt;
}

[[nodiscard]] bool IsTerminalStatus(CommandStatus status) noexcept { return isTerminal(status); }

[[nodiscard]] logging::LogLevel LogLevelForStatus(CommandStatus status) noexcept {
  switch (status) {
  case CommandStatus::Queued:
  case CommandStatus::Running:
    return logging::LogLevel::kDebug;
  case CommandStatus::Completed:
    return logging::LogLevel::kInfo;
  case CommandStatus::Cancelled:
  case CommandStatus::Timeout:
  case CommandStatus::Rejected:
    return logging::LogLevel::kWarning;
  case CommandStatus::Failed:
    return logging::LogLevel::kError;
  case CommandStatus::Pending:
    return logging::LogLevel::kTrace;
  }

  return logging::LogLevel::kWarning;
}

[[nodiscard]] std::string EventLogText(const CommandExecutionEvent& event) {
  std::string text{"execution_id="};
  text += std::to_string(event.executionId);
  text += " command_id=";
  text += std::to_string(event.commandId);
  text += " status=";
  text += std::string{toString(event.status)};
  if (!event.result.message.empty()) {
    text += " message=\"";
    text += event.result.message;
    text += '"';
  }
  return text;
}

} // namespace

class CommandExecutionPipeline::Impl final {
public:
  Impl(Executor executor, CommandExecutionPipelineOptions options,
       std::shared_ptr<logging::ILogger> logger)
      : executor_(std::move(executor)), options_(options), logger_(std::move(logger)) {
    if (!executor_) {
      throw std::invalid_argument{"CommandExecutionPipeline requires an executor"};
    }
    if (options_.maximumQueueSize == 0U) {
      throw std::invalid_argument{"CommandExecutionPipeline queue size must be greater than zero"};
    }
    if (options_.workerCount == 0U) {
      throw std::invalid_argument{
          "CommandExecutionPipeline worker count must be greater than zero"};
    }
    if (options_.maximumHistorySize == 0U) {
      throw std::invalid_argument{
          "CommandExecutionPipeline history size must be greater than zero"};
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

  [[nodiscard]] CommandExecutionHandle Submit(Command command) {
    auto execution = std::make_shared<Execution>(NextExecutionId(), std::move(command));
    std::future<CommandResult> future = execution->promise.get_future();
    CommandExecutionHandle handle{execution->record.executionId, execution->record.commandId,
                                  std::move(future)};

    const std::optional<CommandResult> validation = ValidateForSubmission(execution->command);
    if (validation) {
      CompleteRejectedExecution(execution, *validation);
      return handle;
    }

    CommandResult immediate_result;
    bool accepted = false;
    CommandExecutionEvent queued_event;
    std::vector<Callback> callbacks;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (!accepting_) {
        immediate_result =
            Result(CommandStatus::Rejected, "Command execution pipeline is shut down");
      } else if (command_to_execution_.contains(execution->command.id)) {
        immediate_result =
            Result(CommandStatus::Rejected, "Command identifier is already outstanding");
      } else if (queue_.size() >= options_.maximumQueueSize) {
        immediate_result = Result(CommandStatus::Rejected, "Command execution queue is full");
      } else {
        accepted = true;
        execution->sequence = next_sequence_++;
        execution->record.status = CommandStatus::Queued;
        execution->record.queuedAt = Now();
        execution->record.result = Result(CommandStatus::Queued, "Command queued for execution");
        records_.emplace(execution->record.executionId, execution->record);
        history_order_.push_back(execution->record.executionId);
        executions_.emplace(execution->record.executionId, execution);
        command_to_execution_.emplace(execution->record.commandId, execution->record.executionId);
        queue_.push_back(execution);
        ++metrics_.accepted;
        metrics_.highWatermark = std::max(metrics_.highWatermark, queue_.size());
        queued_event = EventFromRecord(execution->record);
        callbacks = CallbackSnapshotLocked();
      }
    }

    if (!accepted) {
      CompleteRejectedExecution(execution, immediate_result);
      return handle;
    }

    Emit(queued_event, callbacks);
    condition_.notify_one();
    return handle;
  }

  [[nodiscard]] CommandResult Cancel(CommandExecutionId execution_id) {
    if (execution_id == 0U) {
      return Result(CommandStatus::Rejected, "Execution identifier must be nonzero");
    }

    std::shared_ptr<Execution> execution;
    CommandResult result;
    CommandExecutionEvent event;
    std::vector<Callback> callbacks;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      const auto execution_iterator = executions_.find(execution_id);
      if (execution_iterator == executions_.end()) {
        return Result(CommandStatus::Rejected, "Execution is not outstanding");
      }
      execution = execution_iterator->second;
      if (execution->running) {
        return Result(CommandStatus::Rejected, "A running execution cannot be cancelled");
      }

      const auto queued = std::find(queue_.begin(), queue_.end(), execution_iterator->second);
      if (queued != queue_.end()) {
        queue_.erase(queued);
      }
      result = Result(CommandStatus::Cancelled, "Command execution cancelled before start");
      CompleteExecutionLocked(*execution, result, event, callbacks);
    }

    CompletePromise(*execution, result);
    Emit(event, callbacks);
    condition_.notify_all();
    return result;
  }

  [[nodiscard]] CommandResult Shutdown() {
    std::deque<std::shared_ptr<Execution>> cancelled_executions;
    {
      std::unique_lock<std::mutex> lock{mutex_};
      if (shutdown_complete_) {
        return Result(CommandStatus::Completed, "Command execution pipeline is shut down");
      }
      if (IsWorkerThread()) {
        return Result(CommandStatus::Rejected,
                      "Command execution pipeline cannot be shut down from its executor");
      }
      if (shutdown_in_progress_) {
        condition_.wait(lock, [this]() { return shutdown_complete_; });
        return Result(CommandStatus::Completed, "Command execution pipeline is shut down");
      }

      shutdown_in_progress_ = true;
      accepting_ = false;
      cancelled_executions.swap(queue_);
    }

    for (const std::shared_ptr<Execution>& execution : cancelled_executions) {
      CommandExecutionEvent event;
      std::vector<Callback> callbacks;
      const CommandResult result =
          Result(CommandStatus::Cancelled, "Command execution cancelled during pipeline shutdown");
      {
        std::lock_guard<std::mutex> lock{mutex_};
        CompleteExecutionLocked(*execution, result, event, callbacks);
      }
      CompletePromise(*execution, result);
      Emit(event, callbacks);
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
      PruneHistoryLocked();
      shutdown_complete_ = true;
      shutdown_in_progress_ = false;
    }
    condition_.notify_all();
    return Result(CommandStatus::Completed, "Command execution pipeline shut down");
  }

  [[nodiscard]] CommandExecutionCallbackId Subscribe(Callback callback) {
    if (!callback) {
      return CommandExecutionPipeline::kInvalidCallbackId;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    const CommandExecutionCallbackId callback_id = next_callback_id_++;
    callbacks_.emplace(callback_id, std::move(callback));
    return callback_id;
  }

  [[nodiscard]] bool Unsubscribe(CommandExecutionCallbackId callback_id) {
    if (callback_id == CommandExecutionPipeline::kInvalidCallbackId) {
      return false;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    return callbacks_.erase(callback_id) != 0U;
  }

  [[nodiscard]] CommandExecutionMetrics GetMetrics() const {
    std::lock_guard<std::mutex> lock{mutex_};
    CommandExecutionMetrics metrics = metrics_;
    metrics.currentQueued = queue_.size();
    metrics.currentRunning = running_count_;
    metrics.maximumQueueSize = options_.maximumQueueSize;
    metrics.workerCount = options_.workerCount;
    metrics.maximumHistorySize = options_.maximumHistorySize;
    metrics.historySize = records_.size();
    return metrics;
  }

  [[nodiscard]] std::vector<CommandExecutionRecord> GetHistory() const {
    std::lock_guard<std::mutex> lock{mutex_};
    std::vector<CommandExecutionRecord> history;
    history.reserve(history_order_.size());
    for (const CommandExecutionId execution_id : history_order_) {
      const auto record = records_.find(execution_id);
      if (record != records_.end()) {
        history.push_back(record->second);
      }
    }
    return history;
  }

  [[nodiscard]] std::optional<CommandExecutionRecord>
  GetRecord(CommandExecutionId execution_id) const {
    std::lock_guard<std::mutex> lock{mutex_};
    const auto record = records_.find(execution_id);
    if (record == records_.end()) {
      return std::nullopt;
    }
    return record->second;
  }

  [[nodiscard]] bool Contains(CommandExecutionId execution_id) const {
    std::lock_guard<std::mutex> lock{mutex_};
    return executions_.contains(execution_id);
  }

private:
  struct Execution final {
    Execution(CommandExecutionId execution_id, Command command_value)
        : command(std::move(command_value)) {
      record.executionId = execution_id;
      record.commandId = command.id;
      record.commandType = command.type;
      record.priority = command.priority;
      record.submittedAt = Now();
      record.status = CommandStatus::Pending;
      record.result = Result(CommandStatus::Pending, "Command execution submitted");
    }

    Command command;
    std::promise<CommandResult> promise;
    CommandExecutionRecord record;
    std::uint64_t sequence{0U};
    bool running{false};
  };

  [[nodiscard]] CommandExecutionId NextExecutionId() {
    std::lock_guard<std::mutex> lock{mutex_};
    return next_execution_id_++;
  }

  [[nodiscard]] std::vector<Callback> CallbackSnapshotLocked() const {
    std::vector<Callback> callbacks;
    callbacks.reserve(callbacks_.size());
    for (const auto& callback : callbacks_) {
      callbacks.push_back(callback.second);
    }
    return callbacks;
  }

  [[nodiscard]] static CommandExecutionEvent EventFromRecord(const CommandExecutionRecord& record) {
    CommandExecutionEvent event;
    event.executionId = record.executionId;
    event.commandId = record.commandId;
    event.commandType = record.commandType;
    event.status = record.status;
    event.timestamp = Now();
    event.result = record.result;
    return event;
  }

  void CompleteRejectedExecution(const std::shared_ptr<Execution>& execution,
                                 const CommandResult& result) {
    CommandExecutionEvent event;
    std::vector<Callback> callbacks;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      records_.emplace(execution->record.executionId, execution->record);
      history_order_.push_back(execution->record.executionId);
      CompleteRecordLocked(execution->record, result);
      records_[execution->record.executionId] = execution->record;
      RecordTerminalMetricsLocked(result.status);
      PruneHistoryLocked();
      event = EventFromRecord(execution->record);
      callbacks = CallbackSnapshotLocked();
    }
    CompletePromise(*execution, result);
    Emit(event, callbacks);
  }

  void CompleteExecutionLocked(Execution& execution, const CommandResult& result,
                               CommandExecutionEvent& event, std::vector<Callback>& callbacks) {
    CompleteRecordLocked(execution.record, result);
    records_[execution.record.executionId] = execution.record;
    executions_.erase(execution.record.executionId);
    command_to_execution_.erase(execution.record.commandId);
    if (execution.running && running_count_ > 0U) {
      --running_count_;
    }
    execution.running = false;
    RecordTerminalMetricsLocked(result.status);
    PruneHistoryLocked();
    event = EventFromRecord(execution.record);
    callbacks = CallbackSnapshotLocked();
  }

  void CompleteRecordLocked(CommandExecutionRecord& record, const CommandResult& result) const {
    record.status = result.status;
    record.completedAt = Now();
    record.result = result;
  }

  void RecordTerminalMetricsLocked(CommandStatus status) noexcept {
    switch (status) {
    case CommandStatus::Completed:
      ++metrics_.completed;
      break;
    case CommandStatus::Cancelled:
      ++metrics_.cancelled;
      break;
    case CommandStatus::Timeout:
      ++metrics_.timedOut;
      break;
    case CommandStatus::Failed:
      ++metrics_.failed;
      break;
    case CommandStatus::Rejected:
      ++metrics_.rejected;
      break;
    case CommandStatus::Pending:
    case CommandStatus::Queued:
    case CommandStatus::Running:
      ++metrics_.failed;
      break;
    }
  }

  void PruneHistoryLocked() {
    while (records_.size() > options_.maximumHistorySize && !history_order_.empty()) {
      auto candidate = std::find_if(history_order_.begin(), history_order_.end(),
                                    [this](auto id) { return !executions_.contains(id); });
      if (candidate == history_order_.end()) {
        return;
      }

      records_.erase(*candidate);
      history_order_.erase(candidate);
    }
  }

  void CompletePromise(Execution& execution, const CommandResult& result) noexcept {
    try {
      execution.promise.set_value(result);
    } catch (...) {
    }
  }

  void Emit(const CommandExecutionEvent& event, const std::vector<Callback>& callbacks) noexcept {
    Log(event);
    for (const Callback& callback : callbacks) {
      try {
        callback(event);
      } catch (...) {
        RecordCallbackFailure();
      }
    }
  }

  void Log(const CommandExecutionEvent& event) noexcept {
    const std::shared_ptr<logging::ILogger> logger = logger_;
    if (!logger) {
      return;
    }

    try {
      const logging::LogLevel level = LogLevelForStatus(event.status);
      if (!logger->isEnabled(level)) {
        return;
      }
      static_cast<void>(
          logger->log(logging::LogMessage{level, "CommandExecutionPipeline", EventLogText(event)}));
    } catch (...) {
    }
  }

  void RecordCallbackFailure() noexcept {
    std::lock_guard<std::mutex> lock{mutex_};
    ++metrics_.callbackFailures;
  }

  [[nodiscard]] bool IsWorkerThread() const noexcept {
    const std::thread::id current_thread = std::this_thread::get_id();
    return std::find(worker_ids_.begin(), worker_ids_.end(), current_thread) != worker_ids_.end();
  }

  [[nodiscard]] std::shared_ptr<Execution> TakeNextExecutionLocked() {
    const auto highest_priority =
        std::max_element(queue_.begin(), queue_.end(), [](const auto& left, const auto& right) {
          const auto left_priority = static_cast<std::uint8_t>(left->command.priority);
          const auto right_priority = static_cast<std::uint8_t>(right->command.priority);
          if (left_priority == right_priority) {
            return left->sequence > right->sequence;
          }
          return left_priority < right_priority;
        });

    std::shared_ptr<Execution> execution = *highest_priority;
    queue_.erase(highest_priority);
    execution->running = true;
    execution->record.status = CommandStatus::Running;
    execution->record.startedAt = Now();
    execution->record.result = Result(CommandStatus::Running, "Command execution started");
    records_[execution->record.executionId] = execution->record;
    ++running_count_;
    ++metrics_.started;
    metrics_.maximumConcurrentRunning = std::max(metrics_.maximumConcurrentRunning, running_count_);
    return execution;
  }

  void Run(std::stop_token stop_token, std::size_t worker_index) noexcept {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      worker_ids_[worker_index] = std::this_thread::get_id();
    }

    while (true) {
      std::shared_ptr<Execution> execution;
      CommandExecutionEvent running_event;
      std::vector<Callback> running_callbacks;
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

        execution = TakeNextExecutionLocked();
        running_event = EventFromRecord(execution->record);
        running_callbacks = CallbackSnapshotLocked();
      }
      Emit(running_event, running_callbacks);

      CommandResult result;
      if (HasExpired(execution->command)) {
        result = Result(CommandStatus::Timeout, "Command expired before executor invocation");
      } else {
        result = InvokeExecutor(*execution);
        if (result.status == CommandStatus::Completed && HasExpired(execution->command)) {
          result = Result(CommandStatus::Timeout, "Command completed after its timeout");
        } else if (!IsTerminalStatus(result.status)) {
          result = Result(CommandStatus::Failed, "Command executor returned a nonterminal status");
        }
      }

      CommandExecutionEvent final_event;
      std::vector<Callback> final_callbacks;
      {
        std::lock_guard<std::mutex> lock{mutex_};
        CompleteExecutionLocked(*execution, result, final_event, final_callbacks);
      }
      condition_.notify_all();
      CompletePromise(*execution, result);
      Emit(final_event, final_callbacks);
    }
  }

  [[nodiscard]] CommandResult InvokeExecutor(const Execution& execution) noexcept {
    try {
      return executor_(execution.command);
    } catch (const std::exception& exception) {
      try {
        return Result(CommandStatus::Failed,
                      std::string{"Command executor failed: "} + exception.what());
      } catch (...) {
        return Result(CommandStatus::Failed, "Command executor failed");
      }
    } catch (...) {
      return Result(CommandStatus::Failed, "Command executor failed with an unknown error");
    }
  }

  Executor executor_;
  CommandExecutionPipelineOptions options_;
  std::shared_ptr<logging::ILogger> logger_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<std::shared_ptr<Execution>> queue_;
  std::unordered_map<CommandExecutionId, std::shared_ptr<Execution>> executions_;
  std::unordered_map<CommandId, CommandExecutionId> command_to_execution_;
  std::unordered_map<CommandExecutionId, CommandExecutionRecord> records_;
  std::deque<CommandExecutionId> history_order_;
  std::unordered_map<CommandExecutionCallbackId, Callback> callbacks_;
  CommandExecutionMetrics metrics_;
  std::size_t running_count_{0U};
  CommandExecutionId next_execution_id_{1U};
  CommandExecutionCallbackId next_callback_id_{1U};
  std::uint64_t next_sequence_{0U};
  bool accepting_{true};
  bool shutdown_in_progress_{false};
  bool shutdown_complete_{false};
  std::vector<std::thread::id> worker_ids_;
  std::vector<std::jthread> workers_;
};

CommandExecutionPipeline::CommandExecutionPipeline(Executor executor,
                                                   CommandExecutionPipelineOptions options,
                                                   std::shared_ptr<logging::ILogger> logger)
    : impl_(std::make_unique<Impl>(std::move(executor), options, std::move(logger))) {}

CommandExecutionPipeline::~CommandExecutionPipeline() noexcept = default;

CommandExecutionHandle CommandExecutionPipeline::Submit(Command command) {
  return impl_->Submit(std::move(command));
}

CommandResult CommandExecutionPipeline::Cancel(CommandExecutionId execution_id) {
  return impl_->Cancel(execution_id);
}

CommandResult CommandExecutionPipeline::Shutdown() { return impl_->Shutdown(); }

CommandExecutionCallbackId CommandExecutionPipeline::Subscribe(Callback callback) {
  return impl_->Subscribe(std::move(callback));
}

bool CommandExecutionPipeline::Unsubscribe(CommandExecutionCallbackId callback_id) {
  return impl_->Unsubscribe(callback_id);
}

CommandExecutionMetrics CommandExecutionPipeline::GetMetrics() const { return impl_->GetMetrics(); }

std::vector<CommandExecutionRecord> CommandExecutionPipeline::GetHistory() const {
  return impl_->GetHistory();
}

std::optional<CommandExecutionRecord>
CommandExecutionPipeline::GetRecord(CommandExecutionId execution_id) const {
  return impl_->GetRecord(execution_id);
}

bool CommandExecutionPipeline::Contains(CommandExecutionId execution_id) const {
  return impl_->Contains(execution_id);
}

} // namespace humanoid::core
