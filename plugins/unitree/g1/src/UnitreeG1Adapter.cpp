#include <humanoid/plugins/unitree/g1/UnitreeG1Adapter.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#if HUMANOID_CORE_HAS_UNITREE_SDK
#include <AudioAdapter.h>
#include <HandAdapter.h>
#include <SdkConverter.h>
#include <SdkWrapper.h>
#endif

namespace humanoid::plugins::unitree::g1 {
namespace {

[[nodiscard]] humanoid::common::Status Unavailable(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kUnavailable,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                         std::move(message));
}

[[nodiscard]] humanoid::core::CommandResult CommandResult(humanoid::core::CommandStatus status,
                                                          std::string message) {
  humanoid::core::CommandResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

#if HUMANOID_CORE_HAS_UNITREE_SDK
[[nodiscard]] std::optional<double> NumberPayload(const humanoid::core::Command& command,
                                                  std::string_view key) {
  const auto value = command.payload.find(key);
  if (value == command.payload.end()) {
    return std::nullopt;
  }
  if (std::holds_alternative<double>(value->second)) {
    return std::get<double>(value->second);
  }
  if (std::holds_alternative<std::int64_t>(value->second)) {
    return static_cast<double>(std::get<std::int64_t>(value->second));
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> StringPayload(const humanoid::core::Command& command,
                                                       std::string_view key) {
  const auto value = command.payload.find(key);
  if (value == command.payload.end() || !std::holds_alternative<std::string>(value->second)) {
    return std::nullopt;
  }
  return std::get<std::string>(value->second);
}

[[nodiscard]] humanoid::common::Status ToStatus(const sdk::SdkResult& result) {
  if (result.Succeeded()) {
    return humanoid::common::Status::ok();
  }

  switch (result.code) {
  case sdk::SdkErrorCode::kSuccess:
    return humanoid::common::Status::ok();
  case sdk::SdkErrorCode::kSdkUnavailable:
  case sdk::SdkErrorCode::kConnectionFailed:
  case sdk::SdkErrorCode::kTimeout:
    return Unavailable(result.message);
  case sdk::SdkErrorCode::kRobotFault:
    return FailedPrecondition(result.message);
  case sdk::SdkErrorCode::kUnknown:
    return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                           result.message);
  }

  return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                         result.message);
}

[[nodiscard]] humanoid::core::CommandResult ToCommandResult(const sdk::SdkResult& result) {
  if (result.Succeeded()) {
    return CommandResult(humanoid::core::CommandStatus::Completed, result.message);
  }
  if (result.code == sdk::SdkErrorCode::kTimeout) {
    return CommandResult(humanoid::core::CommandStatus::Timeout, result.message);
  }
  if (result.code == sdk::SdkErrorCode::kConnectionFailed ||
      result.code == sdk::SdkErrorCode::kSdkUnavailable) {
    return CommandResult(humanoid::core::CommandStatus::Rejected, result.message);
  }
  return CommandResult(humanoid::core::CommandStatus::Failed, result.message);
}

[[nodiscard]] std::optional<sdk::SdkHandGesture> ToGesture(std::string_view gesture) noexcept {
  if (gesture == "hands_up" || gesture == "HandsUp") {
    return sdk::SdkHandGesture::kHandsUp;
  }
  if (gesture == "clap" || gesture == "Clap") {
    return sdk::SdkHandGesture::kClap;
  }
  if (gesture == "high_five" || gesture == "HighFive") {
    return sdk::SdkHandGesture::kHighFive;
  }
  if (gesture == "hug" || gesture == "Hug") {
    return sdk::SdkHandGesture::kHug;
  }
  if (gesture == "heart" || gesture == "Heart") {
    return sdk::SdkHandGesture::kHeart;
  }
  if (gesture == "reject" || gesture == "Reject") {
    return sdk::SdkHandGesture::kReject;
  }
  if (gesture == "wave" || gesture == "Wave") {
    return sdk::SdkHandGesture::kWave;
  }
  if (gesture == "shake_hand" || gesture == "ShakeHand") {
    return sdk::SdkHandGesture::kShakeHand;
  }
  return std::nullopt;
}
#endif

} // namespace

class UnitreeG1Adapter::Impl final {
public:
  explicit Impl(sdk::SdkConfiguration configuration,
                std::shared_ptr<humanoid::core::RobotStateManager> state_manager)
      : configuration_(std::move(configuration)), state_manager_(std::move(state_manager)) {}

