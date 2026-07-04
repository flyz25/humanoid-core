#include "SdkWrapper.h"

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include "SdkConverter.h"

#if defined(__linux__)
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <unitree/robot/channel/channel_factory.hpp>
#include <unitree/robot/g1/loco/g1_loco_client.hpp>

namespace humanoid::plugins::unitree::sdk {
namespace {

constexpr float kMillisecondsPerSecond = 1000.0F;
constexpr std::chrono::milliseconds kDefaultHeartbeatInterval{500};
constexpr std::chrono::milliseconds kDefaultReconnectInterval{1000};
constexpr std::chrono::milliseconds kDefaultConnectionTimeout{1500};

#if defined(__linux__)
class FileDescriptor final {
public:
  explicit FileDescriptor(int descriptor) noexcept : descriptor_(descriptor) {}

  ~FileDescriptor() noexcept {
    if (descriptor_ >= 0) {
      static_cast<void>(::close(descriptor_));
    }
  }

  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor& operator=(const FileDescriptor&) = delete;
  FileDescriptor(FileDescriptor&&) = delete;
  FileDescriptor& operator=(FileDescriptor&&) = delete;

  [[nodiscard]] bool IsValid() const noexcept { return descriptor_ >= 0; }

private:
  int descriptor_{-1};
};
#endif

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

[[nodiscard]] bool NetworkInterfaceExists(const std::string& network_interface) {
#if defined(__linux__)
  std::error_code error;
  const std::filesystem::path interface_path =
      std::filesystem::path{"/sys/class/net"} / network_interface;
  return std::filesystem::exists(interface_path, error);
#else
  static_cast<void>(network_interface);
  return true;
#endif
}

[[nodiscard]] bool CanOpenRouteNetlinkSocket() noexcept {
#if defined(__linux__)
  const FileDescriptor descriptor{::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE)};
  return descriptor.IsValid();
#else
  return true;
#endif
}

[[nodiscard]] SdkResult ValidateSdkRuntimeEnvironment(const SdkConfiguration& configuration) {
  if (!NetworkInterfaceExists(configuration.network_interface)) {
    return Failure(SdkErrorCode::kConnectionFailed,
                   "network interface does not exist: " + configuration.network_interface);
  }

  if (!CanOpenRouteNetlinkSocket()) {
    return Failure(SdkErrorCode::kConnectionFailed,
                   "route netlink socket is unavailable; Unitree SDK2 transport cannot be "
                   "initialized in this environment");
  }

  return Success("Unitree SDK2 runtime environment validated");
}

[[nodiscard]] std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>
NowTimestamp() {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
}

[[nodiscard]] std::chrono::milliseconds PositiveOrDefault(std::chrono::milliseconds value,
                                                          std::chrono::milliseconds default_value) {
  return value.count() > 0 ? value : default_value;
}

[[nodiscard]] SdkCommunicationOptions
NormalizeCommunicationOptions(SdkCommunicationOptions options) {
  options.heartbeat_interval =
      PositiveOrDefault(options.heartbeat_interval, kDefaultHeartbeatInterval);
  options.reconnect_interval =
      PositiveOrDefault(options.reconnect_interval, kDefaultReconnectInterval);
  options.connection_timeout =
      PositiveOrDefault(options.connection_timeout, kDefaultConnectionTimeout);

  if (options.connection_timeout < options.heartbeat_interval) {
    options.connection_timeout = options.heartbeat_interval;
  }

  return options;
}

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

void UpdateStateSample(SdkRobotState& state, SdkConnectionState connection_state,
                       SdkMotionMode motion_mode, std::int32_t fsm_id, bool emergency_stop,
                       std::int32_t fault_code) {
  state.connection_state = connection_state;
  state.motion_mode = motion_mode;
  state.emergency_stop = emergency_stop;
  state.fault_code = fault_code;
  state.fsm_id = fsm_id;
  state.timestamp = NowTimestamp();
}

void UpdateStateAfterCommand(SdkRobotState& state, SdkConnectionState connection_state,
                             SdkMotionMode motion_mode, bool succeeded) {
  UpdateStateSample(state, connection_state, motion_mode, state.fsm_id,
                    connection_state == SdkConnectionState::kFaulted, succeeded ? 0 : 1);
}

void PublishStateUpdate(const SdkWrapper::StateUpdateCallback& callback,
                        const SdkRobotState& state) noexcept {
  if (!callback) {
    return;
  }

  try {
    callback(ToCoreRobotState(state));
  } catch (...) {
  }
}

} // namespace

