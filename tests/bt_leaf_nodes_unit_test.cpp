/**
 * @file bt_leaf_nodes_unit_test.cpp
 * @brief Validates reusable vendor-independent behavior tree leaf nodes.
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/bt/ActionNode.h>
#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/CommandNode.h>
#include <humanoid/bt/ConditionNode.h>
#include <humanoid/bt/DelayNode.h>
#include <humanoid/bt/MissionNode.h>
#include <humanoid/bt/WaitNode.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/mission/MissionExecutor.h>

namespace {

using humanoid::bt::ActionNode;
using humanoid::bt::BTContext;
using humanoid::bt::BTStatus;
using humanoid::bt::CommandNode;
using humanoid::bt::ConditionNode;
using humanoid::bt::DelayNode;
using humanoid::bt::MissionNode;
using humanoid::bt::WaitNode;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

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

  [[nodiscard]] std::vector<std::string> Actions() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return actions_;
  }

private:
  static humanoid::adapters::Result Success(std::string message) {
    return {humanoid::adapters::ErrorCode::kSuccess, std::move(message)};
  }

  humanoid::adapters::Result Invoke(std::string action) {
    {
      std::unique_lock<std::mutex> lock{mutex_};
      actions_.push_back(std::move(action));
      if (block_next_command_) {
        block_next_command_ = false;
        blocked_command_started_ = true;
        condition_.notify_all();
        condition_.wait(lock, [this]() { return release_blocked_command_; });
      }
    }

    return Success("command completed");
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<std::string> actions_;
  bool block_next_command_{false};
  bool blocked_command_started_{false};
  bool release_blocked_command_{false};
};

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
  step.name = "bt mission step";
  step.command = std::move(command);
  step.timeout = std::chrono::seconds{1};
  return step;
}

[[nodiscard]] humanoid::mission::Mission MakeMission() {
  humanoid::mission::Mission mission;
  mission.id = 8400U;
  mission.name = "bt mission";
  mission.version = "1.0.0";
  mission.author = "humanoid-core";
  mission.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  mission.steps.push_back(MakeStep(1U, MakeCommand(101U, humanoid::core::CommandType::Stop)));
  return mission;
}

[[nodiscard]] BTStatus TickUntilTerminal(auto& node, BTContext& context,
                                         std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  BTStatus status = BTStatus::Running;
  while (std::chrono::steady_clock::now() < deadline) {
    status = node.Tick(context);
    if (status != BTStatus::Running) {
      return status;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
  }
  return status;
}

void TestActionExecution() {
  BTContext context;
  std::atomic<int> ticks{0};
  ActionNode action{"Action", [&ticks](BTContext&) {
                      ++ticks;
                      return BTStatus::Success;
                    }};

  Check(action.Initialize(context) == BTStatus::Idle, "action initialization failed");
  Check(action.Tick(context) == BTStatus::Success, "action did not return success");
  Check(ticks.load() == 1, "action callback did not execute exactly once");
}

void TestConditionAndWaitUseRuntimeContext() {
  BTContext context;
  Check(context.Blackboard()->Store<bool>("bt", "ready", false), "failed to seed blackboard");

  auto ready_predicate = [](const BTContext& node_context) {
    const std::shared_ptr<const bool> ready = node_context.Blackboard()->Get<bool>("bt", "ready");
    return ready && *ready;
  };

  ConditionNode condition{"ReadyCondition", ready_predicate};
  Check(condition.Initialize(context) == BTStatus::Idle, "condition initialization failed");
  Check(condition.Tick(context) == BTStatus::Failure, "condition did not observe false value");

  WaitNode wait{"ReadyWait", ready_predicate};
  Check(wait.Initialize(context) == BTStatus::Idle, "wait initialization failed");
  Check(wait.Tick(context) == BTStatus::Running, "wait did not run while predicate was false");
  Check(context.Blackboard()->Store<bool>("bt", "ready", true), "failed to update blackboard");
  Check(condition.Tick(context) == BTStatus::Success, "condition did not observe true value");
  Check(wait.Tick(context) == BTStatus::Success, "wait did not complete when predicate was true");
}

void TestDelayCompletesWithoutBusyWaiting() {
  BTContext context;
  DelayNode delay{"Delay", std::chrono::milliseconds{20}};

  Check(delay.Initialize(context) == BTStatus::Idle, "delay initialization failed");
  Check(delay.Tick(context) == BTStatus::Running, "delay did not run on first tick");
  std::this_thread::sleep_for(std::chrono::milliseconds{25});
  Check(delay.Tick(context) == BTStatus::Success, "delay did not complete after duration");
}

void TestCommandNodeUsesDispatcher() {
  auto adapter = std::make_shared<MockRobotAdapter>();
  adapter->BlockNextCommand();
  auto dispatcher = std::make_shared<humanoid::core::CommandDispatcher>(adapter);
  BTContext context;
  CommandNode command{"StopCommand", dispatcher,
                      MakeCommand(201U, humanoid::core::CommandType::Stop)};

  Check(command.Initialize(context) == BTStatus::Idle, "command node initialization failed");
  Check(command.Tick(context) == BTStatus::Running, "command node did not start asynchronously");
  Check(adapter->WaitForBlockedCommand(std::chrono::milliseconds{500}),
        "command did not reach adapter through dispatcher");
  adapter->ReleaseBlockedCommand();
  Check(TickUntilTerminal(command, context, std::chrono::milliseconds{500}) == BTStatus::Success,
        "command node did not complete successfully");
  Check(adapter->Actions() == std::vector<std::string>({"Stop"}),
        "command node did not use expected adapter command");
  Check(dispatcher->Shutdown().isSuccess(), "dispatcher shutdown failed");
}

void TestMissionNodeUsesMissionExecutor() {
  auto adapter = std::make_shared<MockRobotAdapter>();
  auto dispatcher = std::make_shared<humanoid::core::CommandDispatcher>(adapter);
  auto executor = std::make_shared<humanoid::mission::MissionExecutor>(dispatcher);
  BTContext context;
  MissionNode mission{"Mission", executor, MakeMission()};

  Check(mission.Initialize(context) == BTStatus::Idle, "mission node initialization failed");
  Check(mission.Tick(context) == BTStatus::Running, "mission node did not start mission");
  Check(TickUntilTerminal(mission, context, std::chrono::milliseconds{500}) == BTStatus::Success,
        "mission node did not complete successfully");
  Check(adapter->Actions() == std::vector<std::string>({"Stop"}),
        "mission node did not execute through mission executor");
  Check(executor->Stop().isSuccess(), "executor stop failed");
  Check(dispatcher->Shutdown().isSuccess(), "dispatcher shutdown failed");
}

} // namespace

int main() {
  try {
    TestActionExecution();
    TestConditionAndWaitUseRuntimeContext();
    TestDelayCompletesWithoutBusyWaiting();
    TestCommandNodeUsesDispatcher();
    TestMissionNodeUsesMissionExecutor();
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    (void)exception;
    return EXIT_FAILURE;
  }
}
