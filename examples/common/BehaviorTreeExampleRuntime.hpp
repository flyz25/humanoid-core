#pragma once

/**
 * @file BehaviorTreeExampleRuntime.hpp
 * @brief Provides hardware-free composition support for behavior tree examples.
 */

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/bt/BehaviorTreeFactory.h>
#include <humanoid/bt/BehaviorTreeRuntime.h>
#include <humanoid/core/Command.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandType.h>
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionStep.h>
#include <humanoid/runtime/Blackboard.h>
#include <humanoid/runtime/ExecutionState.h>
#include <humanoid/runtime/ResourceManager.h>
#include <humanoid/runtime/RuntimeScheduler.h>

namespace humanoid::examples {

/**
 * @brief Process-local adapter used only by hardware-free example applications.
 *
 * The adapter implements the public robot boundary and records generic command
 * execution. Behavior tree nodes still reach it only through
 * `CommandDispatcher` and `MissionExecutor`.
 */
class ExampleRobotAdapter final : public adapters::IRobotAdapter {
public:
  /** @brief Initializes process-local adapter state. */
  adapters::Result Initialize() override {
    std::lock_guard<std::mutex> lock{mutex_};
    initialized_ = true;
    return Success("example adapter initialized");
  }

  /** @brief Connects process-local adapter state. */
  adapters::Result Connect() override {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!initialized_) {
      return Failure("example adapter is not initialized");
    }
    connected_ = true;
    return Success("example adapter connected");
  }

  /** @brief Disconnects process-local adapter state. */
  adapters::Result Disconnect() override {
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    return Success("example adapter disconnected");
  }

  /** @brief Releases process-local adapter state. */
  adapters::Result Shutdown() override {
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    initialized_ = false;
    return Success("example adapter shut down");
  }

  /** @brief Records a generic stand command. */
  adapters::Result StandUp() override { return Record("Stand"); }

  /** @brief Records a generic balanced-stand command. */
  adapters::Result BalanceStand() override { return Record("BalanceStand"); }

  /** @brief Records a generic velocity command. */
  adapters::Result Move(float linear_x, float linear_y, float angular_z) override {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!connected_) {
      return Failure("Move rejected: example adapter is disconnected");
    }
    std::cout << "adapter: Move(" << linear_x << ", " << linear_y << ", " << angular_z << ")\n";
    return Success("Move completed");
  }

  /** @brief Records a generic stop command. */
  adapters::Result Stop() override { return Record("Stop"); }

  /** @brief Records a generic emergency-stop command. */
  adapters::Result EmergencyStop() override { return Record("EmergencyStop"); }

  /** @brief Returns process-local connection state. */
  [[nodiscard]] adapters::RobotStateResult GetRobotState() const override {
    std::lock_guard<std::mutex> lock{mutex_};
    adapters::RobotState state;
    state.vendor = "Example";
    state.model = "ProcessLocal";
    state.initialized = initialized_;
    state.connected = connected_;
    state.connection_state = connected_ ? adapters::RobotConnectionState::kConnected
                                        : adapters::RobotConnectionState::kDisconnected;
    return {Success("example state returned"), std::move(state)};
  }

private:
  [[nodiscard]] static adapters::Result Success(std::string message) {
    return {adapters::ErrorCode::kSuccess, std::move(message)};
  }

  [[nodiscard]] static adapters::Result Failure(std::string message) {
    return {adapters::ErrorCode::kConnectionFailed, std::move(message)};
  }

  adapters::Result Record(std::string_view command) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!connected_) {
      return Failure(std::string{command} + " rejected: example adapter is disconnected");
    }
    std::cout << "adapter: " << command << '\n';
    return Success(std::string{command} + " completed");
  }

  mutable std::mutex mutex_;
  bool initialized_{false};
  bool connected_{false};
};

/**
 * @brief Owns the shared framework services used by one example process.
 */
class BehaviorTreeExampleRuntime final {
public:
  /**
   * @brief Initializes the process-local adapter and runtime service graph.
   *
   * @throws std::runtime_error when adapter lifecycle initialization fails.
   */
  BehaviorTreeExampleRuntime()
      : adapter_(std::make_shared<ExampleRobotAdapter>()),
        dispatcher_(std::make_shared<core::CommandDispatcher>(adapter_)),
        mission_executor_(std::make_shared<mission::MissionExecutor>(dispatcher_)),
        scheduler_(std::make_shared<runtime::RuntimeScheduler>(
            runtime::RuntimeSchedulerOptions{16U, 4U, 16U})),
        blackboard_(std::make_shared<runtime::Blackboard>()),
        resource_manager_(std::make_shared<runtime::ResourceManager>()),
        factory_(std::make_shared<bt::BehaviorTreeFactory>()),
        behavior_tree_runtime_(std::make_unique<bt::BehaviorTreeRuntime>(
            scheduler_, blackboard_, resource_manager_, factory_)) {
    const adapters::Result initialize_result = adapter_->Initialize();
    if (!initialize_result.Succeeded()) {
      throw std::runtime_error{initialize_result.message};
    }
    const adapters::Result connect_result = adapter_->Connect();
    if (!connect_result.Succeeded()) {
      static_cast<void>(adapter_->Shutdown());
      throw std::runtime_error{connect_result.message};
    }
  }

  /** @brief Stops runtime services and releases adapter state. */
  ~BehaviorTreeExampleRuntime() noexcept { Shutdown(); }

