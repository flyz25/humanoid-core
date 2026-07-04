#include <gtest/gtest.h>

#include <factory/RobotFactoryRegistry.h>
#include <humanoid/adapters/IRobotFactory.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class MockRobotAdapter final : public humanoid::adapters::IRobotAdapter {
public:
  humanoid::adapters::Result Initialize() override { return Success("initialized"); }

  humanoid::adapters::Result Connect() override {
    connected_ = true;
    return Success("connected");
  }

  humanoid::adapters::Result Disconnect() override {
    connected_ = false;
    return Success("disconnected");
  }

  humanoid::adapters::Result Shutdown() override {
    connected_ = false;
    return Success("shut down");
  }

  humanoid::adapters::Result StandUp() override {
    ++stand_up_count_;
    return Success("stand up");
  }

  humanoid::adapters::Result BalanceStand() override { return Success("balance stand"); }

  humanoid::adapters::Result Move(float vx, float vy, float omega) override {
    ++move_count_;
    last_vx_ = vx;
    last_vy_ = vy;
    last_omega_ = omega;
    return Success("move");
  }

  humanoid::adapters::Result Stop() override {
    ++stop_count_;
    return Success("stop");
  }

  humanoid::adapters::Result EmergencyStop() override {
    connected_ = false;
    return Success("emergency stop");
  }

  [[nodiscard]] humanoid::adapters::RobotStateResult GetRobotState() const override {
    humanoid::adapters::RobotState state;
    state.vendor = "Mock";
    state.model = "Robot";
    state.connected = connected_;
    return humanoid::adapters::RobotStateResult{Success("state"), state};
  }

  [[nodiscard]] int moveCount() const noexcept { return move_count_; }

  [[nodiscard]] int stopCount() const noexcept { return stop_count_; }

  [[nodiscard]] int standUpCount() const noexcept { return stand_up_count_; }

  [[nodiscard]] float lastVx() const noexcept { return last_vx_; }

  [[nodiscard]] float lastVy() const noexcept { return last_vy_; }

  [[nodiscard]] float lastOmega() const noexcept { return last_omega_; }

private:
  static humanoid::adapters::Result Success(std::string message) {
    return humanoid::adapters::Result{humanoid::adapters::ErrorCode::kSuccess, std::move(message)};
  }

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

  const humanoid::adapters::Result result = adapter.Move(0.4F, 0.1F, 0.2F);

  EXPECT_TRUE(result.Succeeded());
  EXPECT_EQ(1, adapter.moveCount());
  EXPECT_FLOAT_EQ(0.4F, adapter.lastVx());
  EXPECT_FLOAT_EQ(0.1F, adapter.lastVy());
  EXPECT_FLOAT_EQ(0.2F, adapter.lastOmega());
}

TEST(MockRobotAdapterTest, StopRecordsCommand) {
  MockRobotAdapter adapter;

  const humanoid::adapters::Result result = adapter.Stop();

  EXPECT_TRUE(result.Succeeded());
  EXPECT_EQ(1, adapter.stopCount());
}

TEST(MockRobotAdapterTest, StandUpRecordsCommand) {
  MockRobotAdapter adapter;

  const humanoid::adapters::Result result = adapter.StandUp();

  EXPECT_TRUE(result.Succeeded());
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
  EXPECT_TRUE(adapter->Move(0.1F, 0.0F, 0.0F).Succeeded());
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
