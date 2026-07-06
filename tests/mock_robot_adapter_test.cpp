#include <gtest/gtest.h>

#include <factory/RobotFactoryRegistry.h>
#include <humanoid/adapters/IRobotFactory.h>
#include <humanoid/core/CommandStatus.h>
#include <humanoid/core/CommandType.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class MockRobotAdapter final : public humanoid::adapters::IRobotAdapter {
public:
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
    information.model = "Robot";
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
    capabilities.stop = true;
    capabilities.move = true;
    return capabilities;
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteCommand(const humanoid::core::Command& command) override {
    humanoid::core::CommandResult result;
    result.status = humanoid::core::CommandStatus::Completed;
    switch (command.type) {
    case humanoid::core::CommandType::Stand:
      ++stand_up_count_;
      result.message = "stand up";
      return result;
    case humanoid::core::CommandType::Move:
      ++move_count_;
      last_vx_ = 0.4F;
      last_vy_ = 0.1F;
      last_omega_ = 0.2F;
      result.message = "move";
      return result;
    case humanoid::core::CommandType::Stop:
      ++stop_count_;
      result.message = "stop";
      return result;
    default:
      result.status = humanoid::core::CommandStatus::Rejected;
      result.message = "unsupported";
      return result;
    }
  }

  [[nodiscard]] humanoid::common::Status Update() override { return humanoid::common::Status::ok(); }

  [[nodiscard]] int moveCount() const noexcept { return move_count_; }

  [[nodiscard]] int stopCount() const noexcept { return stop_count_; }

  [[nodiscard]] int standUpCount() const noexcept { return stand_up_count_; }

  [[nodiscard]] float lastVx() const noexcept { return last_vx_; }

  [[nodiscard]] float lastVy() const noexcept { return last_vy_; }

  [[nodiscard]] float lastOmega() const noexcept { return last_omega_; }

private:
  bool connected_{false};
  int move_count_{0};
  int stop_count_{0};
  int stand_up_count_{0};
  float last_vx_{0.0F};
  float last_vy_{0.0F};
  float last_omega_{0.0F};
};

class MockRobotFactory final : public humanoid::adapters::IRobotFactory {
public:
  [[nodiscard]] std::string_view Vendor() const noexcept override { return "Mock"; }

  [[nodiscard]] std::vector<std::string> SupportedModels() const override { return {"Robot"}; }

  [[nodiscard]] bool Supports(std::string_view vendor, std::string_view model) const override {
    return vendor == Vendor() && model == "Robot";
  }

  [[nodiscard]] std::unique_ptr<humanoid::adapters::IRobotAdapter>
  CreateAdapter(const humanoid::adapters::RobotConfig& config,
                std::shared_ptr<humanoid::logging::ILogger> logger) const override {
    (void)logger;
    if (!Supports(config.vendor, config.model)) {
      return nullptr;
    }

    return std::make_unique<MockRobotAdapter>();
  }
};

TEST(MockRobotAdapterTest, MoveRecordsVelocityCommand) {
  MockRobotAdapter adapter;
  humanoid::core::Command command;
  command.id = 1U;
  command.type = humanoid::core::CommandType::Move;

  const humanoid::core::CommandResult result = adapter.ExecuteCommand(command);

  EXPECT_TRUE(result.isSuccess());
  EXPECT_EQ(1, adapter.moveCount());
  EXPECT_FLOAT_EQ(0.4F, adapter.lastVx());
  EXPECT_FLOAT_EQ(0.1F, adapter.lastVy());
  EXPECT_FLOAT_EQ(0.2F, adapter.lastOmega());
}

TEST(MockRobotAdapterTest, StopRecordsCommand) {
  MockRobotAdapter adapter;
  humanoid::core::Command command;
  command.id = 1U;
  command.type = humanoid::core::CommandType::Stop;

  const humanoid::core::CommandResult result = adapter.ExecuteCommand(command);

  EXPECT_TRUE(result.isSuccess());
  EXPECT_EQ(1, adapter.stopCount());
}

TEST(MockRobotAdapterTest, StandUpRecordsCommand) {
  MockRobotAdapter adapter;
  humanoid::core::Command command;
  command.id = 1U;
  command.type = humanoid::core::CommandType::Stand;

  const humanoid::core::CommandResult result = adapter.ExecuteCommand(command);

  EXPECT_TRUE(result.isSuccess());
  EXPECT_EQ(1, adapter.standUpCount());
}

TEST(RobotFactoryRegistryTest, CreatesAdapterThroughRegisteredFactory) {
  humanoid::factory::RobotFactoryRegistry registry;
  ASSERT_TRUE(registry.RegisterFactory(std::make_shared<MockRobotFactory>()).Succeeded());

  humanoid::adapters::RobotConfig config;
  config.vendor = "Mock";
  config.model = "Robot";

  std::unique_ptr<humanoid::adapters::IRobotAdapter> adapter =
      registry.CreateAdapter(config, nullptr);
  ASSERT_NE(nullptr, adapter);
  humanoid::core::Command command;
  command.id = 1U;
  command.type = humanoid::core::CommandType::Move;
  EXPECT_TRUE(adapter->ExecuteCommand(command).isSuccess());
}

TEST(RobotFactoryRegistryTest, ReturnsNullptrForUnsupportedRobot) {
  humanoid::factory::RobotFactoryRegistry registry;
  ASSERT_TRUE(registry.RegisterFactory(std::make_shared<MockRobotFactory>()).Succeeded());

  humanoid::adapters::RobotConfig config;
  config.vendor = "Other";
  config.model = "Robot";

  EXPECT_EQ(nullptr, registry.CreateAdapter(config, nullptr));
}

} // namespace
