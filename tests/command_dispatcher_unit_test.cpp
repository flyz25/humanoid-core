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
#include <vector>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/core/CommandDispatcher.h>

namespace {

class MockRobotAdapter final : public humanoid::adapters::IRobotAdapter {
public:
  struct Velocity final {
    float linear_x{0.0F};
    float linear_y{0.0F};
    float angular_z{0.0F};
  };

  humanoid::adapters::Result Initialize() override { return Success("initialized"); }

  humanoid::adapters::Result Connect() override { return Success("connected"); }

  humanoid::adapters::Result Disconnect() override { return Success("disconnected"); }

  humanoid::adapters::Result Shutdown() override { return Success("shutdown"); }

  humanoid::adapters::Result StandUp() override { return Invoke("Stand"); }

  humanoid::adapters::Result BalanceStand() override { return Invoke("BalanceStand"); }

  humanoid::adapters::Result Move(float vx, float vy, float omega) override {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      velocities_.push_back(Velocity{vx, vy, omega});
    }
    return Invoke("Move");
  }

  humanoid::adapters::Result Stop() override { return Invoke("Stop"); }

  humanoid::adapters::Result EmergencyStop() override { return Invoke("EmergencyStop"); }

  [[nodiscard]] humanoid::adapters::RobotStateResult GetRobotState() const override {
    return humanoid::adapters::RobotStateResult{Success("state"), {}};
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

  void FailNextCommand(humanoid::adapters::ErrorCode error_code) {
    std::lock_guard<std::mutex> lock{mutex_};
    next_error_ = error_code;
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
  static humanoid::adapters::Result Success(std::string message) {
    return {humanoid::adapters::ErrorCode::kSuccess, std::move(message)};
  }

  humanoid::adapters::Result Invoke(std::string action) {
    humanoid::adapters::ErrorCode error_code = humanoid::adapters::ErrorCode::kSuccess;
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

      error_code = next_error_;
      next_error_ = humanoid::adapters::ErrorCode::kSuccess;
      should_throw = throw_on_next_command_;
      throw_on_next_command_ = false;
    }

    if (should_throw) {
      throw std::runtime_error{"mock adapter failure"};
    }
    if (error_code != humanoid::adapters::ErrorCode::kSuccess) {
      return {error_code, "mock adapter error"};
    }
    return Success("command completed");
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<std::string> actions_;
  std::vector<Velocity> velocities_;
  humanoid::adapters::ErrorCode next_error_{humanoid::adapters::ErrorCode::kSuccess};
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

  Check(dispatcher.Execute(MakeCommand(11U, humanoid::core::CommandType::Sit)).status ==
            humanoid::core::CommandStatus::Rejected,
        "Unsupported Sit command was accepted");

  humanoid::core::Command expired = MakeCommand(12U, humanoid::core::CommandType::Stop);
  expired.timestamp -= std::chrono::milliseconds{10};
  expired.timeout = std::chrono::milliseconds{1};
  Check(dispatcher.Execute(expired).status == humanoid::core::CommandStatus::Timeout,
        "Expired command did not time out");

  adapter->FailNextCommand(humanoid::adapters::ErrorCode::kTimeout);
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
