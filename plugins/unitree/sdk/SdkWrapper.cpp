#include "SdkWrapper.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/loco/g1_loco_client.hpp>

namespace humanoid::plugins::unitree::sdk {
namespace {

constexpr float kMillisecondsPerSecond = 1000.0F;

[[nodiscard]] SdkResult Success(std::string message) {
  return SdkResult{SdkErrorCode::kSuccess, std::move(message)};
}

[[nodiscard]] SdkResult Failure(SdkErrorCode code, std::string message) {
  return SdkResult{code, std::move(message)};
}

[[nodiscard]] SdkResult FromSdkReturn(std::int32_t sdk_return, const char* command) {
  if (sdk_return == 0) {
    return Success(std::string{command} + " succeeded");
  }

  std::ostringstream message;
  message << command << " failed with Unitree SDK2 return code " << sdk_return;
  return Failure(SdkErrorCode::kUnknown, message.str());
}

template <typename Operation>
[[nodiscard]] SdkResult InvokeSdkCommand(const char* command, SdkErrorCode exception_code,
                                         Operation operation) {
  try {
    return FromSdkReturn(static_cast<std::int32_t>(operation()), command);
  } catch (const std::exception& exception) {
    return Failure(exception_code, std::string{command} +
                                       " failed with Unitree SDK2 exception: " + exception.what());
  } catch (...) {
    return Failure(exception_code,
                   std::string{command} + " failed with an unknown Unitree SDK2 exception");
  }
}

[[nodiscard]] float TimeoutSeconds(std::chrono::milliseconds timeout) {
  return static_cast<float>(timeout.count()) / kMillisecondsPerSecond;
}

[[nodiscard]] bool IsFinite(float value) noexcept { return std::isfinite(value); }

[[nodiscard]] SdkRobotDescriptor MakeDescriptor(const SdkConfiguration& configuration,
                                                bool discovered) {
  SdkRobotDescriptor descriptor;
  descriptor.robot_ip = configuration.robot_ip;
  descriptor.network_interface = configuration.network_interface;
  descriptor.domain_id = configuration.domain_id;
  descriptor.serial_number = configuration.serial_number;
  descriptor.firmware_version = configuration.firmware_version;
  descriptor.discovered = discovered;
  return descriptor;
}

} // namespace

class SdkWrapper::Impl final {
public:
  Impl() = default;

  ~Impl() noexcept {
    if (initialized_) {
      try {
        static_cast<void>(client_.StopMove());
      } catch (...) {
      }
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  ::unitree::robot::g1::LocoClient client_;
  mutable std::mutex mutex_;
  SdkConfiguration configuration_{};
  SdkConnectionState connection_state_{SdkConnectionState::kUninitialized};
  SdkMotionMode motion_mode_{SdkMotionMode::kUnknown};
  bool initialized_{false};
  bool connected_{false};
};

SdkWrapper::SdkWrapper() : impl_(std::make_unique<Impl>()) {}

SdkWrapper::~SdkWrapper() noexcept {
  try {
    static_cast<void>(Shutdown());
  } catch (...) {
  }
}

SdkResult SdkWrapper::Initialize(const SdkConfiguration& configuration) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (impl_->initialized_) {
    return Success("Unitree SDK2 wrapper already initialized");
  }

  if (configuration.network_interface.empty()) {
    return Failure(SdkErrorCode::kConnectionFailed, "network interface is empty");
  }

  if (configuration.timeout.count() <= 0) {
    return Failure(SdkErrorCode::kTimeout, "timeout must be greater than zero");
  }

  if (configuration.domain_id > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    return Failure(SdkErrorCode::kConnectionFailed, "DDS domain id exceeds supported range");
  }

  try {
    ::unitree::robot::ChannelFactory::Instance()->Init(static_cast<int>(configuration.domain_id),
                                                       configuration.network_interface);
    impl_->client_.Init();
    impl_->client_.SetTimeout(TimeoutSeconds(configuration.timeout));
  } catch (const std::exception& exception) {
    impl_->connection_state_ = SdkConnectionState::kFaulted;
    return Failure(SdkErrorCode::kConnectionFailed,
                   std::string{"Unitree SDK2 initialization failed: "} + exception.what());
  } catch (...) {
    impl_->connection_state_ = SdkConnectionState::kFaulted;
    return Failure(SdkErrorCode::kConnectionFailed,
                   "Unitree SDK2 initialization failed with an unknown exception");
  }

  impl_->configuration_ = configuration;
  impl_->initialized_ = true;
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kIdle;
  impl_->connection_state_ = SdkConnectionState::kInitialized;
  return Success("Unitree SDK2 wrapper initialized");
}

SdkDiscoveryResult SdkWrapper::DiscoverRobot() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkDiscoveryResult discovery;
  discovery.descriptor = MakeDescriptor(impl_->configuration_, false);

