/**
 * @file mission_executor_unit_test.cpp
 * @brief Validates mission execution through the command framework.
 */

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/mission/MissionExecutor.h>

namespace {

class MockRobotAdapter final : public humanoid::adapters::IRobotAdapter {
public:
  humanoid::adapters::Result Initialize() override { return Success("initialized"); }

  humanoid::adapters::Result Connect() override { return Success("connected"); }

  humanoid::adapters::Result Disconnect() override { return Success("disconnected"); }

  humanoid::adapters::Result Shutdown() override { return Success("shutdown"); }

  humanoid::adapters::Result StandUp() override { return Invoke("Stand"); }

  humanoid::adapters::Result BalanceStand() override { return Invoke("BalanceStand"); }

  humanoid::adapters::Result Move(float, float, float) override { return Invoke("Move"); }

  humanoid::adapters::Result Stop() override { return Invoke("Stop"); }

  humanoid::adapters::Result EmergencyStop() override { return Invoke("EmergencyStop"); }

  [[nodiscard]] humanoid::adapters::RobotStateResult GetRobotState() const override {
    humanoid::adapters::RobotState state;
    state.initialized = true;
    state.connected = true;
    state.connection_state = humanoid::adapters::RobotConnectionState::kConnected;
    return humanoid::adapters::RobotStateResult{Success("state"), state};
  }

  void BlockNextCommand() {
    std::lock_guard<std::mutex> lock{mutex_};
    block_next_command_ = true;
    blocked_command_started_ = false;
    release_blocked_command_ = false;
  }

  [[nodiscard]] bool WaitForBlockedCommand(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock{mutex_};
    return condition_.wait_for(lock, timeout, [this]() { return blocked_command_started_; });
  }

  void ReleaseBlockedCommand() {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      release_blocked_command_ = true;
    }
    condition_.notify_all();
  }

  void FailNextCommand() {
    std::lock_guard<std::mutex> lock{mutex_};
    fail_next_command_ = true;
  }

  [[nodiscard]] std::vector<std::string> Actions() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return actions_;
  }

private:
  static humanoid::adapters::Result Success(std::string message) {
    return {humanoid::adapters::ErrorCode::kSuccess, std::move(message)};
  }

  humanoid::adapters::Result Invoke(std::string action) {
    bool should_fail = false;
    {
      std::unique_lock<std::mutex> lock{mutex_};
      actions_.push_back(std::move(action));
      if (block_next_command_) {
        block_next_command_ = false;
        blocked_command_started_ = true;
        condition_.notify_all();
        condition_.wait(lock, [this]() { return release_blocked_command_; });
      }

      should_fail = fail_next_command_;
      fail_next_command_ = false;
    }

    if (should_fail) {
      return {humanoid::adapters::ErrorCode::kUnknown, "mock adapter failure"};
    }
    return Success("command completed");
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<std::string> actions_;
  bool block_next_command_{false};
  bool blocked_command_started_{false};
  bool release_blocked_command_{false};
  bool fail_next_command_{false};
};

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

[[nodiscard]] humanoid::core::Command MakeCommand(humanoid::core::CommandId id,
                                                  humanoid::core::CommandType type) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = type;
  return command;
}

[[nodiscard]] humanoid::mission::MissionStep MakeStep(humanoid::mission::MissionStepId step_id,
                                                      humanoid::core::Command command) {
  humanoid::mission::MissionStep step;
  step.id = step_id;
  step.name = "mission step";
  step.command = std::move(command);
  step.timeout = std::chrono::seconds{1};
  return step;
}

[[nodiscard]] humanoid::mission::Mission MakeMission() {
  humanoid::mission::Mission mission;
  mission.id = 10U;
  mission.name = "mission executor test";
  mission.version = "1.0.0";
  mission.author = "humanoid-core";
  mission.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  mission.steps.push_back(MakeStep(1U, MakeCommand(1U, humanoid::core::CommandType::Stand)));
  mission.steps.push_back(MakeStep(2U, MakeCommand(2U, humanoid::core::CommandType::Stop)));
  return mission;
}

[[nodiscard]] bool WaitForStatus(const humanoid::mission::MissionExecutor& executor,
                                 humanoid::mission::MissionStatus status,
                                 std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (executor.GetStatus() == status) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
  }
  return executor.GetStatus() == status;
}

struct ExecutorFixture final {
  std::shared_ptr<MockRobotAdapter> adapter{std::make_shared<MockRobotAdapter>()};
  std::shared_ptr<humanoid::core::CommandDispatcher> dispatcher{
      std::make_shared<humanoid::core::CommandDispatcher>(adapter)};
  humanoid::mission::MissionExecutor executor{dispatcher};
};

