#include "HandAdapter.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "SdkClientSupport.h"

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/arm/g1_arm_action_client.hpp>

namespace humanoid::plugins::unitree::sdk {
namespace {

constexpr std::int32_t kReleaseArmActionId = 99;
constexpr std::int32_t kHandsUpActionId = 15;
constexpr std::int32_t kClapActionId = 17;
constexpr std::int32_t kHighFiveActionId = 18;
constexpr std::int32_t kHugActionId = 19;
constexpr std::int32_t kHeartActionId = 20;
constexpr std::int32_t kRejectActionId = 22;
constexpr std::int32_t kWaveActionId = 26;
constexpr std::int32_t kShakeHandActionId = 27;

[[nodiscard]] std::int32_t ToActionId(SdkHandGesture gesture) noexcept {
  switch (gesture) {
  case SdkHandGesture::kHandsUp:
    return kHandsUpActionId;
  case SdkHandGesture::kClap:
    return kClapActionId;
  case SdkHandGesture::kHighFive:
    return kHighFiveActionId;
  case SdkHandGesture::kHug:
    return kHugActionId;
  case SdkHandGesture::kHeart:
    return kHeartActionId;
  case SdkHandGesture::kReject:
    return kRejectActionId;
  case SdkHandGesture::kWave:
    return kWaveActionId;
  case SdkHandGesture::kShakeHand:
    return kShakeHandActionId;
  }

  return kReleaseArmActionId;
}

[[nodiscard]] SdkResult UnsupportedFingerCommand(const char* command) {
  return internal::Failure(
      SdkErrorCode::kUnknown,
      std::string{command} +
          " is not exposed by Unitree G1 SDK2 arm action client; command was not sent");
}

} // namespace

class HandAdapter::Impl final {
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
                               std::string{command} + " requires initialized hand client");
    }

    return internal::Success(std::string{command} + " precondition satisfied");
  }

  [[nodiscard]] SdkResult RequireConnected(const char* command) const {
    if (!initialized_ || !client_ || !connected_) {
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               std::string{command} + " requires connected hand client");
    }

    return internal::Success(std::string{command} + " precondition satisfied");
  }

  mutable std::mutex mutex_;
  std::unique_ptr<::unitree::robot::g1::G1ArmActionClient> client_;
  SdkConfiguration configuration_{};
  bool initialized_{false};
  bool connected_{false};
};

HandAdapter::HandAdapter() : impl_(std::make_unique<Impl>()) {}

HandAdapter::~HandAdapter() noexcept {
  try {
    static_cast<void>(Shutdown());
  } catch (...) {
  }
}

SdkResult HandAdapter::Initialize(const SdkConfiguration& configuration) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (impl_->initialized_) {
    return internal::Success("Unitree hand adapter already initialized");
  }

  SdkResult validation = internal::ValidateSdkConfiguration(configuration);
  if (!validation.Succeeded()) {
    return validation;
  }

  try {
    ::unitree::robot::ChannelFactory::Instance()->Init(static_cast<int>(configuration.domain_id),
                                                       configuration.network_interface);
    impl_->client_ = std::make_unique<::unitree::robot::g1::G1ArmActionClient>();
    impl_->client_->Init();
    impl_->client_->SetTimeout(internal::TimeoutSeconds(configuration.timeout));
  } catch (const std::exception& exception) {
    impl_->client_.reset();
    impl_->initialized_ = false;
    impl_->connected_ = false;
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             std::string{"Unitree hand adapter initialization failed: "} +
                                 exception.what());
  } catch (...) {
    impl_->client_.reset();
    impl_->initialized_ = false;
    impl_->connected_ = false;
    return internal::Failure(
        SdkErrorCode::kConnectionFailed,
        "Unitree hand adapter initialization failed with an unknown exception");
  }

  impl_->configuration_ = configuration;
  impl_->initialized_ = true;
  impl_->connected_ = false;
  return internal::Success("Unitree hand adapter initialized");
}

SdkResult HandAdapter::Shutdown() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  impl_->client_.reset();
  impl_->initialized_ = false;
  impl_->connected_ = false;
  return internal::Success("Unitree hand adapter shut down");
}

SdkResult HandAdapter::Connect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireInitialized("Connect");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  std::string action_list;
  SdkResult result = internal::InvokeSdkCommand(
      "Hand.Connect", SdkErrorCode::kConnectionFailed,
      [this, &action_list]() { return impl_->client_->GetActionList(action_list); });
  impl_->connected_ = result.Succeeded();
  return result;
}

SdkResult HandAdapter::Open() { return UnsupportedFingerCommand("Open"); }

SdkResult HandAdapter::Close() { return UnsupportedFingerCommand("Close"); }

SdkResult HandAdapter::Grip() { return UnsupportedFingerCommand("Grip"); }

SdkResult HandAdapter::Release() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Release");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  return internal::InvokeSdkCommand("Hand.Release", SdkErrorCode::kRobotFault, [this]() {
    return impl_->client_->ExecuteAction(kReleaseArmActionId);
  });
}

SdkResult HandAdapter::Gesture(SdkHandGesture gesture) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkResult precondition = impl_->RequireConnected("Gesture");
  if (!precondition.Succeeded()) {
    return precondition;
  }

  const std::int32_t action_id = ToActionId(gesture);
  return internal::InvokeSdkCommand("Hand.Gesture", SdkErrorCode::kRobotFault, [this, action_id]() {
    return impl_->client_->ExecuteAction(action_id);
  });
}

bool HandAdapter::IsInitialized() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->initialized_;
}

bool HandAdapter::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->connected_;
}

} // namespace humanoid::plugins::unitree::sdk
