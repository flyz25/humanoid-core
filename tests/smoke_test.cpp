#include <factory/RobotFactoryRegistry.h>
#include <humanoid/adapters/IRobotFactory.h>
#include <humanoid/core/CoreContext.hpp>
#include <humanoid/core/CommandStatus.h>
#include <humanoid/core/CommandType.h>
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
    information.adapterName = "SmokeRobotAdapter";
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
      ++stand_count_;
      result.message = "stand";
      return result;
    case humanoid::core::CommandType::Move:
      ++move_count_;
      last_vx_ = 0.2F;
      last_vy_ = 0.0F;
      last_omega_ = 0.1F;
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

  [[nodiscard]] int standCount() const noexcept { return stand_count_; }

  [[nodiscard]] int stopCount() const noexcept { return stop_count_; }

  [[nodiscard]] float lastVx() const noexcept { return last_vx_; }

  [[nodiscard]] float lastVy() const noexcept { return last_vy_; }

  [[nodiscard]] float lastOmega() const noexcept { return last_omega_; }

private:
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

  humanoid::core::Command move;
  move.id = 1U;
  move.type = humanoid::core::CommandType::Move;
  if (!adapter->ExecuteCommand(move).isSuccess()) {
    return EXIT_FAILURE;
  }

  humanoid::core::Command stand;
  stand.id = 2U;
  stand.type = humanoid::core::CommandType::Stand;
  if (!adapter->ExecuteCommand(stand).isSuccess()) {
    return EXIT_FAILURE;
  }

  humanoid::core::Command stop;
  stop.id = 3U;
  stop.type = humanoid::core::CommandType::Stop;
  if (!adapter->ExecuteCommand(stop).isSuccess()) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