class SdkWrapper::Impl final {
public:
  Impl() = default;
  ~Impl() noexcept = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  std::unique_ptr<::unitree::robot::g1::LocoClient> client_;
  mutable std::mutex mutex_;
  std::condition_variable communication_condition_;
  std::thread communication_thread_;
  SdkConfiguration configuration_{};
  SdkRobotState latest_state_{};
  SdkCommunicationOptions communication_options_{};
  SdkWrapper::StateUpdateCallback state_update_callback_;
  std::chrono::steady_clock::time_point last_successful_heartbeat_{};
  std::chrono::steady_clock::time_point last_reconnect_attempt_{};
  std::uint64_t heartbeat_count_{0};
  std::uint64_t reconnect_attempt_count_{0};
  SdkConnectionState connection_state_{SdkConnectionState::kUninitialized};
  SdkMotionMode motion_mode_{SdkMotionMode::kUnknown};
  bool initialized_{false};
  bool connected_{false};
  bool communication_running_{false};
  bool stop_communication_requested_{false};
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

  SdkResult environment_result = ValidateSdkRuntimeEnvironment(configuration);
  if (!environment_result.Succeeded()) {
    impl_->connection_state_ = SdkConnectionState::kFaulted;
    UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1,
                      false, 0);
    return environment_result;
  }

  try {
    ::unitree::robot::ChannelFactory::Instance()->Init(static_cast<int>(configuration.domain_id),
                                                       configuration.network_interface);
    impl_->client_ = std::make_unique<::unitree::robot::g1::LocoClient>();
    impl_->client_->Init();
    impl_->client_->SetTimeout(TimeoutSeconds(configuration.timeout));
  } catch (const std::exception& exception) {
    impl_->client_.reset();
    impl_->connection_state_ = SdkConnectionState::kFaulted;
    return Failure(SdkErrorCode::kConnectionFailed,
                   std::string{"Unitree SDK2 initialization failed: "} + exception.what());
  } catch (...) {
    impl_->client_.reset();
    impl_->connection_state_ = SdkConnectionState::kFaulted;
    return Failure(SdkErrorCode::kConnectionFailed,
                   "Unitree SDK2 initialization failed with an unknown exception");
  }

  impl_->configuration_ = configuration;
  impl_->initialized_ = true;
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kIdle;
  impl_->connection_state_ = SdkConnectionState::kInitialized;
  impl_->latest_state_ = SdkRobotState{};
  UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1, false,
                    0);
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
                       [this, &fsm_id]() { return impl_->client_->GetFsmId(fsm_id); });
  discovery.descriptor = MakeDescriptor(impl_->configuration_, discovery.result.Succeeded());

  if (!discovery.result.Succeeded()) {
    impl_->connection_state_ = SdkConnectionState::kFaulted;
  }

  return discovery;
}

SdkResult SdkWrapper::Connect() {
  StateUpdateCallback callback;
  SdkRobotState state_snapshot;
  SdkResult result;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};

    if (!impl_->initialized_) {
      return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
    }

    int fsm_id = -1;
    result = InvokeSdkCommand("Connect", SdkErrorCode::kConnectionFailed,
                              [this, &fsm_id]() { return impl_->client_->GetFsmId(fsm_id); });
    const auto now = std::chrono::steady_clock::now();
    impl_->connected_ = result.Succeeded();
    impl_->connection_state_ =
        result.Succeeded() ? SdkConnectionState::kConnected : SdkConnectionState::kFaulted;

    if (result.Succeeded()) {
      impl_->last_successful_heartbeat_ = now;
    }

    UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, fsm_id,
                      impl_->connection_state_ == SdkConnectionState::kFaulted,
                      result.Succeeded() ? 0 : 1);
    callback = impl_->state_update_callback_;
    state_snapshot = impl_->latest_state_;
  }

  PublishStateUpdate(callback, state_snapshot);
  return result;
}

