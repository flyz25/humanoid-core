/**
 * @file command_execution_pipeline_unit_test.cpp
 * @brief Validates command execution lifecycle infrastructure.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/core/CommandExecutionPipeline.h>
#include <humanoid/logging/Logger.hpp>

namespace {

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

[[nodiscard]] humanoid::core::Command
MakeCommand(humanoid::core::CommandId id,
            humanoid::core::CommandPriority priority = humanoid::core::CommandPriority::Normal) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = humanoid::core::CommandType::Stop;
  command.priority = priority;
  return command;
}

[[nodiscard]] humanoid::core::CommandResult Result(humanoid::core::CommandStatus status,
                                                   std::string message) {
  humanoid::core::CommandResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] humanoid::core::CommandResult CompletedResult() {
  return Result(humanoid::core::CommandStatus::Completed, "completed");
}

class MemoryLogger final : public humanoid::logging::ILogger {
public:
  humanoid::common::Status log(const humanoid::logging::LogMessage& message) override {
    std::lock_guard<std::mutex> lock{mutex_};
    messages_.push_back(message.text());
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] bool isEnabled(humanoid::logging::LogLevel) const noexcept override { return true; }

  [[nodiscard]] std::size_t messageCount() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return messages_.size();
  }

private:
  mutable std::mutex mutex_;
  std::vector<std::string> messages_;
};

class EventRecorder final {
public:
  void Record(const humanoid::core::CommandExecutionEvent& event) {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      events_.push_back(event);
    }
    condition_.notify_all();
  }

  [[nodiscard]] bool WaitFor(std::size_t event_count, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock{mutex_};
    return condition_.wait_for(lock, timeout,
                               [this, event_count]() { return events_.size() >= event_count; });
  }

  [[nodiscard]] std::vector<humanoid::core::CommandExecutionEvent> Events() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return events_;
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<humanoid::core::CommandExecutionEvent> events_;
};

class BlockingExecutor final {
public:
  explicit BlockingExecutor(humanoid::core::CommandId blocked_id) : blocked_id_(blocked_id) {}

  [[nodiscard]] humanoid::core::CommandResult Execute(const humanoid::core::Command& command) {
    {
      std::unique_lock<std::mutex> lock{mutex_};
      execution_order_.push_back(command.id);
      if (command.id == blocked_id_) {
        blocked_started_ = true;
        condition_.notify_all();
        condition_.wait(lock, [this]() { return released_; });
      }
    }
    return CompletedResult();
  }

  [[nodiscard]] bool WaitUntilBlocked(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock{mutex_};
    return condition_.wait_for(lock, timeout, [this]() { return blocked_started_; });
  }

  void Release() {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      released_ = true;
    }
    condition_.notify_all();
  }

  [[nodiscard]] std::vector<humanoid::core::CommandId> ExecutionOrder() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return execution_order_;
  }

private:
  humanoid::core::CommandId blocked_id_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<humanoid::core::CommandId> execution_order_;
  bool blocked_started_{false};
  bool released_{false};
};

void UpdateMaximum(std::atomic_size_t& maximum, std::size_t candidate) {
  std::size_t observed = maximum.load(std::memory_order_relaxed);
  while (observed < candidate &&
         !maximum.compare_exchange_weak(observed, candidate, std::memory_order_relaxed)) {
  }
}

void TestLifecycleCallbacksHistoryAndLogging() {
  auto logger = std::make_shared<MemoryLogger>();
  EventRecorder recorder;
  humanoid::core::CommandExecutionPipeline pipeline{
      [](const humanoid::core::Command&) { return CompletedResult(); }, {8U, 1U, 16U}, logger};

  const auto subscription = pipeline.Subscribe(
      [&recorder](const humanoid::core::CommandExecutionEvent& event) { recorder.Record(event); });
  Check(subscription != humanoid::core::CommandExecutionPipeline::kInvalidCallbackId,
        "Pipeline callback subscription failed");

  humanoid::core::CommandExecutionHandle handle = pipeline.Submit(MakeCommand(1U));
  Check(handle.executionId != 0U && handle.commandId == 1U, "Execution handle is invalid");
  Check(handle.result.get().isSuccess(), "Lifecycle command failed");
  Check(recorder.WaitFor(3U, std::chrono::milliseconds{500}), "Lifecycle callbacks were missing");

  const std::vector<humanoid::core::CommandExecutionEvent> events = recorder.Events();
  Check(events.size() == 3U, "Unexpected lifecycle event count");
  Check(events[0].status == humanoid::core::CommandStatus::Queued &&
            events[1].status == humanoid::core::CommandStatus::Running &&
            events[2].status == humanoid::core::CommandStatus::Completed,
        "Lifecycle event order is incorrect");
  Check(events[0].executionId == handle.executionId && events[2].executionId == handle.executionId,
        "Lifecycle events used incorrect execution ID");

  const std::vector<humanoid::core::CommandExecutionRecord> history = pipeline.GetHistory();
  Check(history.size() == 1U, "History size is incorrect");
  Check(history[0].executionId == handle.executionId &&
            history[0].status == humanoid::core::CommandStatus::Completed,
        "History record is incorrect");
  Check(history[0].queuedAt.has_value() && history[0].startedAt.has_value() &&
            history[0].completedAt.has_value(),
        "History timestamps were not recorded");

  const humanoid::core::CommandExecutionMetrics metrics = pipeline.GetMetrics();
  Check(metrics.accepted == 1U && metrics.started == 1U && metrics.completed == 1U,
        "Lifecycle metrics are incorrect");
  Check(logger->messageCount() >= 3U, "Lifecycle events were not logged");
  Check(pipeline.Unsubscribe(subscription), "Pipeline callback unsubscribe failed");
  Check(pipeline.Shutdown().isSuccess(), "Lifecycle pipeline shutdown failed");
}

void TestCancellationAndTimeout() {
  BlockingExecutor executor{10U};
  humanoid::core::CommandExecutionPipeline pipeline{
      [&executor](const humanoid::core::Command& command) { return executor.Execute(command); },
      {8U, 1U, 16U}};

  humanoid::core::CommandExecutionHandle blocker = pipeline.Submit(MakeCommand(10U));
  Check(executor.WaitUntilBlocked(std::chrono::milliseconds{500}),
        "Cancellation blocker did not start");

  humanoid::core::CommandExecutionHandle cancellable = pipeline.Submit(MakeCommand(11U));
  Check(pipeline.Cancel(cancellable.executionId).status == humanoid::core::CommandStatus::Cancelled,
        "Queued execution cancellation failed");
  Check(cancellable.result.get().status == humanoid::core::CommandStatus::Cancelled,
        "Cancelled execution future has incorrect status");

  humanoid::core::Command timed = MakeCommand(12U);
  timed.timeout = std::chrono::milliseconds{20};
  humanoid::core::CommandExecutionHandle timed_handle = pipeline.Submit(std::move(timed));
  std::this_thread::sleep_for(std::chrono::milliseconds{30});
  executor.Release();

  Check(blocker.result.get().isSuccess(), "Blocking execution failed");
  Check(timed_handle.result.get().status == humanoid::core::CommandStatus::Timeout,
        "Queued execution timeout was not propagated");

  const std::vector<humanoid::core::CommandExecutionRecord> history = pipeline.GetHistory();
  const auto cancelled = std::find_if(history.begin(), history.end(), [&](const auto& record) {
    return record.executionId == cancellable.executionId;
  });
  Check(cancelled != history.end() && cancelled->status == humanoid::core::CommandStatus::Cancelled,
        "Cancelled execution was not retained in history");

  const humanoid::core::CommandExecutionMetrics metrics = pipeline.GetMetrics();
  Check(metrics.accepted == 3U && metrics.completed == 1U && metrics.cancelled == 1U &&
            metrics.timedOut == 1U,
        "Cancellation or timeout metrics are incorrect");
  Check(pipeline.Shutdown().isSuccess(), "Cancellation pipeline shutdown failed");
}

void TestErrorPropagation() {
  humanoid::core::CommandExecutionPipeline pipeline{
      [](const humanoid::core::Command& command) {
        if (command.id == 20U) {
          throw std::runtime_error{"executor failure"};
        }
        if (command.id == 21U) {
          return Result(humanoid::core::CommandStatus::Failed, "reported failure");
        }
        if (command.id == 22U) {
          return Result(humanoid::core::CommandStatus::Running, "bad nonterminal result");
        }
        return CompletedResult();
      },
      {8U, 2U, 16U}};

  Check(pipeline.Submit(MakeCommand(20U)).result.get().status ==
            humanoid::core::CommandStatus::Failed,
        "Executor exception was not translated");
  Check(pipeline.Submit(MakeCommand(21U)).result.get().status ==
            humanoid::core::CommandStatus::Failed,
        "Executor failure result was not propagated");
  Check(pipeline.Submit(MakeCommand(22U)).result.get().status ==
            humanoid::core::CommandStatus::Failed,
        "Nonterminal executor result was not normalized");
  Check(pipeline.Submit(MakeCommand(23U)).result.get().isSuccess(),
        "Pipeline did not continue after executor errors");

  const humanoid::core::CommandExecutionMetrics metrics = pipeline.GetMetrics();
  Check(metrics.accepted == 4U && metrics.failed == 3U && metrics.completed == 1U,
        "Error propagation metrics are incorrect");
  Check(pipeline.Shutdown().isSuccess(), "Error propagation pipeline shutdown failed");
}

void TestConcurrentExecution() {
  constexpr std::size_t kProducerCount{8U};
  constexpr std::size_t kCommandsPerProducer{125U};
  constexpr std::size_t kCommandCount{kProducerCount * kCommandsPerProducer};
  constexpr std::size_t kWorkerCount{4U};

  std::atomic_size_t active_workers{0U};
  std::atomic_size_t maximum_active_workers{0U};
  std::atomic_size_t executed{0U};

  humanoid::core::CommandExecutionPipeline pipeline{
      [&](const humanoid::core::Command&) {
        const std::size_t active = active_workers.fetch_add(1U, std::memory_order_relaxed) + 1U;
        UpdateMaximum(maximum_active_workers, active);
        std::this_thread::sleep_for(std::chrono::microseconds{50});
        active_workers.fetch_sub(1U, std::memory_order_relaxed);
        executed.fetch_add(1U, std::memory_order_relaxed);
        return CompletedResult();
      },
      {kCommandCount, kWorkerCount, kCommandCount}};

  std::vector<std::vector<humanoid::core::CommandExecutionHandle>> handles{kProducerCount};
  std::vector<std::thread> producers;
  producers.reserve(kProducerCount);
  for (std::size_t producer_index = 0U; producer_index < kProducerCount; ++producer_index) {
    producers.emplace_back([producer_index, &handles, &pipeline]() {
      handles[producer_index].reserve(kCommandsPerProducer);
      for (std::size_t command_index = 0U; command_index < kCommandsPerProducer; ++command_index) {
        const humanoid::core::CommandId command_id = static_cast<humanoid::core::CommandId>(
            producer_index * kCommandsPerProducer + command_index + 1U);
        handles[producer_index].push_back(pipeline.Submit(MakeCommand(command_id)));
      }
    });
  }

  for (std::thread& producer : producers) {
    producer.join();
  }

  for (std::vector<humanoid::core::CommandExecutionHandle>& producer_handles : handles) {
    for (humanoid::core::CommandExecutionHandle& handle : producer_handles) {
      Check(handle.result.get().isSuccess(), "Concurrent execution failed");
    }
  }

  const humanoid::core::CommandExecutionMetrics metrics = pipeline.GetMetrics();
  Check(executed.load(std::memory_order_relaxed) == kCommandCount,
        "Concurrent executor count is incorrect");
  Check(metrics.accepted == kCommandCount && metrics.completed == kCommandCount &&
            metrics.failed == 0U && metrics.currentQueued == 0U && metrics.currentRunning == 0U,
        "Concurrent execution metrics are incorrect");
  Check(metrics.maximumConcurrentRunning > 1U &&
            maximum_active_workers.load(std::memory_order_relaxed) > 1U,
        "Concurrent workers did not overlap");
  Check(pipeline.GetHistory().size() == kCommandCount, "Concurrent history size is incorrect");
  Check(pipeline.Shutdown().isSuccess(), "Concurrent pipeline shutdown failed");
}

void TestCallbackFailureIsolation() {
  humanoid::core::CommandExecutionPipeline pipeline{
      [](const humanoid::core::Command&) { return CompletedResult(); }, {4U, 1U, 8U}};

  const auto subscription = pipeline.Subscribe([](const humanoid::core::CommandExecutionEvent&) {
    throw std::runtime_error{"callback failure"};
  });
  Check(subscription != humanoid::core::CommandExecutionPipeline::kInvalidCallbackId,
        "Throwing callback subscription failed");
  Check(pipeline.Submit(MakeCommand(30U)).result.get().isSuccess(),
        "Callback failure interrupted execution");
  Check(pipeline.GetMetrics().callbackFailures > 0U, "Callback failure was not recorded");
  Check(pipeline.Shutdown().isSuccess(), "Callback failure pipeline shutdown failed");
}

} // namespace

int main() {
  try {
    TestLifecycleCallbacksHistoryAndLogging();
    TestCancellationAndTimeout();
    TestErrorPropagation();
    TestConcurrentExecution();
    TestCallbackFailureIsolation();
  } catch (...) {
    return 1;
  }

  return 0;
}
