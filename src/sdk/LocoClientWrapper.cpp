#include <sdk/LocoClientWrapper.h>

#include <memory>

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

  unitree_sdk::SdkWrapper sdk_wrapper_;
};

LocoClientWrapper::LocoClientWrapper() : impl_(std::make_unique<Impl>()) {}

LocoClientWrapper::~LocoClientWrapper() = default;

adapters::Result LocoClientWrapper::Initialize(const adapters::RobotConfig& config) {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Initialize(ToSdkConfiguration(config)));
}

adapters::Result LocoClientWrapper::Connect() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Connect());
}

adapters::Result LocoClientWrapper::Disconnect() {
  return unitree_sdk::ToAdapterResult(impl_->sdk_wrapper_.Disconnect());
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