SdkResult SdkWrapper::Disconnect() {
  static_cast<void>(StopCommunication());

  StateUpdateCallback callback;
  SdkRobotState state_snapshot;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    impl_->connected_ = false;
    impl_->motion_mode_ = SdkMotionMode::kIdle;
    impl_->connection_state_ = SdkConnectionState::kDisconnected;
    UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1,
                      false, 0);
    callback = impl_->state_update_callback_;
    state_snapshot = impl_->latest_state_;
  }

  PublishStateUpdate(callback, state_snapshot);
  return Success("Unitree SDK2 wrapper disconnected");
}

SdkResult SdkWrapper::StartCommunication(const SdkCommunicationOptions& options) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
  }

  if (impl_->communication_running_) {
    return Success("Unitree SDK2 communication worker already running");
  }

  impl_->communication_options_ = NormalizeCommunicationOptions(options);
  impl_->stop_communication_requested_ = false;
  impl_->communication_running_ = true;

  try {
    impl_->communication_thread_ = std::thread([this]() { RunCommunicationLoop(); });
  } catch (const std::exception& exception) {
    impl_->communication_running_ = false;
    impl_->stop_communication_requested_ = false;
    return Failure(SdkErrorCode::kUnknown,
                   std::string{"failed to start Unitree SDK2 communication worker: "} +
                       exception.what());
  } catch (...) {
    impl_->communication_running_ = false;
    impl_->stop_communication_requested_ = false;
    return Failure(SdkErrorCode::kUnknown,
                   "failed to start Unitree SDK2 communication worker: unknown exception");
  }

  return Success("Unitree SDK2 communication worker started");
}

SdkResult SdkWrapper::StopCommunication() {
  std::thread worker;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    if (!impl_->communication_running_) {
      return Success("Unitree SDK2 communication worker already stopped");
    }

    impl_->stop_communication_requested_ = true;
    worker = std::move(impl_->communication_thread_);
  }

  impl_->communication_condition_.notify_all();

  if (worker.joinable()) {
    worker.join();
  }

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    impl_->communication_running_ = false;
    impl_->stop_communication_requested_ = false;
  }

  return Success("Unitree SDK2 communication worker stopped");
}

SdkResult SdkWrapper::SynchronizeState() {
  StateUpdateCallback callback;
  SdkRobotState state_snapshot;
  SdkResult result;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};

    if (!impl_->initialized_) {
      return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
    }

    ++impl_->heartbeat_count_;

    int fsm_id = -1;
    result = InvokeSdkCommand("Heartbeat", SdkErrorCode::kConnectionFailed,
                              [this, &fsm_id]() { return impl_->client_->GetFsmId(fsm_id); });

    const auto now = std::chrono::steady_clock::now();
    if (result.Succeeded()) {
      impl_->connected_ = true;
      impl_->connection_state_ = SdkConnectionState::kConnected;
      impl_->last_successful_heartbeat_ = now;
      UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, fsm_id,
                        false, 0);
    } else {
      const bool has_previous_heartbeat =
          impl_->last_successful_heartbeat_.time_since_epoch().count() != 0;
      const bool timed_out =
          !has_previous_heartbeat || now - impl_->last_successful_heartbeat_ >=
                                         impl_->communication_options_.connection_timeout;

      if (timed_out) {
        impl_->connected_ = false;
        impl_->connection_state_ = SdkConnectionState::kDisconnected;
        UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          fsm_id, false, 0);
        result = Failure(SdkErrorCode::kTimeout, "Unitree SDK2 communication heartbeat timed out");
      }

      const bool reconnect_due =
          impl_->last_reconnect_attempt_.time_since_epoch().count() == 0 ||
          now - impl_->last_reconnect_attempt_ >= impl_->communication_options_.reconnect_interval;

      if (reconnect_due) {
        ++impl_->reconnect_attempt_count_;
        impl_->last_reconnect_attempt_ = now;

        int reconnect_fsm_id = -1;
        SdkResult reconnect_result = InvokeSdkCommand(
            "Reconnect", SdkErrorCode::kConnectionFailed,
            [this, &reconnect_fsm_id]() { return impl_->client_->GetFsmId(reconnect_fsm_id); });

        if (reconnect_result.Succeeded()) {
          impl_->connected_ = true;
          impl_->connection_state_ = SdkConnectionState::kConnected;
          impl_->last_successful_heartbeat_ = now;
          UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                            reconnect_fsm_id, false, 0);
          result = reconnect_result;
        } else if (timed_out) {
          result = Failure(SdkErrorCode::kTimeout,
                           "Unitree SDK2 communication timed out and reconnect failed");
        }
      }
    }

    callback = impl_->state_update_callback_;
    state_snapshot = impl_->latest_state_;
  }

  PublishStateUpdate(callback, state_snapshot);
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
    return impl_->client_->Move(vx, vy, omega);
  });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kWalking : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  UpdateStateAfterCommand(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          result.Succeeded());
  return result;
}