  if (!impl_->initialized_) {
    discovery.result =
        Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
    return discovery;
  }

  int fsm_id = 0;
  discovery.result =
      InvokeSdkCommand("DiscoverRobot", SdkErrorCode::kConnectionFailed,
                       [this, &fsm_id]() { return impl_->client_.GetFsmId(fsm_id); });
  discovery.descriptor = MakeDescriptor(impl_->configuration_, discovery.result.Succeeded());

  if (!discovery.result.Succeeded()) {
    impl_->connection_state_ = SdkConnectionState::kFaulted;
  }

  return discovery;
}

SdkResult SdkWrapper::Connect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
  }

  int fsm_id = 0;
  SdkResult result =
      InvokeSdkCommand("Connect", SdkErrorCode::kConnectionFailed,
                       [this, &fsm_id]() { return impl_->client_.GetFsmId(fsm_id); });
  impl_->connected_ = result.Succeeded();
  impl_->connection_state_ =
      result.Succeeded() ? SdkConnectionState::kConnected : SdkConnectionState::kFaulted;
  return result;
}

SdkResult SdkWrapper::Disconnect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    impl_->connected_ = false;
    impl_->connection_state_ = SdkConnectionState::kDisconnected;
    return Success("Unitree SDK2 wrapper is already disconnected");
  }

  SdkResult result = InvokeSdkCommand("Disconnect", SdkErrorCode::kConnectionFailed,
                                      [this]() { return impl_->client_.StopMove(); });
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kIdle;
  impl_->connection_state_ =
      result.Succeeded() ? SdkConnectionState::kDisconnected : SdkConnectionState::kFaulted;
  return result;
}

SdkResult SdkWrapper::Move(float vx, float vy, float omega) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not connected");
  }

  if (!IsFinite(vx) || !IsFinite(vy) || !IsFinite(omega)) {
    return Failure(SdkErrorCode::kUnknown, "velocity command contains a non-finite value");
  }

  SdkResult result = InvokeSdkCommand("Move", SdkErrorCode::kRobotFault, [this, vx, vy, omega]() {
    return impl_->client_.Move(vx, vy, omega);
  });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kWalking : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  return result;
}

SdkResult SdkWrapper::Stop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
  }

  SdkResult result = InvokeSdkCommand("Stop", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_.StopMove(); });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kIdle : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  return result;
}

SdkResult SdkWrapper::StandUp() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = InvokeSdkCommand("StandUp", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_.StandUp(); });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kStanding : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  return result;
}

SdkResult SdkWrapper::BalanceStand() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = InvokeSdkCommand("BalanceStand", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_.BalanceStand(); });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kStanding : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  return result;
}

SdkResult SdkWrapper::EmergencyStop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
  }

  SdkResult stop_result = InvokeSdkCommand("EmergencyStop.StopMove", SdkErrorCode::kRobotFault,
                                           [this]() { return impl_->client_.StopMove(); });
  SdkResult damp_result = InvokeSdkCommand("EmergencyStop.Damp", SdkErrorCode::kRobotFault,
                                           [this]() { return impl_->client_.Damp(); });
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kFaulted;
  impl_->connection_state_ = SdkConnectionState::kFaulted;

  if (!stop_result.Succeeded()) {
    return stop_result;
  }

  return damp_result;
}

SdkResult SdkWrapper::Shutdown() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    impl_->connected_ = false;
    impl_->connection_state_ = SdkConnectionState::kShutdown;
    return Success("Unitree SDK2 wrapper already shut down");
  }

  SdkResult result = InvokeSdkCommand("Shutdown", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_.StopMove(); });
  impl_->connected_ = false;
  impl_->initialized_ = false;
  impl_->motion_mode_ = SdkMotionMode::kIdle;
  impl_->connection_state_ =
      result.Succeeded() ? SdkConnectionState::kShutdown : SdkConnectionState::kFaulted;
  return result;
}

SdkRobotState SdkWrapper::ReadRobotState() const {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkRobotState state;
  state.connection_state = impl_->connection_state_;
  state.motion_mode = impl_->motion_mode_;
  state.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  state.emergency_stop = impl_->connection_state_ == SdkConnectionState::kFaulted;
  state.fault_code = state.emergency_stop ? 1 : 0;
  return state;
}

bool SdkWrapper::IsInitialized() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->initialized_;
}

bool SdkWrapper::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->connected_;
}

} // namespace humanoid::plugins::unitree::sdk
