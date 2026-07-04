#include <factory/RobotFactoryRegistry.h>
#include <humanoid/adapters/IRobotFactory.h>
#include <humanoid/core/CoreContext.hpp>
#include <humanoid/core/RobotState.hpp>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/services/TelemetryService.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class SmokeRobotAdapter final : public humanoid::adapters::IRobotAdapter {
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
    return Success("shutdown");
  }

  humanoid::adapters::Result StandUp() override {
    ++stand_count_;
    return Success("stand");
  }

  humanoid::adapters::Result BalanceStand() override { return Success("balance"); }

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

  [[nodiscard]] int standCount() const noexcept { return stand_count_; }

  [[nodiscard]] int stopCount() const noexcept { return stop_count_; }

  [[nodiscard]] float lastVx() const noexcept { return last_vx_; }

  [[nodiscard]] float lastVy() const noexcept { return last_vy_; }

  [[nodiscard]] float lastOmega() const noexcept { return last_omega_; }

private:
  static humanoid::adapters::Result Success(std::string message) {
    return humanoid::adapters::Result{humanoid::adapters::ErrorCode::kSuccess, std::move(message)};
  }

  bool connected_{false};
  int move_count_{0};
  int stand_count_{0};
  int stop_count_{0};
  float last_vx_{0.0F};
  float last_vy_{0.0F};
  float last_omega_{0.0F};
};

class SmokeRobotFactory final : public humanoid::adapters::IRobotFactory {
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

    return std::make_unique<SmokeRobotAdapter>();
  }
};

} // namespace

int main() {
  humanoid::core::RobotState state;
  if (state.connection.connected || state.power.charging || state.motion.walking ||
      state.health.emergencyStop || state.health.faultCode != 0) {
    return EXIT_FAILURE;
  }

  humanoid::core::RobotStateManager state_manager;
  state.connection.connected = true;
  state.power.batteryLevel = 82.5F;
  state.health.emergencyStop = true;
  state.health.faultCode = 7;
  state_manager.UpdateState(state);

  if (!state_manager.IsConnected() || state_manager.BatteryLevel() != 82.5F ||
      !state_manager.EmergencyStop() || state_manager.FaultCode() != 7) {
    return EXIT_FAILURE;
  }

  const humanoid::core::RobotState snapshot = state_manager.GetState();
  if (!snapshot.connection.connected || snapshot.power.batteryLevel != 82.5F) {
    return EXIT_FAILURE;
  }

  state_manager.Reset();
  if (state_manager.IsConnected() || state_manager.EmergencyStop() ||
      state_manager.FaultCode() != 0) {
    return EXIT_FAILURE;
  }

  humanoid::core::CoreContext core_context;
  auto telemetry_state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  core_context.setRobotStateManager(telemetry_state_manager);
  if (!core_context.hasRobotStateManager() ||
      core_context.robotStateManager() != telemetry_state_manager) {
    return EXIT_FAILURE;
  }

  humanoid::core::RobotState telemetry_state;
  telemetry_state.connection.connected = true;
  telemetry_state.power.batteryLevel = 64.0F;
  core_context.robotStateManager()->UpdateState(telemetry_state);

  humanoid::services::TelemetryService telemetry_service{telemetry_state_manager,
                                                         std::chrono::milliseconds{5}};

  std::mutex telemetry_mutex;
  std::condition_variable telemetry_condition;
  int first_listener_count = 0;
  int second_listener_count = 0;
  humanoid::core::RobotState observed_telemetry_state;

  const auto first_subscription =
      telemetry_service.Subscribe([&](const humanoid::core::RobotState& published_state) {
        std::lock_guard<std::mutex> lock{telemetry_mutex};
        observed_telemetry_state = published_state;
        ++first_listener_count;
        telemetry_condition.notify_all();
      });
  const auto second_subscription =
      telemetry_service.Subscribe([&](const humanoid::core::RobotState& published_state) {
        std::lock_guard<std::mutex> lock{telemetry_mutex};
        observed_telemetry_state = published_state;
        ++second_listener_count;
        telemetry_condition.notify_all();
      });

  if (first_subscription == humanoid::services::TelemetryService::kInvalidSubscriptionId ||
      second_subscription == humanoid::services::TelemetryService::kInvalidSubscriptionId) {
    return EXIT_FAILURE;
  }

  if (!telemetry_service.Start()) {
    return EXIT_FAILURE;
  }

  {
    std::unique_lock<std::mutex> lock{telemetry_mutex};
    if (!telemetry_condition.wait_for(lock, std::chrono::milliseconds{250}, [&]() {
          return first_listener_count > 0 && second_listener_count > 0;
        })) {
      telemetry_service.Stop();
      return EXIT_FAILURE;
    }
  }

  telemetry_service.Stop();

  if (!observed_telemetry_state.connection.connected ||
      observed_telemetry_state.power.batteryLevel != 64.0F) {
    return EXIT_FAILURE;
  }

  if (!telemetry_service.Unsubscribe(first_subscription) ||
      !telemetry_service.Unsubscribe(second_subscription)) {
    return EXIT_FAILURE;
  }

  humanoid::factory::RobotFactoryRegistry registry;
  const humanoid::adapters::Result registration =
      registry.RegisterFactory(std::make_shared<SmokeRobotFactory>());
  if (!registration.Succeeded()) {
    return EXIT_FAILURE;
  }

  humanoid::adapters::RobotConfig config;
  config.vendor = "Mock";
  config.model = "Robot";

  std::unique_ptr<humanoid::adapters::IRobotAdapter> adapter =
      registry.CreateAdapter(config, nullptr);
  if (!adapter) {
    return EXIT_FAILURE;
  }

  if (!adapter->Move(0.2F, 0.0F, 0.1F).Succeeded()) {
    return EXIT_FAILURE;
  }

  if (!adapter->StandUp().Succeeded()) {
    return EXIT_FAILURE;
  }

  if (!adapter->Stop().Succeeded()) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
