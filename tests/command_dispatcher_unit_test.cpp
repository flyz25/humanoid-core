/**
 * @file command_dispatcher_unit_test.cpp
 * @brief Validates vendor-independent command dispatch without robot hardware.
 */

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/RobotAdapter.h>

namespace {

class MockRobotAdapter final : public humanoid::core::RobotAdapter {
public:
  struct Velocity final {
    float linear_x{0.0F};
    float linear_y{0.0F};
    float angular_z{0.0F};
  };

  humanoid::common::Status Initialize() override { return humanoid::common::Status::ok(); }

  humanoid::common::Status Connect() override { return humanoid::common::Status::ok(); }

  humanoid::common::Status Disconnect() override { return humanoid::common::Status::ok(); }

  humanoid::common::Status Shutdown() override { return humanoid::common::Status::ok(); }

  [[nodiscard]] bool IsConnected() const noexcept override { return true; }

  [[nodiscard]] humanoid::core::RobotState GetRobotState() const override {
    humanoid::core::RobotState state;
    state.connection.connected = true;
    state.power.batteryLevel = 100.0F;
    state.motion.standing = true;
    return state;
  }

  [[nodiscard]] humanoid::core::RobotInformation GetRobotInformation() const override {
    humanoid::core::RobotInformation information;
    information.vendor = "Mock";
    information.model = "Dispatcher";
    information.adapterName = "MockRobotAdapter";
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
    capabilities.sit = true;
    capabilities.walk = true;
    capabilities.stop = true;
    capabilities.move = true;
    capabilities.rotate = true;
    capabilities.velocity = true;
    capabilities.emergencyStop = true;
    return capabilities;
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteCommand(const humanoid::core::Command& command) override {
    switch (command.type) {
    case humanoid::core::CommandType::Stand:
      return Invoke("Stand");
    case humanoid::core::CommandType::Sit:
      return Invoke("Sit");
    case humanoid::core::CommandType::Walk:
    case humanoid::core::CommandType::Move:
    case humanoid::core::CommandType::Velocity:
      RecordVelocity(command, false);
      return Invoke("Move");
    case humanoid::core::CommandType::Rotate:
      RecordVelocity(command, true);
      return Invoke("Move");
    case humanoid::core::CommandType::Stop:
      return Invoke("Stop");
    case humanoid::core::CommandType::EmergencyStop:
      return Invoke("EmergencyStop");
    case humanoid::core::CommandType::HandOpen:
    case humanoid::core::CommandType::HandClose:
    case humanoid::core::CommandType::Gesture:
    case humanoid::core::CommandType::PlayAudio:
    case humanoid::core::CommandType::StopAudio:
    case humanoid::core::CommandType::SetVolume:
    case humanoid::core::CommandType::MuteAudio:
    case humanoid::core::CommandType::Custom:
      return {humanoid::core::CommandStatus::Rejected, "unsupported mock command"};
    }
    return {humanoid::core::CommandStatus::Rejected, "unknown mock command"};
  }

  [[nodiscard]] humanoid::common::Status Update() override { return humanoid::common::Status::ok(); }

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

  void FailNextCommand(humanoid::core::CommandStatus status) {
    std::lock_guard<std::mutex> lock{mutex_};
    next_status_ = status;
  }

  void ThrowOnNextCommand() {
    std::lock_guard<std::mutex> lock{mutex_};
    throw_on_next_command_ = true;
  }

  [[nodiscard]] std::vector<std::string> Actions() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return actions_;
  }

  [[nodiscard]] std::vector<Velocity> Velocities() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return velocities_;
  }

private:
  void RecordVelocity(const humanoid::core::Command& command, bool rotate_only) {
    const auto number = [&command](const std::string& key) -> float {
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
    };
    const float linear_x = rotate_only ? 0.0F : number("linear_x");
    const float linear_y = rotate_only ? 0.0F : number("linear_y");
    const float angular_z = number("angular_z");
    std::lock_guard<std::mutex> lock{mutex_};
    velocities_.push_back(Velocity{linear_x, linear_y, angular_z});
  }

