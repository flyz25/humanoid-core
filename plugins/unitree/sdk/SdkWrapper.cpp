#include "SdkWrapper.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include <unitree/idl/hg/LowState_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

#include "LocoAdapter.h"
#include "SdkClientSupport.h"
#include "SdkConverter.h"

namespace humanoid::plugins::unitree::sdk {
namespace {

constexpr std::chrono::milliseconds kDefaultHeartbeatInterval{500};
constexpr std::chrono::milliseconds kDefaultReconnectInterval{1000};
constexpr std::chrono::milliseconds kDefaultConnectionTimeout{1500};
constexpr const char* kLowStateTopic = "rt/lowstate";

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
  state.motion_mode_id = static_cast<std::int32_t>(motion_mode);
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

[[nodiscard]] SdkRobotState
WithLowState(const SdkRobotState& current, const ::unitree_hg::msg::dds_::LowState_& low_state,
             std::uint64_t heartbeat_count, std::uint64_t reconnect_attempt_count) {
  SdkRobotState next = current;
  next.low_state_available = true;
  next.low_state_tick = low_state.tick();
  next.robot_mode = static_cast<std::int32_t>(low_state.mode_pr());
  next.motion_mode_id = static_cast<std::int32_t>(low_state.mode_machine());
  next.heartbeat_count = heartbeat_count;
  next.reconnect_attempt_count = reconnect_attempt_count;
  next.timestamp = NowTimestamp();

  const auto& imu = low_state.imu_state();
  next.imu_quaternion = imu.quaternion();
  next.imu_angular_velocity = imu.gyroscope();
  next.imu_linear_acceleration = imu.accelerometer();
  next.imu_temperature_celsius = static_cast<float>(imu.temperature());
  const auto& rpy = imu.rpy();
  next.roll = rpy[0];
  next.pitch = rpy[1];
  next.yaw = rpy[2];

  const auto& motors = low_state.motor_state();
  next.joint_count = static_cast<std::uint32_t>(
      motors.size() > kSdkMaxJointStates ? kSdkMaxJointStates : motors.size());
  std::uint32_t aggregate_fault_code = 0U;
  for (std::uint32_t index = 0U; index < next.joint_count; ++index) {
    const auto& motor = motors[index];
    next.joint_position[index] = motor.q();
    next.joint_velocity[index] = motor.dq();
    next.joint_acceleration[index] = motor.ddq();
    next.joint_torque[index] = motor.tau_est();
    next.joint_voltage[index] = motor.vol();
    next.joint_temperature_celsius[index] = static_cast<float>(motor.temperature()[0]);
    next.joint_mode[index] = static_cast<std::uint32_t>(motor.mode());
    next.joint_fault[index] = motor.motorstate();
    if (aggregate_fault_code == 0U && motor.motorstate() != 0U) {
      aggregate_fault_code = motor.motorstate();
    }
  }

  next.fault_code =
      aggregate_fault_code > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
          ? std::numeric_limits<std::int32_t>::max()
          : static_cast<std::int32_t>(aggregate_fault_code);
  next.emergency_stop = next.fault_code != 0;
  return next;
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

  LocoAdapter loco_adapter_;
  std::unique_ptr<::unitree::robot::ChannelSubscriber<::unitree_hg::msg::dds_::LowState_>>
      low_state_subscriber_;
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
  std::unique_lock<std::mutex> lock{impl_->mutex_};

  if (impl_->initialized_) {
    return internal::Success("Unitree SDK2 wrapper already initialized");
  }

  SdkResult initialize_result = impl_->loco_adapter_.Initialize(configuration);
  if (!initialize_result.Succeeded()) {
    impl_->connection_state_ = SdkConnectionState::kFaulted;
    UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1,
                      false, 0);
    return initialize_result;
  }

  impl_->configuration_ = configuration;
  impl_->initialized_ = true;
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kIdle;
  impl_->connection_state_ = SdkConnectionState::kInitialized;
  impl_->latest_state_ = SdkRobotState{};
  UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1, false,
                    0);
  lock.unlock();

