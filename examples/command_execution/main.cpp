/**
 * @file main.cpp
 * @brief Shows lifecycle-managed command execution without robot hardware.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <utility>

#include <humanoid/core/CommandExecutionPipeline.h>

namespace {

[[nodiscard]] humanoid::core::Command MakeStopCommand(humanoid::core::CommandId id) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = humanoid::core::CommandType::Stop;
  return command;
}

[[nodiscard]] humanoid::core::CommandResult Completed(std::string message) {
  humanoid::core::CommandResult result;
  result.status = humanoid::core::CommandStatus::Completed;
  result.message = std::move(message);
  return result;
}

} // namespace

int main() {
  humanoid::core::CommandExecutionPipeline pipeline{
      [](const humanoid::core::Command&) { return Completed("command executed"); },
      humanoid::core::CommandExecutionPipelineOptions{
          .maximumQueueSize = 8U,
          .workerCount = 1U,
          .maximumHistorySize = 8U,
      }};

  const auto subscription =
      pipeline.Subscribe([](const humanoid::core::CommandExecutionEvent& event) {
        std::cout << "execution " << event.executionId << " status "
                  << humanoid::core::toString(event.status) << '\n';
      });
  if (subscription == humanoid::core::CommandExecutionPipeline::kInvalidCallbackId) {
    return EXIT_FAILURE;
  }

  humanoid::core::CommandExecutionHandle handle = pipeline.Submit(MakeStopCommand(1U));
  const humanoid::core::CommandResult result = handle.result.get();
  if (!result.isSuccess()) {
    return EXIT_FAILURE;
  }

  const humanoid::core::CommandExecutionMetrics metrics = pipeline.GetMetrics();
  if (metrics.completed != 1U || metrics.historySize != 1U) {
    return EXIT_FAILURE;
  }

  if (!pipeline.Unsubscribe(subscription)) {
    return EXIT_FAILURE;
  }

  std::cout << "Command execution completed\n";
  return pipeline.Shutdown().isSuccess() ? EXIT_SUCCESS : EXIT_FAILURE;
}