  humanoid::core::CommandResult Invoke(std::string action) {
    humanoid::core::CommandStatus status = humanoid::core::CommandStatus::Completed;
    bool should_throw = false;
    {
      std::unique_lock<std::mutex> lock{mutex_};
      actions_.push_back(std::move(action));
      if (block_next_command_) {
        block_next_command_ = false;
        blocked_command_started_ = true;
        condition_.notify_all();
        condition_.wait(lock, [this]() { return release_blocked_command_; });
      }

      if (next_status_ != humanoid::core::CommandStatus::Pending) {
        status = next_status_;
        next_status_ = humanoid::core::CommandStatus::Pending;
      }
      should_throw = throw_on_next_command_;
      throw_on_next_command_ = false;
    }

    if (should_throw) {
      throw std::runtime_error{"mock adapter failure"};
    }
    if (status != humanoid::core::CommandStatus::Completed) {
      return {status, "mock adapter error"};
    }
    return {humanoid::core::CommandStatus::Completed, "command completed"};
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<std::string> actions_;
  std::vector<Velocity> velocities_;
  humanoid::core::CommandStatus next_status_{humanoid::core::CommandStatus::Pending};
  bool block_next_command_{false};
  bool blocked_command_started_{false};
  bool release_blocked_command_{false};
  bool throw_on_next_command_{false};
};

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

struct CommandVelocity final {
  double linear_x{0.0};
  double linear_y{0.0};
  double angular_z{0.0};
};

struct CommandRotation final {
  double angular_z{0.0};
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

[[nodiscard]] humanoid::core::Command MakeMoveCommand(humanoid::core::CommandId id,
                                                      const CommandVelocity& velocity) {
  humanoid::core::Command command = MakeCommand(id, humanoid::core::CommandType::Move);
  command.payload.emplace("linear_x", velocity.linear_x);
  command.payload.emplace("linear_y", velocity.linear_y);
  command.payload.emplace("angular_z", velocity.angular_z);
  return command;
}

[[nodiscard]] humanoid::core::Command MakeRotateCommand(humanoid::core::CommandId id,
                                                        const CommandRotation& rotation) {
  humanoid::core::Command command = MakeCommand(id, humanoid::core::CommandType::Rotate);
  command.payload.emplace("angular_z", rotation.angular_z);
  return command;
}

void TestSynchronousForwarding() {
  auto adapter = std::make_shared<MockRobotAdapter>();
  humanoid::core::CommandDispatcher dispatcher{adapter};

  Check(dispatcher.Execute(MakeCommand(1U, humanoid::core::CommandType::Stand)).isSuccess(),
        "Stand command failed");
  Check(dispatcher.Execute(MakeMoveCommand(2U, {0.25, -0.1, 0.2})).isSuccess(),
        "Move command failed");
  Check(dispatcher.Execute(MakeRotateCommand(3U, {0.4})).isSuccess(), "Rotate command failed");
  Check(dispatcher.Execute(MakeCommand(4U, humanoid::core::CommandType::Stop)).isSuccess(),
        "Stop command failed");

  const std::vector<std::string> actions = adapter->Actions();
  Check(actions == std::vector<std::string>({"Stand", "Move", "Move", "Stop"}),
        "Unexpected adapter forwarding sequence");

  const std::vector<MockRobotAdapter::Velocity> velocities = adapter->Velocities();
  Check(velocities.size() == 2U, "Unexpected velocity call count");
  Check(velocities[0].linear_x == 0.25F && velocities[0].linear_y == -0.1F &&
            velocities[0].angular_z == 0.2F,
        "Move payload was not forwarded");
  Check(velocities[1].linear_x == 0.0F && velocities[1].linear_y == 0.0F &&
            velocities[1].angular_z == 0.4F,
        "Rotate payload was not forwarded");

  Check(dispatcher.Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestValidationAndFailureTranslation() {
  auto adapter = std::make_shared<MockRobotAdapter>();
  humanoid::core::CommandDispatcher dispatcher{adapter};

  humanoid::core::Command invalid_id = MakeCommand(0U, humanoid::core::CommandType::Stop);
  Check(dispatcher.Execute(invalid_id).status == humanoid::core::CommandStatus::Rejected,
        "Zero command ID was accepted");

  humanoid::core::Command malformed_move = MakeCommand(10U, humanoid::core::CommandType::Move);
  malformed_move.payload.emplace("linear_x", 0.1);
  Check(dispatcher.Execute(malformed_move).status == humanoid::core::CommandStatus::Rejected,
        "Malformed Move payload was accepted");

  Check(dispatcher.Execute(MakeCommand(11U, humanoid::core::CommandType::HandOpen)).status ==
            humanoid::core::CommandStatus::Rejected,
        "Unsupported HandOpen command was accepted");

  humanoid::core::Command expired = MakeCommand(12U, humanoid::core::CommandType::Stop);
  expired.timestamp -= std::chrono::milliseconds{10};
  expired.timeout = std::chrono::milliseconds{1};
  Check(dispatcher.Execute(expired).status == humanoid::core::CommandStatus::Timeout,
        "Expired command did not time out");

  adapter->FailNextCommand(humanoid::core::CommandStatus::Timeout);
  Check(dispatcher.Execute(MakeCommand(13U, humanoid::core::CommandType::Stop)).status ==
            humanoid::core::CommandStatus::Timeout,
        "Adapter timeout was not translated");

  adapter->ThrowOnNextCommand();
  Check(dispatcher.Execute(MakeCommand(14U, humanoid::core::CommandType::Stop)).status ==
            humanoid::core::CommandStatus::Failed,
        "Adapter exception escaped command translation");

  humanoid::core::CommandDispatcher missing_adapter{nullptr};
  Check(missing_adapter.Execute(MakeCommand(15U, humanoid::core::CommandType::Stop)).status ==
            humanoid::core::CommandStatus::Rejected,
        "Missing adapter dependency was not rejected");

  Check(dispatcher.Shutdown().isSuccess(), "Dispatcher shutdown failed");
  Check(missing_adapter.Shutdown().isSuccess(), "Null-adapter dispatcher shutdown failed");
}

void TestAsyncPriorityAndCancellation() {
  auto adapter = std::make_shared<MockRobotAdapter>();
  humanoid::core::CommandDispatcher dispatcher{adapter};

  adapter->BlockNextCommand();
  std::future<humanoid::core::CommandResult> blocker =
      dispatcher.ExecuteAsync(MakeCommand(20U, humanoid::core::CommandType::Stand));
  Check(adapter->WaitForBlockedCommand(std::chrono::milliseconds{500}),
        "Blocking async command did not start");

  humanoid::core::Command low_priority = MakeMoveCommand(21U, {0.1, 0.0, 0.0});
  low_priority.priority = humanoid::core::CommandPriority::Low;
  humanoid::core::Command normal_priority = MakeRotateCommand(22U, {0.7});
  normal_priority.priority = humanoid::core::CommandPriority::Normal;
  humanoid::core::Command critical_priority = MakeCommand(23U, humanoid::core::CommandType::Stop);
  critical_priority.priority = humanoid::core::CommandPriority::Critical;

  std::future<humanoid::core::CommandResult> low = dispatcher.ExecuteAsync(std::move(low_priority));
  std::future<humanoid::core::CommandResult> normal =
      dispatcher.ExecuteAsync(std::move(normal_priority));
  std::future<humanoid::core::CommandResult> critical =
      dispatcher.ExecuteAsync(std::move(critical_priority));

  Check(dispatcher.Cancel(20U).status == humanoid::core::CommandStatus::Rejected,
        "Running command was cancelled");
  Check(dispatcher.Execute(MakeCommand(20U, humanoid::core::CommandType::Stop)).status ==
            humanoid::core::CommandStatus::Rejected,
        "Duplicate in-flight command ID was accepted");

  adapter->ReleaseBlockedCommand();
  Check(blocker.get().isSuccess(), "Blocking command failed");
  Check(critical.get().isSuccess(), "Critical command failed");
  Check(normal.get().isSuccess(), "Normal command failed");
  Check(low.get().isSuccess(), "Low-priority command failed");

  const std::vector<std::string> actions = adapter->Actions();
  Check(actions == std::vector<std::string>({"Stand", "Stop", "Move", "Move"}),
        "Async commands were not ordered by priority");
  const std::vector<MockRobotAdapter::Velocity> velocities = adapter->Velocities();
  Check(velocities.size() == 2U && velocities[0].angular_z == 0.7F &&
            velocities[1].linear_x == 0.1F,
        "Equal adapter methods were not dispatched in priority order");

  adapter->BlockNextCommand();
  std::future<humanoid::core::CommandResult> second_blocker =
      dispatcher.ExecuteAsync(MakeCommand(24U, humanoid::core::CommandType::Stand));
  Check(adapter->WaitForBlockedCommand(std::chrono::milliseconds{500}),
        "Second blocking command did not start");
  std::future<humanoid::core::CommandResult> cancelled =
      dispatcher.ExecuteAsync(MakeCommand(25U, humanoid::core::CommandType::Stop));
  Check(dispatcher.Cancel(25U).status == humanoid::core::CommandStatus::Cancelled,
        "Queued command cancellation failed");
  Check(cancelled.get().status == humanoid::core::CommandStatus::Cancelled,
        "Cancelled future returned an incorrect status");
  adapter->ReleaseBlockedCommand();
  Check(second_blocker.get().isSuccess(), "Second blocking command failed");

  Check(dispatcher.Shutdown().isSuccess(), "Dispatcher shutdown failed");
}

void TestShutdownCancelsQueueAndRejectsNewWork() {
  auto adapter = std::make_shared<MockRobotAdapter>();
  humanoid::core::CommandDispatcher dispatcher{adapter};

  adapter->BlockNextCommand();
  std::future<humanoid::core::CommandResult> running =
      dispatcher.ExecuteAsync(MakeCommand(30U, humanoid::core::CommandType::Stand));
  Check(adapter->WaitForBlockedCommand(std::chrono::milliseconds{500}),
        "Shutdown test command did not start");
  std::future<humanoid::core::CommandResult> queued =
      dispatcher.ExecuteAsync(MakeCommand(31U, humanoid::core::CommandType::Stop));

  std::future<humanoid::core::CommandResult> shutdown =
      std::async(std::launch::async, [&dispatcher]() { return dispatcher.Shutdown(); });
  Check(queued.wait_for(std::chrono::milliseconds{500}) == std::future_status::ready,
        "Shutdown did not cancel queued command");
  Check(queued.get().status == humanoid::core::CommandStatus::Cancelled,
        "Shutdown cancellation returned an incorrect status");
  Check(shutdown.wait_for(std::chrono::milliseconds{20}) == std::future_status::timeout,
        "Shutdown did not wait for the active adapter call");

  adapter->ReleaseBlockedCommand();
  Check(running.get().isSuccess(), "Active command failed during shutdown");
  Check(shutdown.get().isSuccess(), "Shutdown operation failed");

  Check(dispatcher.Execute(MakeCommand(32U, humanoid::core::CommandType::Stop)).status ==
            humanoid::core::CommandStatus::Rejected,
        "Synchronous command was accepted after shutdown");
  Check(dispatcher.ExecuteAsync(MakeCommand(33U, humanoid::core::CommandType::Stop)).get().status ==
            humanoid::core::CommandStatus::Rejected,
        "Asynchronous command was accepted after shutdown");
  Check(dispatcher.Shutdown().isSuccess(), "Idempotent shutdown failed");
}

} // namespace

int main() {
  try {
    TestSynchronousForwarding();
    TestValidationAndFailureTranslation();
    TestAsyncPriorityAndCancellation();
    TestShutdownCancelsQueueAndRejectsNewWork();
  } catch (...) {
    return 1;
  }
  return 0;
}
