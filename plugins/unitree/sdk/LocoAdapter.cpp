#include "LocoAdapter.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

#include "SdkClientSupport.h"

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/loco/g1_loco_client.hpp>

namespace humanoid::plugins::unitree::sdk {
namespace {

[[nodiscard]] bool IsFinite(const SdkVelocityCommand& velocity) noexcept {
  return std::isfinite(velocity.linear_x) && std::isfinite(velocity.linear_y) &&
         std::isfinite(velocity.angular_z);
}

} // namespace

class LocoAdapter::Impl final {
public:
  Impl() = default;
  ~Impl() noexcept = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] SdkResult RequireInitialized(const char* command) const {
    if (!initialized_ || !client_) {
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               std::string{command} + " requires initialized locomotion client");
    }

    return internal::Success(std::string{command} + " precondition satisfied");
  }

  [[nodiscard]] SdkResult RequireConnected(const char* command) const {
    if (!initialized_ || !client_ || !connected_) {
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               std::string{command} + " requires connected locomotion client");
    }

    return internal::Success(std::string{command} + " precondition satisfied");
  }

  mutable std::mutex mutex_;
  std::unique_ptr<::unitree::robot::g1::LocoClient> client_;
  SdkConfiguration configuration_{};
  bool initialized_{false};
  bool connected_{false};
};

LocoAdapter::LocoAdapter() : impl_(std::make_unique<Impl>()) {}

LocoAdapter::~LocoAdapter() noexcept {
  try {
    static_cast<void>(Shutdown());
  } catch (...) {
  }
}

SdkResult LocoAdapter::Initialize(const SdkConfiguration& configuration) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (impl_->initialized_) {
    return internal::Success("Unitree locomotion adapter already initialized");
  }

  SdkResult validation = internal::ValidateSdkConfiguration(configuration);
  if (!validation.Succeeded()) {
    return validation;
  }

  try {
    ::unitree::robot::ChannelFactory::Instance()->Init(static_cast<int>(configuration.domain_id),
                                                       configuration.network_interface);
    impl_->client_ = std::make_unique<::unitree::robot::g1::LocoClient>();
    impl_->client_->Init();
    impl_->client_->SetTimeout(internal::TimeoutSeconds(configuration.timeout));
  } catch (const std::exception& exception) {
    impl_->client_.reset();
    impl_->initialized_ = false;
    impl_->connected_ = false;
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             std::string{"Unitree locomotion adapter initialization failed: "} +
                                 exception.what());
  } catch (...) {
    impl_->client_.reset();
    impl_->initialized_ = false;
    impl_->connected_ = false;
    return internal::Failure(
        SdkErrorCode::kConnectionFailed,
        "Unitree locomotion adapter initialization failed with an unknown exception");
  }

  impl_->configuration_ = configuration;
  impl_->initialized_ = true;
  impl_->connected_ = false;
  return internal::Success("Unitree locomotion adapter initialized");
}

SdkResult LocoAdapter::Shutdown() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  impl_->client_.reset();
  impl_->initialized_ = false;
  impl_->connected_ = false;
  return internal::Success("Unitree locomotion adapter shut down");
}

SdkResult LocoAdapter::Connect() {
  std::int32_t fsm_id = -1;
  return GetFsmId(fsm_id);
}

SdkResult LocoAdapter::Disconnect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  impl_->connected_ = false;
  return internal::Success("Unitree locomotion adapter disconnected");
}

SdkResult LocoAdapter::GetFsmId(std::int32_t& fsm_id) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireInitialized("GetFsmId");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  int sdk_fsm_id = -1;
  SdkResult result = internal::InvokeSdkCommand(
      "GetFsmId", SdkErrorCode::kConnectionFailed,
      [this, &sdk_fsm_id]() { return impl_->client_->GetFsmId(sdk_fsm_id); });
  impl_->connected_ = result.Succeeded();
  fsm_id = static_cast<std::int32_t>(sdk_fsm_id);
  return result;
}

SdkResult LocoAdapter::Stand() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Stand");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Stand", SdkErrorCode::kRobotFault,
                                    [this]() { return impl_->client_->StandUp(); });
}

SdkResult LocoAdapter::BalanceStand() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("BalanceStand");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("BalanceStand", SdkErrorCode::kRobotFault,
                                    [this]() { return impl_->client_->BalanceStand(); });
}

SdkResult LocoAdapter::Sit() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Sit");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Sit", SdkErrorCode::kRobotFault,
                                    [this]() { return impl_->client_->Sit(); });
}

SdkResult LocoAdapter::Walk(const SdkVelocityCommand& velocity) {
  if (!IsFinite(velocity)) {
    return internal::Failure(SdkErrorCode::kUnknown,
                             "walk command contains a non-finite velocity value");
  }

  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Walk");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Walk", SdkErrorCode::kRobotFault, [this, &velocity]() {
    return impl_->client_->Move(velocity.linear_x, velocity.linear_y, velocity.angular_z);
  });
}

SdkResult LocoAdapter::SetVelocity(const SdkVelocityCommand& velocity) {
  if (!IsFinite(velocity)) {
    return internal::Failure(SdkErrorCode::kUnknown,
                             "velocity command contains a non-finite value");
  }

  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("SetVelocity");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("SetVelocity", SdkErrorCode::kRobotFault, [this, &velocity]() {
    return impl_->client_->SetVelocity(velocity.linear_x, velocity.linear_y, velocity.angular_z);
  });
}

SdkResult LocoAdapter::Stop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireInitialized("Stop");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Stop", SdkErrorCode::kRobotFault,
                                    [this]() { return impl_->client_->StopMove(); });
}

SdkResult LocoAdapter::EmergencyStop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireInitialized("EmergencyStop");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  SdkResult stop_result =
      internal::InvokeSdkCommand("EmergencyStop.StopMove", SdkErrorCode::kRobotFault,
                                 [this]() { return impl_->client_->StopMove(); });
  SdkResult damp_result = internal::InvokeSdkCommand(
      "EmergencyStop.Damp", SdkErrorCode::kRobotFault, [this]() { return impl_->client_->Damp(); });

  impl_->connected_ = false;
  if (!stop_result.Succeeded()) {
    return stop_result;
  }

  return damp_result;
}

bool LocoAdapter::IsInitialized() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->initialized_;
}

bool LocoAdapter::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->connected_;
}

} // namespace humanoid::plugins::unitree::sdk
