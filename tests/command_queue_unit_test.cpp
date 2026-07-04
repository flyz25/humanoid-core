/**
 * @file command_queue_unit_test.cpp
 * @brief Validates bounded asynchronous command queue behavior.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/core/CommandQueue.h>

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

[[nodiscard]] humanoid::core::CommandResult CompletedResult() {
  humanoid::core::CommandResult result;
  result.status = humanoid::core::CommandStatus::Completed;
  result.message = "completed";
  return result;
}

class ExecutionGate final {
public:
  explicit ExecutionGate(humanoid::core::CommandId blocked_id) : blocked_id_(blocked_id) {}

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

void TestConstructionValidation() {
  bool missing_executor_rejected = false;
  try {
    humanoid::core::CommandQueue queue{humanoid::core::CommandQueue::Executor{}};
  } catch (const std::invalid_argument&) {
    missing_executor_rejected = true;
  }
  Check(missing_executor_rejected, "Missing queue executor was accepted");

  bool zero_capacity_rejected = false;
  try {
    humanoid::core::CommandQueue queue{
        [](const humanoid::core::Command&) { return CompletedResult(); }, {0U, 1U}};
  } catch (const std::invalid_argument&) {
    zero_capacity_rejected = true;
  }
  Check(zero_capacity_rejected, "Zero queue capacity was accepted");

  bool zero_workers_rejected = false;
  try {
    humanoid::core::CommandQueue queue{
        [](const humanoid::core::Command&) { return CompletedResult(); }, {1U, 0U}};
  } catch (const std::invalid_argument&) {
    zero_workers_rejected = true;
  }
  Check(zero_workers_rejected, "Zero worker count was accepted");
}

void TestPriorityAndFifoOrdering() {
  ExecutionGate gate{1U};
  humanoid::core::CommandQueue queue{
      [&gate](const humanoid::core::Command& command) { return gate.Execute(command); }, {8U, 1U}};

  std::future<humanoid::core::CommandResult> blocker =
      queue.Enqueue(MakeCommand(1U, humanoid::core::CommandPriority::Normal));
  Check(gate.WaitUntilBlocked(std::chrono::milliseconds{500}),
        "Priority test blocker did not start");

  std::future<humanoid::core::CommandResult> normal_first =
      queue.Enqueue(MakeCommand(2U, humanoid::core::CommandPriority::Normal));
  std::future<humanoid::core::CommandResult> normal_second =
      queue.Enqueue(MakeCommand(3U, humanoid::core::CommandPriority::Normal));
  std::future<humanoid::core::CommandResult> critical =
      queue.Enqueue(MakeCommand(4U, humanoid::core::CommandPriority::Critical));
  std::future<humanoid::core::CommandResult> low =
      queue.Enqueue(MakeCommand(5U, humanoid::core::CommandPriority::Low));

  Check(queue.Size() == 4U, "Priority test queue size is incorrect");
  gate.Release();

  Check(blocker.get().isSuccess(), "Priority blocker failed");
  Check(normal_first.get().isSuccess(), "First normal command failed");
  Check(normal_second.get().isSuccess(), "Second normal command failed");
  Check(critical.get().isSuccess(), "Critical command failed");
  Check(low.get().isSuccess(), "Low command failed");

  const std::vector<humanoid::core::CommandId> expected{1U, 4U, 2U, 3U, 5U};
  Check(gate.ExecutionOrder() == expected, "Priority or FIFO ordering is incorrect");

  const humanoid::core::CommandQueueStatistics statistics = queue.GetStatistics();
  Check(statistics.accepted == 5U && statistics.completed == 5U,
        "Priority queue completion statistics are incorrect");
  Check(statistics.highWatermark == 4U && statistics.queued == 0U && statistics.active == 0U,
        "Priority queue depth statistics are incorrect");
  Check(queue.Shutdown().isSuccess(), "Priority queue shutdown failed");
}

void TestCapacityTimeoutAndCancellation() {
  ExecutionGate gate{10U};
  humanoid::core::CommandQueue queue{
      [&gate](const humanoid::core::Command& command) { return gate.Execute(command); }, {2U, 1U}};

  std::future<humanoid::core::CommandResult> blocker = queue.Enqueue(MakeCommand(10U));
  Check(gate.WaitUntilBlocked(std::chrono::milliseconds{500}),
        "Capacity test blocker did not start");

  humanoid::core::Command timed_command = MakeCommand(11U);
  timed_command.timeout = std::chrono::milliseconds{20};
  std::future<humanoid::core::CommandResult> timed = queue.Enqueue(std::move(timed_command));
  std::future<humanoid::core::CommandResult> cancellable = queue.Enqueue(MakeCommand(12U));

  Check(queue.Size() == 2U, "Bounded queue did not reach configured capacity");
  Check(queue.Contains(11U) && queue.Contains(12U), "Outstanding command lookup failed");

  Check(queue.Enqueue(MakeCommand(11U)).get().status == humanoid::core::CommandStatus::Rejected,
        "Duplicate outstanding command ID was accepted");
  Check(queue.Enqueue(MakeCommand(13U)).get().status == humanoid::core::CommandStatus::Rejected,
        "Command was accepted beyond maximum queue size");
  Check(queue.Cancel(10U).status == humanoid::core::CommandStatus::Rejected,
        "Running command was cancelled");
  Check(queue.Cancel(12U).status == humanoid::core::CommandStatus::Cancelled,
        "Queued command cancellation failed");
  Check(cancellable.get().status == humanoid::core::CommandStatus::Cancelled,
        "Cancelled command future has incorrect status");

  std::this_thread::sleep_for(std::chrono::milliseconds{30});
  gate.Release();
  Check(blocker.get().isSuccess(), "Capacity blocker failed");
  Check(timed.get().status == humanoid::core::CommandStatus::Timeout,
        "Queued command timeout was not enforced");

  const humanoid::core::CommandQueueStatistics statistics = queue.GetStatistics();
  Check(statistics.maximumQueueSize == 2U && statistics.workerCount == 1U,
        "Queue configuration statistics are incorrect");
  Check(statistics.accepted == 3U && statistics.completed == 1U && statistics.cancelled == 1U &&
            statistics.timedOut == 1U && statistics.rejected == 2U,
        "Capacity, timeout, or cancellation statistics are incorrect");
  Check(statistics.highWatermark == 2U, "Queue high-water mark is incorrect");
  Check(queue.Shutdown().isSuccess(), "Capacity queue shutdown failed");
  Check(queue.Enqueue(MakeCommand(14U)).get().status == humanoid::core::CommandStatus::Rejected,
        "Command was accepted after queue shutdown");
}

void TestExecutorFailureIsolation() {
  std::optional<std::reference_wrapper<humanoid::core::CommandQueue>> queue_reference;
  humanoid::core::CommandQueue queue{[&queue_reference](const humanoid::core::Command& command) {
                                       if (command.id == 20U) {
                                         throw std::runtime_error{"executor failure"};
                                       }
                                       if (command.id == 21U) {
                                         humanoid::core::CommandResult result;
                                         result.status = humanoid::core::CommandStatus::Running;
                                         return result;
                                       }
                                       if (command.id == 22U) {
                                         return queue_reference->get().Shutdown();
                                       }
                                       return CompletedResult();
                                     },
                                     {8U, 1U}};
  queue_reference.emplace(queue);

  Check(queue.Enqueue(MakeCommand(20U)).get().status == humanoid::core::CommandStatus::Failed,
        "Executor exception was not contained");
  Check(queue.Enqueue(MakeCommand(21U)).get().status == humanoid::core::CommandStatus::Failed,
        "Nonterminal executor result was accepted");
  Check(queue.Enqueue(MakeCommand(22U)).get().status == humanoid::core::CommandStatus::Rejected,
        "Executor was allowed to self-join during shutdown");
  Check(queue.Enqueue(MakeCommand(23U)).get().isSuccess(),
        "Queue did not continue after executor failures");

  const humanoid::core::CommandQueueStatistics statistics = queue.GetStatistics();
  Check(statistics.accepted == 4U && statistics.completed == 1U && statistics.failed == 2U &&
            statistics.rejected == 1U,
        "Executor failure statistics are incorrect");
  Check(queue.Shutdown().isSuccess(), "Failure-isolation queue shutdown failed");
}

void UpdateMaximum(std::atomic_size_t& maximum, std::size_t candidate) {
  std::size_t observed = maximum.load(std::memory_order_relaxed);
  while (observed < candidate &&
         !maximum.compare_exchange_weak(observed, candidate, std::memory_order_relaxed)) {
  }
}

void TestConcurrentProducersAndConsumers() {
  constexpr std::size_t kProducerCount{8U};
  constexpr std::size_t kCommandsPerProducer{250U};
  constexpr std::size_t kCommandCount{kProducerCount * kCommandsPerProducer};
  constexpr std::size_t kConsumerCount{4U};

  std::atomic_size_t active_consumers{0U};
  std::atomic_size_t maximum_active_consumers{0U};
  std::atomic_size_t executed{0U};

  humanoid::core::CommandQueue queue{
      [&](const humanoid::core::Command&) {
        const std::size_t active = active_consumers.fetch_add(1U, std::memory_order_relaxed) + 1U;
        UpdateMaximum(maximum_active_consumers, active);
        std::this_thread::sleep_for(std::chrono::microseconds{100});
        active_consumers.fetch_sub(1U, std::memory_order_relaxed);
        executed.fetch_add(1U, std::memory_order_relaxed);
        return CompletedResult();
      },
      {kCommandCount, kConsumerCount}};

  std::vector<std::future<humanoid::core::CommandResult>> futures(kCommandCount);
  std::atomic_bool producer_failed{false};
  std::vector<std::jthread> producers;
  producers.reserve(kProducerCount);

  for (std::size_t producer = 0U; producer < kProducerCount; ++producer) {
    producers.emplace_back([&, producer]() {
      try {
        const std::size_t begin = producer * kCommandsPerProducer;
        const std::size_t end = begin + kCommandsPerProducer;
        for (std::size_t index = begin; index < end; ++index) {
          const auto priority = static_cast<humanoid::core::CommandPriority>(index % 4U);
          futures[index] = queue.Enqueue(MakeCommand(index + 1000U, priority));
        }
      } catch (...) {
        producer_failed.store(true, std::memory_order_relaxed);
      }
    });
  }
  producers.clear();

  Check(!producer_failed.load(std::memory_order_relaxed), "Concurrent producer failed");
  for (std::future<humanoid::core::CommandResult>& future : futures) {
    Check(future.valid(), "Concurrent producer did not return a future");
    Check(future.get().isSuccess(), "Concurrent command execution failed");
  }

  const humanoid::core::CommandQueueStatistics statistics = queue.GetStatistics();
  Check(executed.load(std::memory_order_relaxed) == kCommandCount,
        "Concurrent consumers lost commands");
  Check(maximum_active_consumers.load(std::memory_order_relaxed) > 1U,
        "Multiple consumers did not execute concurrently");
  Check(statistics.accepted == kCommandCount && statistics.completed == kCommandCount,
        "Concurrent queue statistics are incorrect");
  Check(statistics.failed == 0U && statistics.cancelled == 0U && statistics.timedOut == 0U &&
            statistics.rejected == 0U,
        "Concurrent queue reported unexpected terminal outcomes");
  Check(statistics.queued == 0U && statistics.active == 0U, "Concurrent queue did not drain");
  Check(queue.Shutdown().isSuccess(), "Concurrent queue shutdown failed");
  Check(queue.Shutdown().isSuccess(), "Idempotent queue shutdown failed");
}

} // namespace

int main() {
  try {
    TestConstructionValidation();
    TestPriorityAndFifoOrdering();
    TestCapacityTimeoutAndCancellation();
    TestExecutorFailureIsolation();
    TestConcurrentProducersAndConsumers();
  } catch (...) {
    return 1;
  }
  return 0;
}
