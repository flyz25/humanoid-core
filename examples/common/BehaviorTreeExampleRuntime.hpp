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
#include <humanoid/core/CommandStatus.h>
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
  common::Status Initialize() override {
    std::lock_guard<std::mutex> lock{mutex_};
    initialized_ = true;
    return common::Status::ok();
  }

  /** @brief Connects process-local adapter state. */
  common::Status Connect() override {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!initialized_) {
      return common::Status::error(common::StatusCode::kFailedPrecondition,
                                   "example adapter is not initialized");
    }
    connected_ = true;
    return common::Status::ok();
  }

  /** @brief Disconnects process-local adapter state. */
  common::Status Disconnect() override {
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    return common::Status::ok();
  }

  /** @brief Releases process-local adapter state. */
  common::Status Shutdown() override {
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    initialized_ = false;
    return common::Status::ok();
  }

  /** @brief Returns current process-local connection state. */
  [[nodiscard]] bool IsConnected() const noexcept override { return connected_; }

  /** @brief Returns process-local robot state. */
  [[nodiscard]] core::RobotState GetRobotState() const override {
    std::lock_guard<std::mutex> lock{mutex_};
    core::RobotState state;
    state.connection.connected = connected_;
    state.power.batteryLevel = 100.0F;
    state.motion.standing = true;
    return state;
  }

  /** @brief Returns process-local robot metadata. */
  [[nodiscard]] core::RobotInformation GetRobotInformation() const override {
    core::RobotInformation information;
    information.vendor = "Example";
    information.model = "ProcessLocal";
    information.adapterName = "ExampleRobotAdapter";
    return information;
  }

  /** @brief Returns process-local adapter capabilities. */
  [[nodiscard]] core::RobotCapabilities GetCapabilities() const override {
    core::RobotCapabilities capabilities;
    capabilities.supportsLifecycle = true;
    capabilities.supportsConnectionManagement = true;
    capabilities.supportsStateFeedback = true;
    capabilities.supportsPowerState = true;
    capabilities.supportsCommandExecution = true;
    return capabilities;
  }

  /** @brief Returns command capabilities used by example missions and trees. */
  [[nodiscard]] core::CommandCapabilitySet GetCommandCapabilities() const override {
    core::CommandCapabilitySet capabilities;
    capabilities.stand = true;
    capabilities.stop = true;
    capabilities.move = true;
    capabilities.custom = true;
    return capabilities;
  }

  /** @brief Executes a command through the unified adapter boundary. */
  [[nodiscard]] core::CommandResult ExecuteCommand(const core::Command& command) override {
    switch (command.type) {
    case core::CommandType::Stand:
      return Record("Stand");
    case core::CommandType::Move:
      return Record("Move");
    case core::CommandType::Stop:
      return Record("Stop");
    case core::CommandType::Custom:
      return Record("Custom");
    default:
      return {core::CommandStatus::Rejected, "unsupported example command"};
    }
  }

  /** @brief Advances process-local adapter state. */
  [[nodiscard]] common::Status Update() override { return common::Status::ok(); }

private:
  core::CommandResult ConnectedResult(std::string_view command) const {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!connected_) {
      return {core::CommandStatus::Rejected,
              std::string{command} + " rejected: example adapter is disconnected"};
    }
    return {core::CommandStatus::Completed, std::string{command} + " completed"};
  }

  core::CommandResult Record(std::string_view command) const {
    std::cout << "adapter: " << command << '\n';
    return ConnectedResult(command);
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
    const common::Status initialize_result = adapter_->Initialize();
    if (!initialize_result.isOk()) {
      throw std::runtime_error{initialize_result.message()};
    }
    const common::Status connect_result = adapter_->Connect();
    if (!connect_result.isOk()) {
      static_cast<void>(adapter_->Shutdown());
      throw std::runtime_error{connect_result.message()};
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