  try {
    auto subscriber =
        std::make_unique<::unitree::robot::ChannelSubscriber<::unitree_hg::msg::dds_::LowState_>>(
            kLowStateTopic);
    subscriber->InitChannel(
        [this](const void* message) {
          if (message == nullptr) {
            return;
          }

          StateUpdateCallback callback;
          SdkRobotState state_snapshot;
          {
            std::lock_guard<std::mutex> callback_lock{impl_->mutex_};
            const auto& low_state =
                *static_cast<const ::unitree_hg::msg::dds_::LowState_*>(message);
            impl_->latest_state_ =
                WithLowState(impl_->latest_state_, low_state, impl_->heartbeat_count_,
                             impl_->reconnect_attempt_count_);
            callback = impl_->state_update_callback_;
            state_snapshot = impl_->latest_state_;
          }
          PublishStateUpdate(callback, state_snapshot);
        },
        1);

    std::lock_guard<std::mutex> subscriber_lock{impl_->mutex_};
    impl_->low_state_subscriber_ = std::move(subscriber);
  } catch (...) {
    std::lock_guard<std::mutex> subscriber_lock{impl_->mutex_};
    impl_->low_state_subscriber_.reset();
  }
  return internal::Success("Unitree SDK2 wrapper initialized");
}

SdkDiscoveryResult SdkWrapper::DiscoverRobot() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  SdkDiscoveryResult discovery;
  discovery.descriptor = MakeDescriptor(impl_->configuration_, false);

  if (!impl_->initialized_) {
    discovery.result = internal::Failure(SdkErrorCode::kConnectionFailed,
                                         "Unitree SDK2 wrapper is not initialized");
    return discovery;
  }

  std::int32_t fsm_id = 0;
  discovery.result = impl_->loco_adapter_.GetFsmId(fsm_id);
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
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               "Unitree SDK2 wrapper is not initialized");
    }

    std::int32_t fsm_id = -1;
    result = impl_->loco_adapter_.GetFsmId(fsm_id);
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
    impl_->latest_state_.heartbeat_count = impl_->heartbeat_count_;
    impl_->latest_state_.reconnect_attempt_count = impl_->reconnect_attempt_count_;
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
    static_cast<void>(impl_->loco_adapter_.Disconnect());
    impl_->connected_ = false;
    impl_->motion_mode_ = SdkMotionMode::kIdle;
    impl_->connection_state_ = SdkConnectionState::kDisconnected;
    UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_, -1,
                      false, 0);
    callback = impl_->state_update_callback_;
    state_snapshot = impl_->latest_state_;
  }

  PublishStateUpdate(callback, state_snapshot);
  return internal::Success("Unitree SDK2 wrapper disconnected");
}

SdkResult SdkWrapper::StartCommunication(const SdkCommunicationOptions& options) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not initialized");
  }

  if (impl_->communication_running_) {
    return internal::Success("Unitree SDK2 communication worker already running");
  }

  impl_->communication_options_ = NormalizeCommunicationOptions(options);
  impl_->stop_communication_requested_ = false;
  impl_->communication_running_ = true;

  try {
    impl_->communication_thread_ = std::thread([this]() { RunCommunicationLoop(); });
  } catch (const std::exception& exception) {
    impl_->communication_running_ = false;
    impl_->stop_communication_requested_ = false;
    return internal::Failure(SdkErrorCode::kUnknown,
                             std::string{"failed to start Unitree SDK2 communication worker: "} +
                                 exception.what());
  } catch (...) {
    impl_->communication_running_ = false;
    impl_->stop_communication_requested_ = false;
    return internal::Failure(
        SdkErrorCode::kUnknown,
        "failed to start Unitree SDK2 communication worker: unknown exception");
  }

  return internal::Success("Unitree SDK2 communication worker started");
}

