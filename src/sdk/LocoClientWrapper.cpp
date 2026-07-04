#include <sdk/LocoClientWrapper.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <utility>

#include <humanoid/core/RobotStateManager.hpp>

#include <SdkConverter.h>
#include <SdkTypes.h>
#include <SdkWrapper.h>

namespace humanoid::sdk {
namespace {

namespace unitree_sdk = humanoid::plugins::unitree::sdk;

[[nodiscard]] unitree_sdk::SdkConfiguration
ToSdkConfiguration(const adapters::RobotConfig& config) {
  unitree_sdk::SdkConfiguration configuration;
  configuration.robot_ip = config.ip;
  configuration.network_interface = config.network_interface;
  configuration.domain_id = config.domain_id;
  configuration.timeout = config.timeout;
  configuration.serial_number = config.serial_number;
  configuration.firmware_version = config.firmware;
  return configuration;
}

} // namespace

class LocoClientWrapper::Impl final {
public:
  Impl() = default;
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  mutable std::mutex mutex_;
  std::chrono::milliseconds communication_timeout_{500};
  unitree_sdk::SdkWrapper sdk_wrapper_;
};

LocoClientWrapper::LocoClientWrapper() : impl_(std::make_unique<Impl>()) {}

LocoClientWrapper::~LocoClientWrapper() = default;

adapters::Result LocoClientWrapper::Initialize(const adapters::RobotConfig& config) {
  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    impl_->communication_timeout_ = config.timeout;
  }

  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Initialize(ToSdkConfiguration(config)));
}

adapters::Result LocoClientWrapper::Connect() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Connect());
}

adapters::Result LocoClientWrapper::Disconnect() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Disconnect());
}

adapters::Result LocoClientWrapper::StartCommunication() {
  std::chrono::milliseconds timeout{500};
  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    timeout = impl_->communication_timeout_;
  }

  unitree_sdk::SdkCommunicationOptions options;
  options.heartbeat_interval = timeout;
  options.reconnect_interval = timeout * 2;
  options.connection_timeout = timeout * 3;
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.StartCommunication(options));
}

adapters::Result LocoClientWrapper::StopCommunication() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.StopCommunication());
}

adapters::Result LocoClientWrapper::SynchronizeState() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.SynchronizeState());
}

void LocoClientWrapper::SetRobotStateManager(
    std::shared_ptr<core::RobotStateManager> state_manager) {
  if (!state_manager) {
    impl_->sdk_wrapper_.SetStateUpdateCallback({});
    return;
  }

  impl_->sdk_wrapper_.SetStateUpdateCallback(
      [state_manager = std::move(state_manager)](const core::RobotState& state) {
        state_manager->UpdateState(state);
      });
}

adapters::Result LocoClientWrapper::Move(float vx, float vy, float omega) {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Move(vx, vy, omega));
}

adapters::Result LocoClientWrapper::Stop() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Stop());
}

adapters::Result LocoClientWrapper::StandUp() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.StandUp());
}

adapters::Result LocoClientWrapper::BalanceStand() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.BalanceStand());
}

adapters::Result LocoClientWrapper::EmergencyStop() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.EmergencyStop());
}

adapters::Result LocoClientWrapper::Shutdown() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Shutdown());
}

bool LocoClientWrapper::IsInitialized() const noexcept {
  return impl_->sdk_wrapper_.IsInitialized();
}

bool LocoClientWrapper::IsConnected() const noexcept { return impl_->sdk_wrapper_.IsConnected(); }

} // namespace humanoid::sdk
