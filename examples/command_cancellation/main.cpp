/**
 * @file main.cpp
 * @brief Shows queued command cancellation without robot hardware.
 */

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>

#include <humanoid/core/CommandExecutionPipeline.h>

namespace {

[[nodiscard]] humanoid::core::Command MakeCommand(humanoid::core::CommandId id) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = humanoid::core::CommandType::Stop;
  return command;
}

[[nodiscard]] humanoid::core::CommandResult Completed() {
  humanoid::core::CommandResult result;
  result.status = humanoid::core::CommandStatus::Completed;
  result.message = "command executed";
  return result;
}

class ExecutionGate final {
public:
  void WaitInsideExecutor() {
    std::unique_lock<std::mutex> lock{mutex_};
    started_ = true;
    condition_.notify_all();
    condition_.wait(lock, [this]() { return released_; });
  }

  [[nodiscard]] bool WaitUntilStarted(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock{mutex_};
    return condition_.wait_for(lock, timeout, [this]() { return started_; });
  }

  void Release() {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      released_ = true;
    }
    condition_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable condition_;
  bool started_{false};
  bool released_{false};
};

} // namespace

int main() {
  ExecutionGate gate;
  humanoid::core::CommandExecutionPipeline pipeline{
      [&gate](const humanoid::core::Command& command) {
        if (command.id == 1U) {
          gate.WaitInsideExecutor();
        }
        return Completed();
      },
      humanoid::core::CommandExecutionPipelineOptions{
          .maximumQueueSize = 4U,
          .workerCount = 1U,
          .maximumHistorySize = 8U,
      }};

  humanoid::core::CommandExecutionHandle running = pipeline.Submit(MakeCommand(1U));
  if (!gate.WaitUntilStarted(std::chrono::milliseconds{500})) {
    return EXIT_FAILURE;
  }

  humanoid::core::CommandExecutionHandle queued = pipeline.Submit(MakeCommand(2U));
  const humanoid::core::CommandResult cancellation = pipeline.Cancel(queued.executionId);
  if (cancellation.status != humanoid::core::CommandStatus::Cancelled) {
    return EXIT_FAILURE;
  }

  gate.Release();
  if (!running.result.get().isSuccess()) {
    return EXIT_FAILURE;
  }
  if (queued.result.get().status != humanoid::core::CommandStatus::Cancelled) {
    return EXIT_FAILURE;
  }

  std::cout << "Queued command cancelled\n";
  return pipeline.Shutdown().isSuccess() ? EXIT_SUCCESS : EXIT_FAILURE;
}