SdkResult SdkWrapper::StopCommunication() {
  std::thread worker;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    if (!impl_->communication_running_) {
      return internal::Success("Unitree SDK2 communication worker already stopped");
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

  return internal::Success("Unitree SDK2 communication worker stopped");
}

SdkResult SdkWrapper::SynchronizeState() {
  StateUpdateCallback callback;
  SdkRobotState state_snapshot;
  SdkResult result;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};

    if (!impl_->initialized_) {
      return internal::Failure(SdkErrorCode::kConnectionFailed,
                               "Unitree SDK2 wrapper is not initialized");
    }

    ++impl_->heartbeat_count_;

    std::int32_t fsm_id = -1;
    result = impl_->loco_adapter_.GetFsmId(fsm_id);

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
        result = internal::Failure(SdkErrorCode::kTimeout,
                                   "Unitree SDK2 communication heartbeat timed out");
      }

      const bool reconnect_due =
          impl_->last_reconnect_attempt_.time_since_epoch().count() == 0 ||
          now - impl_->last_reconnect_attempt_ >= impl_->communication_options_.reconnect_interval;

      if (reconnect_due) {
        ++impl_->reconnect_attempt_count_;
        impl_->last_reconnect_attempt_ = now;

        std::int32_t reconnect_fsm_id = -1;
        SdkResult reconnect_result = impl_->loco_adapter_.GetFsmId(reconnect_fsm_id);

        if (reconnect_result.Succeeded()) {
          impl_->connected_ = true;
          impl_->connection_state_ = SdkConnectionState::kConnected;
          impl_->last_successful_heartbeat_ = now;
          UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                            reconnect_fsm_id, false, 0);
          result = reconnect_result;
        } else if (timed_out) {
          result = internal::Failure(SdkErrorCode::kTimeout,
                                     "Unitree SDK2 communication timed out and reconnect failed");
        }
      }
    }

    impl_->latest_state_.heartbeat_count = impl_->heartbeat_count_;
    impl_->latest_state_.reconnect_attempt_count = impl_->reconnect_attempt_count_;
    callback = impl_->state_update_callback_;
    state_snapshot = impl_->latest_state_;
  }

  PublishStateUpdate(callback, state_snapshot);
  return result;
}

SdkResult SdkWrapper::Move(float vx, float vy, float omega) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = impl_->loco_adapter_.Walk(SdkVelocityCommand{vx, vy, omega});
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
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not initialized");
  }

  SdkResult result = impl_->loco_adapter_.Stop();
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
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = impl_->loco_adapter_.Stand();
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kStanding : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  UpdateStateAfterCommand(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          result.Succeeded());
  return result;
}

SdkResult SdkWrapper::Sit() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = impl_->loco_adapter_.Sit();
  impl_->motion_mode_ = result.Succeeded() ? SdkMotionMode::kSitting : SdkMotionMode::kFaulted;
  impl_->connection_state_ =
      result.Succeeded() ? impl_->connection_state_ : SdkConnectionState::kFaulted;
  UpdateStateAfterCommand(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                          result.Succeeded());
  return result;
}

SdkResult SdkWrapper::BalanceStand() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not connected");
  }

  SdkResult result = impl_->loco_adapter_.BalanceStand();
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
    return internal::Failure(SdkErrorCode::kConnectionFailed,
                             "Unitree SDK2 wrapper is not initialized");
  }

  SdkResult result = impl_->loco_adapter_.EmergencyStop();
  impl_->connected_ = false;
  impl_->motion_mode_ = SdkMotionMode::kFaulted;
  impl_->connection_state_ = SdkConnectionState::kFaulted;
  UpdateStateSample(impl_->latest_state_, impl_->connection_state_, impl_->motion_mode_,
                    impl_->latest_state_.fsm_id, true, 1);

  return result;
}

SdkResult SdkWrapper::Shutdown() {
  static_cast<void>(StopCommunication());

  StateUpdateCallback callback;
  SdkRobotState state_snapshot;

  {
    std::lock_guard<std::mutex> lock{impl_->mutex_};
    impl_->connected_ = false;
    impl_->initialized_ = false;
    if (impl_->low_state_subscriber_) {
      impl_->low_state_subscriber_->CloseChannel();
      impl_->low_state_subscriber_.reset();
    }
    static_cast<void>(impl_->loco_adapter_.Shutdown());
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
  return internal::Success("Unitree SDK2 wrapper shut down");
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