  BehaviorTreeExampleRuntime(const BehaviorTreeExampleRuntime&) = delete;
  BehaviorTreeExampleRuntime& operator=(const BehaviorTreeExampleRuntime&) = delete;
  BehaviorTreeExampleRuntime(BehaviorTreeExampleRuntime&&) = delete;
  BehaviorTreeExampleRuntime& operator=(BehaviorTreeExampleRuntime&&) = delete;

  /** @brief Returns the command dispatcher used by command leaf nodes. */
  [[nodiscard]] std::shared_ptr<core::CommandDispatcher> Dispatcher() const noexcept {
    return dispatcher_;
  }

  /** @brief Returns the mission executor used by mission leaf nodes. */
  [[nodiscard]] std::shared_ptr<mission::MissionExecutor> MissionExecutor() const noexcept {
    return mission_executor_;
  }

  /** @brief Returns the shared runtime blackboard. */
  [[nodiscard]] std::shared_ptr<runtime::Blackboard> Blackboard() const noexcept {
    return blackboard_;
  }

  /**
   * @brief Writes one synchronized example event.
   *
   * @param event Event text.
   */
  void Print(std::string_view event) const {
    std::lock_guard<std::mutex> lock{output_mutex_};
    std::cout << event << '\n';
  }

  /**
   * @brief Executes one owned behavior tree through the shared runtime.
   *
   * @param name Human-readable example name.
   * @param execution_id Nonzero runtime execution id.
   * @param root Owned root node.
   * @return True when the runtime job completes successfully.
   */
  [[nodiscard]] bool Run(std::string_view name, runtime::RuntimeJobId execution_id,
                         std::unique_ptr<bt::BTNode> root) {
    if (!root) {
      return false;
    }

    auto tree = std::make_unique<bt::BehaviorTree>(std::move(root));
    bt::BehaviorTreeJobOptions options;
    options.executionId = execution_id;
    options.resourceId = "example/robot";
    options.resourceTimeout = std::chrono::seconds{1};
    options.tickInterval = std::chrono::milliseconds{5};
    options.metadata.emplace("example", std::string{name});

    runtime::RuntimeJobHandle handle =
        behavior_tree_runtime_->Submit(std::move(tree), std::move(options));
    const runtime::RuntimeJobResult result = handle.result.get();
    Print(std::string{name} + ": " + result.message);
    return result.state == runtime::ExecutionState::Completed;
  }

private:
  void Shutdown() noexcept {
    try {
      scheduler_->Shutdown();
      static_cast<void>(mission_executor_->Stop());
      static_cast<void>(dispatcher_->Shutdown());
      static_cast<void>(adapter_->Disconnect());
      static_cast<void>(adapter_->Shutdown());
    } catch (...) {
    }
  }

  std::shared_ptr<ExampleRobotAdapter> adapter_;
  std::shared_ptr<core::CommandDispatcher> dispatcher_;
  std::shared_ptr<mission::MissionExecutor> mission_executor_;
  std::shared_ptr<runtime::RuntimeScheduler> scheduler_;
  std::shared_ptr<runtime::Blackboard> blackboard_;
  std::shared_ptr<runtime::ResourceManager> resource_manager_;
  std::shared_ptr<bt::BehaviorTreeFactory> factory_;
  std::unique_ptr<bt::BehaviorTreeRuntime> behavior_tree_runtime_;
  mutable std::mutex output_mutex_;
};

/**
 * @brief Creates a generic command with a monotonic timestamp.
 *
 * @param id Producer-assigned command id.
 * @param type Generic command type.
 * @return Initialized command value.
 */
[[nodiscard]] inline core::Command MakeCommand(core::CommandId id, core::CommandType type) {
  core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = type;
  command.timeout = std::chrono::seconds{1};
  return command;
}

/**
 * @brief Creates a generic velocity command.
 *
 * @param id Producer-assigned command id.
 * @param linear_x Forward velocity.
 * @param linear_y Lateral velocity.
 * @param angular_z Yaw velocity.
 * @return Initialized move command.
 */
[[nodiscard]] inline core::Command MakeMoveCommand(core::CommandId id, double linear_x,
                                                   double linear_y, double angular_z) {
  core::Command command = MakeCommand(id, core::CommandType::Move);
  command.payload.emplace("linear_x", linear_x);
  command.payload.emplace("linear_y", linear_y);
  command.payload.emplace("angular_z", angular_z);
  return command;
}

/**
 * @brief Creates a one-step mission for a generic command.
 *
 * @param mission_id Producer-assigned mission id.
 * @param step_id Mission-local step id.
 * @param name Mission name.
 * @param command Generic command executed by the mission.
 * @return Valid mission value.
 */
[[nodiscard]] inline mission::Mission MakeMission(mission::MissionId mission_id,
                                                  mission::MissionStepId step_id, std::string name,
                                                  core::Command command) {
  mission::MissionStep step;
  step.id = step_id;
  step.name = name + " command";
  step.command = std::move(command);

  mission::Mission mission;
  mission.id = mission_id;
  mission.name = std::move(name);
  mission.description = "Behavior tree example mission";
  mission.version = "1.0";
  mission.author = "humanoid-core";
  mission.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  mission.steps.push_back(std::move(step));
  return mission;
}

} // namespace humanoid::examples