void TestExecuteStepUsesCommandDispatcher() {
  ExecutorFixture fixture;

  const humanoid::mission::MissionResult result = fixture.executor.ExecuteStep(
      MakeStep(1U, MakeCommand(1U, humanoid::core::CommandType::Stop)));

  Check(result.isSuccess(), "Mission step did not complete");
  Check(fixture.adapter->Actions() == std::vector<std::string>({"Stop"}),
        "Mission step did not use dispatcher adapter path");
  Check(fixture.executor.Stop().isSuccess(), "Executor stop failed");
  Check(fixture.dispatcher->Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestMissionCompletesAllEnabledSteps() {
  ExecutorFixture fixture;

  Check(fixture.executor.Start(MakeMission()).status == humanoid::mission::MissionStatus::Running,
        "Mission did not start");
  Check(WaitForStatus(fixture.executor, humanoid::mission::MissionStatus::Completed,
                      std::chrono::milliseconds{500}),
        "Mission did not complete");
  Check(fixture.executor.GetLastResult().isSuccess(), "Completed mission result is not success");
  Check(fixture.executor.GetCurrentStepIndex().has_value(), "Current step index was not tracked");
  Check(*fixture.executor.GetCurrentStepIndex() == 1U, "Unexpected final step index");
  Check(fixture.adapter->Actions() == std::vector<std::string>({"Stand", "Stop"}),
        "Mission did not execute expected steps");
  Check(fixture.executor.Stop().isSuccess(), "Executor stop failed");
  Check(fixture.dispatcher->Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestPauseAndResumeBetweenSteps() {
  ExecutorFixture fixture;
  fixture.adapter->BlockNextCommand();

  Check(fixture.executor.Start(MakeMission()).status == humanoid::mission::MissionStatus::Running,
        "Mission did not start");
  Check(fixture.adapter->WaitForBlockedCommand(std::chrono::milliseconds{500}),
        "First command did not start");
  Check(fixture.executor.Pause().status == humanoid::mission::MissionStatus::Paused,
        "Mission pause was not accepted");

  fixture.adapter->ReleaseBlockedCommand();
  Check(WaitForStatus(fixture.executor, humanoid::mission::MissionStatus::Paused,
                      std::chrono::milliseconds{500}),
        "Mission did not pause before next step");
  Check(fixture.adapter->Actions() == std::vector<std::string>({"Stand"}),
        "Mission executed a step while paused");

  Check(fixture.executor.Resume().status == humanoid::mission::MissionStatus::Running,
        "Mission resume was not accepted");
  Check(WaitForStatus(fixture.executor, humanoid::mission::MissionStatus::Completed,
                      std::chrono::milliseconds{500}),
        "Mission did not complete after resume");
  Check(fixture.adapter->Actions() == std::vector<std::string>({"Stand", "Stop"}),
        "Mission did not resume expected step execution");
  Check(fixture.executor.Stop().isSuccess(), "Executor stop failed");
  Check(fixture.dispatcher->Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestCancelStopsBeforeNextStep() {
  ExecutorFixture fixture;
  fixture.adapter->BlockNextCommand();

  Check(fixture.executor.Start(MakeMission()).status == humanoid::mission::MissionStatus::Running,
        "Mission did not start");
  Check(fixture.adapter->WaitForBlockedCommand(std::chrono::milliseconds{500}),
        "First command did not start");
  Check(fixture.executor.Cancel().status == humanoid::mission::MissionStatus::Cancelled,
        "Mission cancel was not accepted");

  fixture.adapter->ReleaseBlockedCommand();
  Check(WaitForStatus(fixture.executor, humanoid::mission::MissionStatus::Cancelled,
                      std::chrono::milliseconds{500}),
        "Mission did not cancel");
  Check(fixture.adapter->Actions() == std::vector<std::string>({"Stand"}),
        "Mission executed a step after cancellation");
  Check(fixture.executor.Stop().status == humanoid::mission::MissionStatus::Cancelled,
        "Executor stop did not preserve cancellation result");
  Check(fixture.dispatcher->Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestStepRetry() {
  ExecutorFixture fixture;
  humanoid::mission::MissionStep step =
      MakeStep(1U, MakeCommand(1U, humanoid::core::CommandType::Stop));
  step.retry = 1U;
  fixture.adapter->FailNextCommand();

  const humanoid::mission::MissionResult result = fixture.executor.ExecuteStep(step);

  Check(result.isSuccess(), "Mission step retry did not recover");
  Check(fixture.adapter->Actions() == std::vector<std::string>({"Stop", "Stop"}),
        "Mission step retry did not dispatch twice");
  Check(fixture.executor.Stop().isSuccess(), "Executor stop failed");
  Check(fixture.dispatcher->Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

} // namespace

int main() {
  try {
    TestExecuteStepUsesCommandDispatcher();
    TestMissionCompletesAllEnabledSteps();
    TestPauseAndResumeBetweenSteps();
    TestCancelStopsBeforeNextStep();
    TestStepRetry();
  } catch (...) {
    return 1;
  }

  return 0;
}
