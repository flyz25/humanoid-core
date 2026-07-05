/**
 * @file main.cpp
 * @brief Loads and executes mission YAML without robot hardware.
 */

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionLoader.h>

namespace {

enum class ExecutionMode { Execute, PauseResume, Cancel };

class ExampleRobotAdapter final : public humanoid::adapters::IRobotAdapter {
public:
  humanoid::adapters::Result Initialize() override {
    std::lock_guard<std::mutex> lock{mutex_};
    initialized_ = true;
    return Success("Example adapter initialized");
  }

  humanoid::adapters::Result Connect() override {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!initialized_) {
      return Failure("Example adapter is not initialized");
    }
    connected_ = true;
    return Success("Example adapter connected");
  }

  humanoid::adapters::Result Disconnect() override {
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    return Success("Example adapter disconnected");
  }

  humanoid::adapters::Result Shutdown() override {
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    initialized_ = false;
    return Success("Example adapter shut down");
  }

  humanoid::adapters::Result StandUp() override { return Record("Stand"); }

  humanoid::adapters::Result BalanceStand() override { return Record("BalanceStand"); }

  humanoid::adapters::Result Move(float vx, float vy, float omega) override {
    std::cout << "adapter: Move(" << vx << ", " << vy << ", " << omega << ")\n";
    return ConnectedResult("Move");
  }

  humanoid::adapters::Result Stop() override { return Record("Stop"); }

  humanoid::adapters::Result EmergencyStop() override { return Record("EmergencyStop"); }

  [[nodiscard]] humanoid::adapters::RobotStateResult GetRobotState() const override {
    std::lock_guard<std::mutex> lock{mutex_};
    humanoid::adapters::RobotState state;
    state.vendor = "Example";
    state.model = "ProcessLocal";
    state.initialized = initialized_;
    state.connected = connected_;
    if (connected_) {
      state.connection_state = humanoid::adapters::RobotConnectionState::kConnected;
    } else if (initialized_) {
      state.connection_state = humanoid::adapters::RobotConnectionState::kDisconnected;
    }
    return {Success("Example state returned"), std::move(state)};
  }

private:
  [[nodiscard]] static humanoid::adapters::Result Success(std::string message) {
    return {humanoid::adapters::ErrorCode::kSuccess, std::move(message)};
  }

  [[nodiscard]] static humanoid::adapters::Result Failure(std::string message) {
    return {humanoid::adapters::ErrorCode::kConnectionFailed, std::move(message)};
  }

  humanoid::adapters::Result ConnectedResult(std::string_view action) const {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!connected_) {
      return Failure(std::string{action} + " rejected: adapter is disconnected");
    }
    return Success(std::string{action} + " completed");
  }

  humanoid::adapters::Result Record(std::string_view action) const {
    std::cout << "adapter: " << action << '\n';
    return ConnectedResult(action);
  }

  mutable std::mutex mutex_;
  bool initialized_{false};
  bool connected_{false};
};

[[nodiscard]] bool Succeeded(const humanoid::adapters::Result& result) {
  if (!result.Succeeded()) {
    std::cerr << result.message << '\n';
    return false;
  }
  return true;
}

[[nodiscard]] bool WaitForStep(const humanoid::mission::MissionExecutor& executor,
                               std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (executor.GetCurrentStepIndex().has_value()) {
      return true;
    }
    if (humanoid::mission::isTerminal(executor.GetStatus())) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
  }
  return executor.GetCurrentStepIndex().has_value();
}

[[nodiscard]] bool WaitForTerminal(const humanoid::mission::MissionExecutor& executor,
                                   std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (humanoid::mission::isTerminal(executor.GetStatus())) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
  }
  return humanoid::mission::isTerminal(executor.GetStatus());
}

[[nodiscard]] bool ParseMode(std::string_view value, ExecutionMode& mode) noexcept {
  if (value == "execute") {
    mode = ExecutionMode::Execute;
    return true;
  }
  if (value == "pause-resume") {
    mode = ExecutionMode::PauseResume;
    return true;
  }
  if (value == "cancel") {
    mode = ExecutionMode::Cancel;
    return true;
  }
  return false;
}

