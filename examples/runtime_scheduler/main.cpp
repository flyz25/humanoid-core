/**
 * @file main.cpp
 * @brief Shows generic runtime scheduling without mission or robot logic.
 */

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

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

} // namespace

int main() {
  try {
    RuntimeScheduler scheduler{RuntimeSchedulerOptions{
        .maximumQueueSize = 8U,
        .workerCount = 2U,
        .maximumCompletedHistorySize = 8U,
    }};

    std::atomic<bool> pausable_running{false};
    std::atomic<bool> pausable_resumed{false};

    RuntimeJobHandle sequential = scheduler.Submit(MakeJob(
        1U, RuntimeJobPriority::High, RuntimeExecutionMode::Sequential,
        [](RuntimeJobContext& context) {
          context.Execution().SetMetadataValue("example", "sequential");
          return RuntimeJobResult{ExecutionState::Completed, "sequential runtime job completed"};
        }));

    RuntimeJobHandle pausable = scheduler.Submit(MakeJob(
        2U, RuntimeJobPriority::Normal, RuntimeExecutionMode::Parallel,
        [&pausable_running, &pausable_resumed](RuntimeJobContext& context) {
          pausable_running = true;
          while (!context.IsPaused() && !context.IsCancellationRequested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
          }
          if (!context.WaitIfPaused()) {
            return RuntimeJobResult{ExecutionState::Cancelled, "pausable runtime job cancelled"};
          }
          pausable_resumed = true;
          return RuntimeJobResult{ExecutionState::Completed, "pausable runtime job resumed"};
        }));

    while (!pausable_running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    Check(scheduler.Pause(2U), "failed to pause runtime job");
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
    Check(!pausable_resumed.load(), "runtime job resumed before resume request");
    Check(scheduler.Resume(2U), "failed to resume runtime job");

    Check(sequential.result.get().state == ExecutionState::Completed,
          "sequential runtime job failed");
    Check(pausable.result.get().state == ExecutionState::Completed, "pausable runtime job failed");

    const auto statistics = scheduler.GetStatistics();
    Check(statistics.completed == 2U, "scheduler completed count mismatch");

    std::cout << "RuntimeScheduler example completed=" << statistics.completed
              << " high_watermark=" << statistics.highWatermark << '\n';
    scheduler.Shutdown();
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