  ~Impl() noexcept {
    try {
      static_cast<void>(Shutdown());
    } catch (...) {
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] humanoid::common::Status Initialize() {
    std::lock_guard<std::mutex> lock{mutex_};
    if (initialized_) {
      return humanoid::common::Status::ok();
    }

#if HUMANOID_CORE_HAS_UNITREE_SDK
    sdk_wrapper_.SetStateUpdateCallback([this](const humanoid::core::RobotState& state) {
      std::shared_ptr<humanoid::core::RobotStateManager> state_manager;
      {
        std::lock_guard<std::mutex> callback_lock{mutex_};
        latest_state_ = state;
        state_manager = state_manager_;
      }
      if (state_manager) {
        state_manager->UpdateState(state);
      }
    });

    const sdk::SdkResult loco_result = sdk_wrapper_.Initialize(configuration_);
    if (!loco_result.Succeeded()) {
      return ToStatus(loco_result);
    }
    const sdk::SdkResult hand_result = hand_adapter_.Initialize(configuration_);
    hand_available_ = hand_result.Succeeded();
    const sdk::SdkResult audio_result = audio_adapter_.Initialize(configuration_);
    audio_available_ = audio_result.Succeeded();
    initialized_ = true;
    return humanoid::common::Status::ok();
#else
    return Unavailable("Unitree SDK2 abstraction target is not built");
#endif
  }

  [[nodiscard]] humanoid::common::Status Shutdown() {
#if HUMANOID_CORE_HAS_UNITREE_SDK
    static_cast<void>(sdk_wrapper_.Shutdown());
    static_cast<void>(hand_adapter_.Shutdown());
    static_cast<void>(audio_adapter_.Shutdown());
#endif
    std::lock_guard<std::mutex> lock{mutex_};
    initialized_ = false;
    connected_ = false;
    hand_available_ = false;
    audio_available_ = false;
    latest_state_ = humanoid::core::RobotState{};
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Connect() {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (!initialized_) {
        return FailedPrecondition("Unitree G1 adapter is not initialized");
      }
    }

#if HUMANOID_CORE_HAS_UNITREE_SDK
    const sdk::SdkResult connect_result = sdk_wrapper_.Connect();
    if (!connect_result.Succeeded()) {
      std::lock_guard<std::mutex> lock{mutex_};
      connected_ = false;
      return ToStatus(connect_result);
    }

    sdk::SdkCommunicationOptions options;
    options.heartbeat_interval = configuration_.timeout;
    options.reconnect_interval = configuration_.timeout * 2;
    options.connection_timeout = configuration_.timeout * 3;
    const sdk::SdkResult communication_result = sdk_wrapper_.StartCommunication(options);
    if (!communication_result.Succeeded()) {
      std::lock_guard<std::mutex> lock{mutex_};
      connected_ = false;
      return ToStatus(communication_result);
    }

    bool hand_available = false;
    bool audio_available = false;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      hand_available = hand_available_;
      audio_available = audio_available_;
    }

    const bool hand_connected = hand_available && hand_adapter_.Connect().Succeeded();
    const bool audio_connected = audio_available && audio_adapter_.Connect().Succeeded();

    std::lock_guard<std::mutex> lock{mutex_};
    hand_available_ = hand_connected;
    audio_available_ = audio_connected;
    connected_ = true;
    latest_state_ = sdk::ToCoreRobotState(sdk_wrapper_.ReadRobotState());
    latest_state_.connection.connected = true;
    if (state_manager_) {
      state_manager_->UpdateState(latest_state_);
    }
    return humanoid::common::Status::ok();
#else
    return Unavailable("Unitree SDK2 abstraction target is not built");
#endif
  }

  [[nodiscard]] humanoid::common::Status Disconnect() {
#if HUMANOID_CORE_HAS_UNITREE_SDK
    const sdk::SdkResult result = sdk_wrapper_.Disconnect();
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    latest_state_.connection.connected = false;
    if (state_manager_) {
      state_manager_->UpdateState(latest_state_);
    }
    return ToStatus(result);
#else
    std::lock_guard<std::mutex> lock{mutex_};
    connected_ = false;
    latest_state_.connection.connected = false;
    return humanoid::common::Status::ok();
#endif
  }

  [[nodiscard]] bool IsConnected() const noexcept {
    std::lock_guard<std::mutex> lock{mutex_};
    return connected_;
  }

  [[nodiscard]] humanoid::core::RobotState GetRobotState() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return latest_state_;
  }

  [[nodiscard]] humanoid::core::RobotInformation GetRobotInformation() const {
    humanoid::core::RobotInformation information;
    information.vendor = "Unitree";
    information.model = "G1";
    information.adapterName = "UnitreeG1PluginAdapter";
    information.serialNumber = configuration_.serial_number;
    information.firmwareVersion = configuration_.firmware_version;
    return information;
  }

