/**
 * @file runtime_scheduler_unit_test.cpp
 * @brief Validates vendor-independent runtime scheduler behavior.
 */

#include <algorithm>
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/runtime/RuntimeScheduler.h>

namespace {

using humanoid::runtime::ExecutionState;
using humanoid::runtime::RuntimeExecutionMode;
using humanoid::runtime::RuntimeJob;
using humanoid::runtime::RuntimeJobContext;
using humanoid::runtime::RuntimeJobHandle;
using humanoid::runtime::RuntimeJobPriority;
using humanoid::runtime::RuntimeJobResult;
using humanoid::runtime::RuntimeScheduler;
using humanoid::runtime::RuntimeSchedulerOptions;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

RuntimeJob MakeJob(std::uint64_t id, RuntimeJobPriority priority, RuntimeExecutionMode mode,
                   humanoid::runtime::RuntimeJobCallback callback) {
  RuntimeJob job;
  job.id = id;
  job.priority = priority;
  job.executionMode = mode;
  job.callback = std::move(callback);
  return job;
}

void TestPriorityAndFifoScheduling() {
  RuntimeScheduler scheduler{RuntimeSchedulerOptions{8U, 1U, 16U}};
  std::mutex mutex;
  std::vector<std::uint64_t> order;
  std::atomic<bool> blocker_running{false};
  std::atomic<bool> release_blocker{false};

  RuntimeJobHandle blocker =
      scheduler.Submit(MakeJob(99U, RuntimeJobPriority::Normal, RuntimeExecutionMode::Parallel,
                               [&blocker_running, &release_blocker](RuntimeJobContext&) {
                                 blocker_running = true;
                                 while (!release_blocker.load()) {
                                   std::this_thread::sleep_for(std::chrono::milliseconds{1});
                                 }
                                 return RuntimeJobResult{ExecutionState::Completed, "blocker"};
                               }));

  while (!blocker_running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  RuntimeJobHandle low =
      scheduler.Submit(MakeJob(1U, RuntimeJobPriority::Low, RuntimeExecutionMode::Parallel,
                               [&order, &mutex](RuntimeJobContext& context) {
                                 std::lock_guard<std::mutex> lock{mutex};
                                 order.push_back(context.JobId());
                                 return RuntimeJobResult{ExecutionState::Completed, "low"};
                               }));
  RuntimeJobHandle high_a =
      scheduler.Submit(MakeJob(2U, RuntimeJobPriority::High, RuntimeExecutionMode::Parallel,
                               [&order, &mutex](RuntimeJobContext& context) {
                                 std::lock_guard<std::mutex> lock{mutex};
                                 order.push_back(context.JobId());
                                 return RuntimeJobResult{ExecutionState::Completed, "high-a"};
                               }));
  RuntimeJobHandle high_b =
      scheduler.Submit(MakeJob(3U, RuntimeJobPriority::High, RuntimeExecutionMode::Parallel,
                               [&order, &mutex](RuntimeJobContext& context) {
                                 std::lock_guard<std::mutex> lock{mutex};
                                 order.push_back(context.JobId());
                                 return RuntimeJobResult{ExecutionState::Completed, "high-b"};
                               }));

  release_blocker = true;
  Check(blocker.result.get().state == ExecutionState::Completed, "Blocker job failed");
  Check(low.result.get().state == ExecutionState::Completed, "Low priority job failed");
  Check(high_a.result.get().state == ExecutionState::Completed, "First high priority job failed");
  Check(high_b.result.get().state == ExecutionState::Completed, "Second high priority job failed");
  Check(order.size() == 3U, "Unexpected priority test execution count");
  Check(order[0] == 2U, "Highest priority job did not execute first");
  Check(order[1] == 3U, "FIFO order inside equal priority was not preserved");
  Check(order[2] == 1U, "Low priority job did not execute last");
}

void TestParallelExecution() {
  RuntimeScheduler scheduler{RuntimeSchedulerOptions{8U, 3U, 16U}};
  std::barrier<> start_barrier{3};
  std::atomic<int> active{0};
  std::atomic<int> maximum_active{0};
  std::vector<RuntimeJobHandle> handles;
  handles.reserve(3U);

  for (std::uint64_t id = 10U; id < 13U; ++id) {
    handles.push_back(scheduler.Submit(MakeJob(
        id, RuntimeJobPriority::Normal, RuntimeExecutionMode::Parallel,
        [&start_barrier, &active, &maximum_active](RuntimeJobContext&) {
          start_barrier.arrive_and_wait();
          const int current = ++active;
          int observed = maximum_active.load();
          while (current > observed && !maximum_active.compare_exchange_weak(observed, current)) {
          }
          std::this_thread::sleep_for(std::chrono::milliseconds{20});
          --active;
          return RuntimeJobResult{ExecutionState::Completed, "parallel"};
        })));
  }

  for (RuntimeJobHandle& handle : handles) {
    Check(handle.result.get().state == ExecutionState::Completed, "Parallel job failed");
  }
  Check(maximum_active.load() > 1, "Parallel jobs did not execute concurrently");
  const auto statistics = scheduler.GetStatistics();
  Check(statistics.maximumConcurrentRunning > 1U, "Scheduler did not record concurrent execution");
}

void TestSequentialExecutionIsExclusive() {
  RuntimeScheduler scheduler{RuntimeSchedulerOptions{8U, 3U, 16U}};
  std::atomic<int> active{0};
  std::atomic<int> maximum_active{0};
  std::vector<RuntimeJobHandle> handles;
  handles.reserve(3U);

  for (std::uint64_t id = 20U; id < 23U; ++id) {
    handles.push_back(scheduler.Submit(MakeJob(
        id, RuntimeJobPriority::Normal, RuntimeExecutionMode::Sequential,
        [&active, &maximum_active](RuntimeJobContext&) {
          const int current = ++active;
          int observed = maximum_active.load();
          while (current > observed && !maximum_active.compare_exchange_weak(observed, current)) {
          }
          std::this_thread::sleep_for(std::chrono::milliseconds{10});
          --active;
          return RuntimeJobResult{ExecutionState::Completed, "sequential"};
        })));
  }

  for (RuntimeJobHandle& handle : handles) {
    Check(handle.result.get().state == ExecutionState::Completed, "Sequential job failed");
  }
  Check(maximum_active.load() == 1, "Sequential jobs overlapped");
}

void TestPauseAndResume() {
  RuntimeScheduler scheduler{RuntimeSchedulerOptions{4U, 1U, 16U}};
  std::atomic<bool> running{false};
  std::atomic<bool> passed_pause{false};

  RuntimeJobHandle handle =
      scheduler.Submit(MakeJob(30U, RuntimeJobPriority::Normal, RuntimeExecutionMode::Parallel,
                               [&running, &passed_pause](RuntimeJobContext& context) {
                                 running = true;
                                 while (!context.IsPaused()) {
                                   std::this_thread::sleep_for(std::chrono::milliseconds{1});
                                 }
                                 if (!context.WaitIfPaused()) {
                                   return RuntimeJobResult{ExecutionState::Cancelled, "cancelled"};
                                 }
                                 passed_pause = true;
                                 return RuntimeJobResult{ExecutionState::Completed, "resumed"};
                               }));

  while (!running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  Check(scheduler.Pause(30U), "Pause request failed");
  std::this_thread::sleep_for(std::chrono::milliseconds{10});
  Check(!passed_pause.load(), "Job passed pause before resume");
  Check(scheduler.Resume(30U), "Resume request failed");
  Check(handle.result.get().state == ExecutionState::Completed, "Paused job did not complete");
  Check(passed_pause.load(), "Job did not continue after resume");
}

void TestStopRunningJob() {
  RuntimeScheduler scheduler{RuntimeSchedulerOptions{4U, 1U, 16U}};
  std::atomic<bool> running{false};

  RuntimeJobHandle handle =
      scheduler.Submit(MakeJob(40U, RuntimeJobPriority::Normal, RuntimeExecutionMode::Parallel,
                               [&running](RuntimeJobContext& context) {
                                 running = true;
                                 while (!context.IsCancellationRequested()) {
                                   std::this_thread::sleep_for(std::chrono::milliseconds{1});
                                 }
                                 return RuntimeJobResult{ExecutionState::Cancelled, "stopped"};
                               }));

  while (!running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  Check(scheduler.Stop(40U), "Stop request failed");
  Check(handle.result.get().state == ExecutionState::Cancelled, "Stopped job was not cancelled");
}

void TestConcurrentSubmission() {
  constexpr int kProducerCount = 4;
  constexpr int kJobsPerProducer = 25;
  RuntimeScheduler scheduler{RuntimeSchedulerOptions{128U, 4U, 128U}};
  std::barrier<> start_barrier{kProducerCount};
  std::atomic<int> completed{0};
  std::mutex handles_mutex;
  std::vector<RuntimeJobHandle> handles;
  handles.reserve(kProducerCount * kJobsPerProducer);
  std::vector<std::jthread> producers;
  producers.reserve(kProducerCount);

  for (int producer = 0; producer < kProducerCount; ++producer) {
    producers.emplace_back(
        [producer, &scheduler, &start_barrier, &completed, &handles, &handles_mutex]() {
          start_barrier.arrive_and_wait();
          for (int index = 0; index < kJobsPerProducer; ++index) {
            const std::uint64_t id =
                1000U + static_cast<std::uint64_t>(producer * kJobsPerProducer + index);
            RuntimeJobHandle handle = scheduler.Submit(
                MakeJob(id, RuntimeJobPriority::Normal, RuntimeExecutionMode::Parallel,
                        [&completed](RuntimeJobContext&) {
                          ++completed;
                          return RuntimeJobResult{ExecutionState::Completed, "complete"};
                        }));
            std::lock_guard<std::mutex> lock{handles_mutex};
            handles.push_back(std::move(handle));
          }
        });
  }

  producers.clear();
  for (RuntimeJobHandle& handle : handles) {
    Check(handle.result.get().state == ExecutionState::Completed, "Concurrent job failed");
  }
  Check(completed.load() == kProducerCount * kJobsPerProducer, "Concurrent submission missed jobs");
  Check(scheduler.GetStatistics().accepted == static_cast<std::uint64_t>(completed.load()),
        "Accepted statistics mismatch after concurrent submission");
}

} // namespace

int main() {
  try {
    TestPriorityAndFifoScheduling();
    TestParallelExecution();
    TestSequentialExecutionIsExclusive();
    TestPauseAndResume();
    TestStopRunningJob();
    TestConcurrentSubmission();
  } catch (...) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