SdkResult SdkWrapper::Stop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
  }

  SdkResult result = InvokeSdkCommand("Stop", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_->StopMove(); });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kIdle : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  UpdateStateAfterCommand(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          result.Succeeded());
  return result;
}

SdkResult SdkWrapper::StandUp() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = InvokeSdkCommand("StandUp", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_->StandUp(); });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kStanding : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  UpdateStateAfterCommand(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          result.Succeeded());
  return result;
}

SdkResult SdkWrapper::BalanceStand() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = InvokeSdkCommand("BalanceStand", SdkErrorCode::kRobotFault,
                                      [this]() { return impl_->client_->BalanceStand(); });
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kStanding : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  UpdateStateAfterCommand(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          result.Succeeded());
  return result;
}

SdkResult SdkWrapper::EmergencyStop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(SdkErrorCode::kConnectionFailed, "Unitree SDK2 wrapper is not initialized");
  }

  SdkResult stop_result = InvokeSdkCommand("EmergencyStop.StopMove", SdkErrorCode::kRobotFault,
                                           [this]() { return impl_->client_->StopMove(); });
  SdkResult damp_result = InvokeSdkCommand("EmergencyStop.Damp", SdkErrorCode::kRobotFault,
                                           [this]() { return impl_->client_->Damp(); });
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kFaulted;
  impl_->connection_state_ = SdkConnectionState::kFaulted;
  UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                    impl_->latest_state_.fsm_id, true, 1);

  if (!stop_result.Succeeded()) {
    return stop_result;
  }

  return damp_result;
}

SdkResult SdkWrapper::Shutdown() {
  static_cast<void>(StopCommunication());

  StateUpdateCallback callback;
  SdkRobotState state_snapshot;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    impl_->connected_ = false;
    impl_->initialized_ = false;
    impl_->client_.reset();
    impl_->motion_mode_ = SdkMotionMode::kIdle;
    impl_->connection_state_ = SdkConnectionState::kShutdown;
    impl_->last_successful_heartbeat_ = {};
    impl_->last_reconnect_attempt_ = {};
    UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1,
                      false, 0);
    callback = impl_->state_update_callback_;
    state_snapshot = impl_->latest_state_;
  }

  PublishStateUpdate(callback, state_snapshot);
  return Success("Unitree SDK2 wrapper shut down");
}

SdkRobotState SdkWrapper::ReadRobotState() const {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->latest_state_;
}

void SdkWrapper::SetStateUpdateCallback(StateUpdateCallback callback) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  impl_->state_update_callback_ = std::move(callback);
}

SdkCommunicationStatus SdkWrapper::CommunicationStatus() const {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkCommunicationStatus status;
  status.running = impl_->communication_running_;
  status.initialized = impl_->initialized_;
  status.connected = impl_->connected_;
  status.connection_state = impl_->connection_state_;
  status.heartbeat_count = impl_->heartbeat_count_;
  status.reconnect_attempt_count = impl_->reconnect_attempt_count_;
  status.last_successful_heartbeat = impl_->last_successful_heartbeat_;
  status.last_reconnect_attempt = impl_->last_reconnect_attempt_;
  return status;
}

bool SdkWrapper::IsInitialized() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->initialized_;
}

bool SdkWrapper::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->connected_;
}

bool SdkWrapper::IsCommunicationRunning() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->communication_running_;
}

void SdkWrapper::RunCommunicationLoop() noexcept {
  while (true) {
    SdkCommunicationOptions options;
    {
      std::unique_lock<std::mutex> lock{impl_->mutex_};
      if (impl_->stop_communication_requested_) {
        break;
      }

      options = impl_->communication_options_;
      if (impl_->communication_condition_.wait_for(lock, options.heartbeat_interval, [this]() {
            return impl_->stop_communication_requested_;
          })) {
        break;
      }
    }

    try {
      static_cast<void>(SynchronizeState());
    } catch (...) {
    }
  }
}

} // namespace humanoid::plugins::unitree::sdk
