/**
 * @file main.cpp
 * @brief Shows priority-aware command queue execution without robot hardware.
 */

#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

#include <humanoid/core/CommandQueue.h>

namespace {

[[nodiscard]] humanoid::core::Command MakeCommand(humanoid::core::CommandId id,
                                                  humanoid::core::CommandPriority priority) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = humanoid::core::CommandType::Stop;
  command.priority = priority;
  return command;
}

[[nodiscard]] humanoid::core::CommandResult Completed() {
  humanoid::core::CommandResult result;
  result.status = humanoid::core::CommandStatus::Completed;
  result.message = "queued command executed";
  return result;
}

} // namespace

int main() {
  std::mutex order_mutex;
  std::vector<humanoid::core::CommandId> execution_order;

  humanoid::core::CommandQueue queue{
      [&order_mutex, &execution_order](const humanoid::core::Command& command) {
        std::lock_guard<std::mutex> lock{order_mutex};
        execution_order.push_back(command.id);
        return Completed();
      },
      humanoid::core::CommandQueueOptions{
          .maximumQueueSize = 8U,
          .workerCount = 1U,
      }};

  std::vector<std::future<humanoid::core::CommandResult>> futures;
  futures.push_back(queue.Enqueue(MakeCommand(1U, humanoid::core::CommandPriority::Low)));
  futures.push_back(queue.Enqueue(MakeCommand(2U, humanoid::core::CommandPriority::Critical)));
  futures.push_back(queue.Enqueue(MakeCommand(3U, humanoid::core::CommandPriority::Normal)));

  for (std::future<humanoid::core::CommandResult>& future : futures) {
    if (!future.get().isSuccess()) {
      return EXIT_FAILURE;
    }
  }

  const humanoid::core::CommandQueueStatistics statistics = queue.GetStatistics();
  if (statistics.completed != 3U || statistics.failed != 0U) {
    return EXIT_FAILURE;
  }

  for (const humanoid::core::CommandId command_id : execution_order) {
    std::cout << "executed command " << command_id << '\n';
  }

  return queue.Shutdown().isSuccess() ? EXIT_SUCCESS : EXIT_FAILURE;
}