  [[nodiscard]] humanoid::core::RobotCapabilities GetCapabilities() const {
    std::lock_guard<std::mutex> lock{mutex_};
    humanoid::core::RobotCapabilities capabilities;
    capabilities.supportsLifecycle = true;
    capabilities.supportsConnectionManagement = true;
    capabilities.supportsStateFeedback = true;
    capabilities.supportsRobotInformation = true;
    capabilities.supportsPeriodicUpdate = true;
    capabilities.supportsPowerState = true;
    capabilities.supportsPoseEstimation = true;
    capabilities.supportsHealthState = true;
    capabilities.supportsCommandExecution = true;
    return capabilities;
  }

  [[nodiscard]] humanoid::core::CommandCapabilitySet GetCommandCapabilities() const {
    std::lock_guard<std::mutex> lock{mutex_};
    humanoid::core::CommandCapabilitySet capabilities;
    capabilities.stand = true;
    capabilities.sit = true;
    capabilities.walk = true;
    capabilities.stop = true;
    capabilities.move = true;
    capabilities.rotate = true;
    capabilities.velocity = true;
    capabilities.emergencyStop = true;
    capabilities.handOpen = false;
    capabilities.handClose = false;
    capabilities.gesture = hand_available_;
    capabilities.playAudio = audio_available_;
    capabilities.stopAudio = audio_available_;
    capabilities.setVolume = audio_available_;
    capabilities.muteAudio = audio_available_;
    capabilities.custom = false;
    return capabilities;
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteCommand(const humanoid::core::Command& command) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!initialized_) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "Unitree G1 adapter is not initialized");
    }

#if HUMANOID_CORE_HAS_UNITREE_SDK
    switch (command.type) {
    case humanoid::core::CommandType::Stand:
      return ToCommandResult(sdk_wrapper_.StandUp());
    case humanoid::core::CommandType::Sit:
      return ToCommandResult(sdk_wrapper_.Sit());
    case humanoid::core::CommandType::Walk:
    case humanoid::core::CommandType::Move:
    case humanoid::core::CommandType::Velocity:
      return ExecuteVelocity(command);
    case humanoid::core::CommandType::Rotate:
      return ExecuteRotate(command);
    case humanoid::core::CommandType::Stop:
      return ToCommandResult(sdk_wrapper_.Stop());
    case humanoid::core::CommandType::EmergencyStop:
      connected_ = false;
      return ToCommandResult(sdk_wrapper_.EmergencyStop());
    case humanoid::core::CommandType::HandOpen:
      return ToCommandResult(hand_adapter_.Open());
    case humanoid::core::CommandType::HandClose:
      return ToCommandResult(hand_adapter_.Close());
    case humanoid::core::CommandType::Gesture:
      return ExecuteGesture(command);
    case humanoid::core::CommandType::PlayAudio:
      return ExecutePlayAudio(command);
    case humanoid::core::CommandType::StopAudio:
      return ExecuteStopAudio(command);
    case humanoid::core::CommandType::SetVolume:
      return ExecuteSetVolume(command);
    case humanoid::core::CommandType::MuteAudio:
      return ToCommandResult(audio_adapter_.Mute());
    case humanoid::core::CommandType::Custom:
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "Custom commands are not enabled for Unitree G1");
    }

    return CommandResult(humanoid::core::CommandStatus::Rejected, "Unknown command type");
#else
    static_cast<void>(command);
    return CommandResult(humanoid::core::CommandStatus::Rejected,
                         "Unitree SDK2 abstraction target is not built");
#endif
  }

  [[nodiscard]] humanoid::common::Status Update() {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (!initialized_) {
        return FailedPrecondition("Unitree G1 adapter is not initialized");
      }
    }

#if HUMANOID_CORE_HAS_UNITREE_SDK
    const sdk::SdkResult result = sdk_wrapper_.SynchronizeState();
    std::lock_guard<std::mutex> lock{mutex_};
    latest_state_ = sdk::ToCoreRobotState(sdk_wrapper_.ReadRobotState());
    latest_state_.connection.connected = sdk_wrapper_.IsConnected();
    if (state_manager_) {
      state_manager_->UpdateState(latest_state_);
    }
    return ToStatus(result);
#else
    return Unavailable("Unitree SDK2 abstraction target is not built");
#endif
  }