[[nodiscard]] bool ApplyLifecycleAction(humanoid::mission::MissionExecutor& executor,
                                        ExecutionMode mode) {
  if (mode == ExecutionMode::Execute) {
    return true;
  }
  if (!WaitForStep(executor, std::chrono::seconds{1})) {
    std::cerr << "Mission reached no executable step\n";
    return false;
  }

  if (mode == ExecutionMode::Cancel) {
    const humanoid::mission::MissionResult result = executor.Cancel();
    std::cout << "mission: " << humanoid::mission::toString(result.status) << '\n';
    return result.status == humanoid::mission::MissionStatus::Cancelled;
  }

  const humanoid::mission::MissionResult pause_result = executor.Pause();
  std::cout << "mission: " << humanoid::mission::toString(pause_result.status) << '\n';
  if (pause_result.status != humanoid::mission::MissionStatus::Paused) {
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds{50});
  const humanoid::mission::MissionResult resume_result = executor.Resume();
  std::cout << "mission: " << humanoid::mission::toString(resume_result.status) << '\n';
  return resume_result.status == humanoid::mission::MissionStatus::Running;
}

int Run(const std::filesystem::path& mission_path, ExecutionMode mode) {
  const humanoid::mission::MissionLoader loader;
  humanoid::mission::MissionLoadResult load_result = loader.LoadFile(mission_path);
  if (!load_result.Succeeded()) {
    std::cerr << "Mission load failed: " << load_result.message << '\n';
    return EXIT_FAILURE;
  }
  std::cout << "loaded: " << load_result.mission.name << '\n';

  auto adapter = std::make_shared<ExampleRobotAdapter>();
  if (!Succeeded(adapter->Initialize())) {
    return EXIT_FAILURE;
  }
  if (!Succeeded(adapter->Connect())) {
    static_cast<void>(adapter->Shutdown());
    return EXIT_FAILURE;
  }

  auto dispatcher = std::make_shared<humanoid::core::CommandDispatcher>(adapter);
  humanoid::mission::MissionExecutor executor{dispatcher};
  const humanoid::mission::MissionResult start_result =
      executor.Start(std::move(load_result.mission));
  if (start_result.status != humanoid::mission::MissionStatus::Running) {
    std::cerr << "Mission start failed: " << start_result.message << '\n';
    static_cast<void>(dispatcher->Shutdown());
    static_cast<void>(adapter->Disconnect());
    static_cast<void>(adapter->Shutdown());
    return EXIT_FAILURE;
  }

  bool success =
      ApplyLifecycleAction(executor, mode) && WaitForTerminal(executor, std::chrono::seconds{5});
  const humanoid::mission::MissionResult final_result = executor.GetLastResult();
  std::cout << "final: " << humanoid::mission::toString(final_result.status) << " - "
            << final_result.message << '\n';
  const humanoid::mission::MissionStatus expected =
      mode == ExecutionMode::Cancel ? humanoid::mission::MissionStatus::Cancelled
                                    : humanoid::mission::MissionStatus::Completed;
  success = success && final_result.status == expected;

  static_cast<void>(executor.Stop());
  success = dispatcher->Shutdown().isSuccess() && success;
  success = Succeeded(adapter->Disconnect()) && success;
  success = Succeeded(adapter->Shutdown()) && success;
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: humanoid_core_mission_execution_example "
                 "<mission.yaml> <execute|pause-resume|cancel>\n";
    return EXIT_FAILURE;
  }

  ExecutionMode mode = ExecutionMode::Execute;
  if (!ParseMode(argv[2], mode)) {
    std::cerr << "Unknown execution mode: " << argv[2] << '\n';
    return EXIT_FAILURE;
  }

  try {
    return Run(argv[1], mode);
  } catch (const std::exception& exception) {
    std::cerr << "Mission example failed: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "Mission example failed with an unknown error\n";
  }
  return EXIT_FAILURE;
}
