/**
 * @file safety_validator_unit_test.cpp
 * @brief Validates command safety policy without robot hardware.
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandStatus.h>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/core/SafetyValidator.h>

namespace {

class SafetyMockAdapter final : public humanoid::adapters::IRobotAdapter {
public:
  explicit SafetyMockAdapter(bool connected = true) noexcept : connected_(connected) {}

  humanoid::common::Status Initialize() override { return humanoid::common::Status::ok(); }

  humanoid::common::Status Connect() override {
    connected_ = true;
    return humanoid::common::Status::ok();
  }

  humanoid::common::Status Disconnect() override {
    connected_ = false;
    return humanoid::common::Status::ok();
  }

  humanoid::common::Status Shutdown() override {
    connected_ = false;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] bool IsConnected() const noexcept override { return connected_; }

  [[nodiscard]] humanoid::core::RobotState GetRobotState() const override {
    humanoid::core::RobotState state;
    state.connection.connected = connected_;
    state.power.batteryLevel = 100.0F;
    state.motion.standing = true;
    return state;
  }

  [[nodiscard]] humanoid::core::RobotInformation GetRobotInformation() const override {
    humanoid::core::RobotInformation information;
    information.vendor = "Mock";
    information.model = "Safety";
    information.adapterName = "SafetyMockAdapter";
    return information;
  }

  [[nodiscard]] humanoid::core::RobotCapabilities GetCapabilities() const override {
    humanoid::core::RobotCapabilities capabilities;
    capabilities.supportsLifecycle = true;
    capabilities.supportsConnectionManagement = true;
    capabilities.supportsStateFeedback = true;
    capabilities.supportsPowerState = true;
    capabilities.supportsCommandExecution = true;
    return capabilities;
  }

  [[nodiscard]] humanoid::core::CommandCapabilitySet GetCommandCapabilities() const override {
    humanoid::core::CommandCapabilitySet capabilities;
    capabilities.stand = true;
    capabilities.stop = true;
    capabilities.move = true;
    capabilities.rotate = true;
    return capabilities;
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteCommand(const humanoid::core::Command& command) override {
    std::lock_guard<std::mutex> lock{mutex_};
    humanoid::core::CommandResult result;
    result.status = humanoid::core::CommandStatus::Completed;
    switch (command.type) {
    case humanoid::core::CommandType::Stand:
      ++stand_count_;
      return result;
    case humanoid::core::CommandType::Move:
    case humanoid::core::CommandType::Rotate:
      ++move_count_;
      last_vx_ = command.type == humanoid::core::CommandType::Move ? NumberPayload(command, "linear_x")
                                                                   : 0.0F;
      last_vy_ = command.type == humanoid::core::CommandType::Move ? NumberPayload(command, "linear_y")
                                                                   : 0.0F;
      last_omega_ = NumberPayload(command, "angular_z");
      return result;
    case humanoid::core::CommandType::Stop:
      ++stop_count_;
      return result;
    default:
      result.status = humanoid::core::CommandStatus::Rejected;
      return result;
    }
  }

  [[nodiscard]] humanoid::common::Status Update() override { return humanoid::common::Status::ok(); }

  [[nodiscard]] int standCount() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return stand_count_;
  }

  [[nodiscard]] int moveCount() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return move_count_;
  }

  [[nodiscard]] int stopCount() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return stop_count_;
  }

  [[nodiscard]] float lastVx() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return last_vx_;
  }

private:
  [[nodiscard]] static float NumberPayload(const humanoid::core::Command& command,
                                           const std::string& key) {
    const auto value = command.payload.find(key);
    if (value == command.payload.end()) {
      return 0.0F;
    }
    if (std::holds_alternative<double>(value->second)) {
      return static_cast<float>(std::get<double>(value->second));
    }
    if (std::holds_alternative<std::int64_t>(value->second)) {
      return static_cast<float>(std::get<std::int64_t>(value->second));
    }
    return 0.0F;
  }

  mutable std::mutex mutex_;
  bool connected_{true};
  int stand_count_{0};
  int move_count_{0};
  int stop_count_{0};
  float last_vx_{0.0F};
  float last_vy_{0.0F};
  float last_omega_{0.0F};
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

[[nodiscard]] humanoid::core::Command MakeMoveCommand(humanoid::core::CommandId id) {
  humanoid::core::Command command = MakeCommand(id, humanoid::core::CommandType::Move);
  command.payload.emplace("linear_x", 0.25);
  command.payload.emplace("linear_y", 0.0);
  command.payload.emplace("angular_z", 0.0);
  return command;
}

[[nodiscard]] humanoid::core::RobotState SafeRobotState() {
  humanoid::core::RobotState state;
  state.connection.connected = true;
  state.power.batteryLevel = 80.0F;
  state.motion.standing = true;
  return state;
}

[[nodiscard]] humanoid::core::SafetyValidationContext SafeContext() {
  humanoid::core::SafetyValidationContext context;
  context.robotState = SafeRobotState();
  context.capabilities = humanoid::core::CommandCapabilitySet::LegacyAdapterDefaults();
  return context;
}

void TestRejectsDisconnectedRobot() {
  humanoid::core::SafetyValidator validator;
  humanoid::core::SafetyValidationContext context = SafeContext();
  context.robotState.connection.connected = false;

  const humanoid::core::CommandResult result =
      validator.Validate(MakeCommand(1U, humanoid::core::CommandType::Stop), context);
  Check(result.status == humanoid::core::CommandStatus::Rejected,
        "Disconnected robot command was not rejected");
}

void TestRejectsEmergencyStopExceptStop() {
  humanoid::core::SafetyValidator validator;
  humanoid::core::SafetyValidationContext context = SafeContext();
  context.robotState.health.emergencyStop = true;

  Check(validator.Validate(MakeCommand(2U, humanoid::core::CommandType::Stand), context).status ==
            humanoid::core::CommandStatus::Rejected,
        "Stand was accepted during emergency stop");
  Check(validator.Validate(MakeCommand(3U, humanoid::core::CommandType::Stop), context).isSuccess(),
        "Stop was rejected during emergency stop");
}

void TestRejectsUnsupportedCapability() {
  humanoid::core::SafetyValidator validator;
  humanoid::core::SafetyValidationContext context = SafeContext();
  context.capabilities.move = false;

  const humanoid::core::CommandResult result = validator.Validate(MakeMoveCommand(4U), context);
  Check(result.status == humanoid::core::CommandStatus::Rejected,
        "Unsupported Move capability was accepted");
}

void TestRejectsBatteryAndUnsafeState() {
  humanoid::core::SafetyValidator validator;
  humanoid::core::SafetyValidationContext context = SafeContext();
  context.robotState.power.batteryLevel = 5.0F;

  Check(validator.Validate(MakeMoveCommand(5U), context).status ==
            humanoid::core::CommandStatus::Rejected,
        "Low-battery Move was accepted");
  Check(validator.Validate(MakeCommand(6U, humanoid::core::CommandType::Stop), context).isSuccess(),
        "Stop was rejected because battery was low");

  context.robotState.power.batteryLevel = 80.0F;
  context.robotState.motion.standing = false;
  Check(validator.Validate(MakeMoveCommand(7U), context).status ==
            humanoid::core::CommandStatus::Rejected,
        "Base motion was accepted without standing state");

  context.robotState.motion.standing = true;
  context.robotState.motion.sitting = true;
  Check(validator.Validate(MakeCommand(8U, humanoid::core::CommandType::Stand), context).status ==
            humanoid::core::CommandStatus::Rejected,
        "Contradictory motion state was accepted");
  Check(validator.Validate(MakeCommand(9U, humanoid::core::CommandType::Stop), context).isSuccess(),
        "Stop was rejected for contradictory motion state");
}

void TestDispatcherUsesInjectedStateManager() {
  auto adapter = std::make_shared<SafetyMockAdapter>();
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state = SafeRobotState();
  state.power.batteryLevel = 5.0F;
  state_manager->UpdateState(state);

  humanoid::core::CommandDispatcher dispatcher{
      adapter, state_manager, humanoid::core::CommandCapabilitySet::LegacyAdapterDefaults()};

  Check(dispatcher.Execute(MakeMoveCommand(10U)).status == humanoid::core::CommandStatus::Rejected,
        "Dispatcher forwarded a low-battery Move");
  Check(adapter->moveCount() == 0, "Adapter was called for a rejected Move");

  state.power.batteryLevel = 80.0F;
  state.motion.standing = true;
  state.health.emergencyStop = false;
  state_manager->UpdateState(state);
  Check(dispatcher.Execute(MakeMoveCommand(11U)).isSuccess(), "Safe Move was rejected");
  Check(adapter->moveCount() == 1 && adapter->lastVx() == 0.25F, "Safe Move was not forwarded");

  state.health.emergencyStop = true;
  state_manager->UpdateState(state);
  Check(dispatcher.Execute(MakeCommand(12U, humanoid::core::CommandType::Stand)).status ==
            humanoid::core::CommandStatus::Rejected,
        "Dispatcher forwarded Stand during emergency stop");
  Check(adapter->standCount() == 0, "Adapter was called for rejected Stand");
  Check(dispatcher.Execute(MakeCommand(13U, humanoid::core::CommandType::Stop)).isSuccess(),
        "Dispatcher rejected Stop during emergency stop");
  Check(adapter->stopCount() == 1, "Stop was not forwarded");
  Check(dispatcher.Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestLegacyDispatcherRejectsDisconnectedAdapter() {
  auto adapter = std::make_shared<SafetyMockAdapter>(false);
  humanoid::core::CommandDispatcher dispatcher{adapter};

  const humanoid::core::CommandResult result =
      dispatcher.Execute(MakeCommand(14U, humanoid::core::CommandType::Stop));
  Check(result.status == humanoid::core::CommandStatus::Rejected,
        "Legacy dispatcher accepted a disconnected adapter");
  Check(adapter->stopCount() == 0, "Disconnected adapter received Stop");
  Check(dispatcher.Shutdown().isSuccess(), "Legacy dispatcher shutdown failed");
}

} // namespace

int main() {
  try {
    TestRejectsDisconnectedRobot();
    TestRejectsEmergencyStopExceptStop();
    TestRejectsUnsupportedCapability();
    TestRejectsBatteryAndUnsafeState();
    TestDispatcherUsesInjectedStateManager();
    TestLegacyDispatcherRejectsDisconnectedAdapter();
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}