private:
#if HUMANOID_CORE_HAS_UNITREE_SDK
  [[nodiscard]] humanoid::core::CommandResult
  ExecuteVelocity(const humanoid::core::Command& command) {
    const auto linear_x = NumberPayload(command, "linear_x");
    const auto linear_y = NumberPayload(command, "linear_y");
    const auto angular_z = NumberPayload(command, "angular_z");
    if (!linear_x || !linear_y || !angular_z) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "Velocity command payload is incomplete");
    }
    return ToCommandResult(sdk_wrapper_.Move(static_cast<float>(*linear_x),
                                             static_cast<float>(*linear_y),
                                             static_cast<float>(*angular_z)));
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteRotate(const humanoid::core::Command& command) {
    const auto angular_z = NumberPayload(command, "angular_z");
    if (!angular_z) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "Rotate command payload is incomplete");
    }
    return ToCommandResult(sdk_wrapper_.Move(0.0F, 0.0F, static_cast<float>(*angular_z)));
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteGesture(const humanoid::core::Command& command) {
    const auto gesture_name = StringPayload(command, "gesture");
    if (!gesture_name) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "Gesture command payload is incomplete");
    }
    const std::optional<sdk::SdkHandGesture> gesture = ToGesture(*gesture_name);
    if (!gesture) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "Gesture command is not supported by Unitree G1");
    }
    return ToCommandResult(hand_adapter_.Gesture(*gesture));
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecutePlayAudio(const humanoid::core::Command& command) {
    const auto app_name = StringPayload(command, "app_name");
    const auto stream_id = StringPayload(command, "stream_id");
    const auto pcm_data = StringPayload(command, "pcm_data");
    if (!app_name || !stream_id || !pcm_data) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "PlayAudio command payload is incomplete");
    }
    sdk::SdkAudioPlayback playback;
    playback.app_name = *app_name;
    playback.stream_id = *stream_id;
    playback.pcm_data.assign(pcm_data->begin(), pcm_data->end());
    return ToCommandResult(audio_adapter_.Play(playback));
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteStopAudio(const humanoid::core::Command& command) {
    const auto app_name = StringPayload(command, "app_name");
    if (!app_name) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "StopAudio command payload is incomplete");
    }
    return ToCommandResult(audio_adapter_.Stop(*app_name));
  }

  [[nodiscard]] humanoid::core::CommandResult
  ExecuteSetVolume(const humanoid::core::Command& command) {
    const auto volume = NumberPayload(command, "volume");
    if (!volume || *volume < 0.0 || *volume > 100.0) {
      return CommandResult(humanoid::core::CommandStatus::Rejected,
                           "SetVolume command payload is invalid");
    }
    return ToCommandResult(audio_adapter_.SetVolume(static_cast<std::uint8_t>(*volume)));
  }

  sdk::SdkWrapper sdk_wrapper_;
  sdk::HandAdapter hand_adapter_;
  sdk::AudioAdapter audio_adapter_;
#endif

  mutable std::mutex mutex_;
  sdk::SdkConfiguration configuration_;
  std::shared_ptr<humanoid::core::RobotStateManager> state_manager_;
  humanoid::core::RobotState latest_state_{};
  bool initialized_{false};
  bool connected_{false};
  bool hand_available_{false};
  bool audio_available_{false};
};

UnitreeG1Adapter::UnitreeG1Adapter()
    : UnitreeG1Adapter(sdk::SdkConfiguration{}, nullptr) {}

UnitreeG1Adapter::UnitreeG1Adapter(
    sdk::SdkConfiguration configuration,
    std::shared_ptr<humanoid::core::RobotStateManager> state_manager)
    : impl_(std::make_unique<Impl>(std::move(configuration), std::move(state_manager))) {}

UnitreeG1Adapter::~UnitreeG1Adapter() noexcept = default;

humanoid::common::Status UnitreeG1Adapter::Initialize() { return impl_->Initialize(); }

humanoid::common::Status UnitreeG1Adapter::Shutdown() { return impl_->Shutdown(); }

humanoid::common::Status UnitreeG1Adapter::Connect() { return impl_->Connect(); }

humanoid::common::Status UnitreeG1Adapter::Disconnect() { return impl_->Disconnect(); }

bool UnitreeG1Adapter::IsConnected() const noexcept { return impl_->IsConnected(); }

humanoid::core::RobotState UnitreeG1Adapter::GetRobotState() const {
  return impl_->GetRobotState();
}

humanoid::core::RobotInformation UnitreeG1Adapter::GetRobotInformation() const {
  return impl_->GetRobotInformation();
}

humanoid::core::RobotCapabilities UnitreeG1Adapter::GetCapabilities() const {
  return impl_->GetCapabilities();
}

humanoid::core::CommandCapabilitySet UnitreeG1Adapter::GetCommandCapabilities() const {
  return impl_->GetCommandCapabilities();
}

humanoid::core::CommandResult
UnitreeG1Adapter::ExecuteCommand(const humanoid::core::Command& command) {
  return impl_->ExecuteCommand(command);
}

humanoid::common::Status UnitreeG1Adapter::Update() { return impl_->Update(); }

} // namespace humanoid::plugins::unitree::g1
