#include <humanoid/ros2/bridge/ROS2Bridge.h>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <string_view>

#include <humanoid/core/CommandPriority.h>
#include <humanoid/core/CommandStatus.h>
#include <humanoid/core/CommandType.h>

namespace humanoid::ros2::messages {
namespace {

[[nodiscard]] constexpr std::string_view
detectionTypeName(humanoid::perception::DetectionType type) noexcept {
  switch (type) {
  case humanoid::perception::DetectionType::Object:
    return "Object";
  case humanoid::perception::DetectionType::Pose:
    return "Pose";
  case humanoid::perception::DetectionType::Face:
    return "Face";
  case humanoid::perception::DetectionType::QrCode:
    return "QrCode";
  case humanoid::perception::DetectionType::Marker:
    return "Marker";
  case humanoid::perception::DetectionType::SemanticSegmentation:
    return "SemanticSegmentation";
  case humanoid::perception::DetectionType::Custom:
    return "Custom";
  }

  return "Unknown";
}

[[nodiscard]] constexpr humanoid::core::CommandType
commandTypeFromName(std::string_view name) noexcept {
  if (name == "Stand") {
    return humanoid::core::CommandType::Stand;
  }
  if (name == "Sit") {
    return humanoid::core::CommandType::Sit;
  }
  if (name == "Walk") {
    return humanoid::core::CommandType::Walk;
  }
  if (name == "Stop") {
    return humanoid::core::CommandType::Stop;
  }
  if (name == "Move") {
    return humanoid::core::CommandType::Move;
  }
  if (name == "Rotate") {
    return humanoid::core::CommandType::Rotate;
  }
  if (name == "HandOpen") {
    return humanoid::core::CommandType::HandOpen;
  }
  if (name == "HandClose") {
    return humanoid::core::CommandType::HandClose;
  }
  if (name == "PlayAudio") {
    return humanoid::core::CommandType::PlayAudio;
  }
  if (name == "StopAudio") {
    return humanoid::core::CommandType::StopAudio;
  }

  return humanoid::core::CommandType::Custom;
}

[[nodiscard]] constexpr humanoid::core::CommandPriority
commandPriorityFromName(std::string_view name) noexcept {
  if (name == "Low") {
    return humanoid::core::CommandPriority::Low;
  }
  if (name == "High") {
    return humanoid::core::CommandPriority::High;
  }
  if (name == "Critical") {
    return humanoid::core::CommandPriority::Critical;
  }

  return humanoid::core::CommandPriority::Normal;
}

[[nodiscard]] std::optional<bool> parseBool(std::string_view value) noexcept {
  if (value == "true" || value == "1") {
    return true;
  }
  if (value == "false" || value == "0") {
    return false;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<std::int64_t> parseInteger(std::string_view value) noexcept {
  std::int64_t parsed{0};
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec == std::errc{} && result.ptr == end) {
    return parsed;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<double> parseDouble(std::string_view value) noexcept {
  double parsed{0.0};
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec == std::errc{} && result.ptr == end) {
    return parsed;
  }

  return std::nullopt;
}

[[nodiscard]] humanoid::core::CommandPayloadValue parsePayloadValue(const std::string& value) {
  if (const auto parsed = parseBool(value)) {
    return *parsed;
  }
  if (const auto parsed = parseInteger(value)) {
    return *parsed;
  }
  if (const auto parsed = parseDouble(value)) {
    return *parsed;
  }

  return value;
}

} // namespace

RobotStateMessage FromRobotState(const humanoid::core::RobotState& state) noexcept {
  RobotStateMessage message{};
  message.connected = state.connection.connected;
  message.batteryLevel = state.power.batteryLevel;
  message.charging = state.power.charging;
  message.standing = state.motion.standing;
  message.walking = state.motion.walking;
  message.sitting = state.motion.sitting;
  message.linearX = state.velocity.linearX;
  message.linearY = state.velocity.linearY;
  message.angularZ = state.velocity.angularZ;
  message.positionX = state.pose.positionX;
  message.positionY = state.pose.positionY;
  message.positionZ = state.pose.positionZ;
  message.roll = state.orientation.roll;
  message.pitch = state.orientation.pitch;
  message.yaw = state.orientation.yaw;
  message.emergencyStop = state.health.emergencyStop;
  message.faultCode = state.health.faultCode;
  message.timestamp = state.timestamp;
  return message;
}

DetectionResultMessage FromDetectionResult(const humanoid::perception::DetectionResult& detection) {
  DetectionResultMessage message{};
  message.id = detection.id;
  message.type = std::string{detectionTypeName(detection.type)};
  message.confidence = detection.confidence;
  message.label = detection.label;
  message.x = detection.boundingBox.x;
  message.y = detection.boundingBox.y;
  message.width = detection.boundingBox.width;
  message.height = detection.boundingBox.height;
  message.trackingId = detection.trackingId;
  message.timestamp = detection.timestamp;

  if (detection.position.has_value()) {
    message.hasPosition = true;
    message.positionX = detection.position->x;
    message.positionY = detection.position->y;
    message.positionZ = detection.position->z;
    message.coordinateFrame = detection.position->coordinateFrame;
  }

  return message;
}

humanoid::core::Command ToCommand(const CommandMessage& message) {
  humanoid::core::Command command{};
  command.id = message.id;
  command.timestamp =
      humanoid::core::CommandTimestamp{std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())};
  command.type = commandTypeFromName(message.type);
  command.priority = commandPriorityFromName(message.priority);
  command.timeout = humanoid::core::CommandTimeout{message.timeoutMs};
  command.metadata = message.metadata;

  for (const auto& [key, value] : message.payload) {
    command.payload.emplace(key, parsePayloadValue(value));
  }

  return command;
}

CommandResultMessage FromCommandResult(const humanoid::core::CommandResult& result) {
  CommandResultMessage message{};
  message.status = std::string{humanoid::core::toString(result.status)};
  message.message = result.message;
  message.success = result.isSuccess();
  return message;
}

} // namespace humanoid::ros2::messages

namespace humanoid::ros2::bridge {
namespace {

[[nodiscard]] constexpr bool ros2RuntimeAvailable() noexcept {
#ifdef HUMANOID_CORE_HAS_ROS2_RUNTIME
  return HUMANOID_CORE_HAS_ROS2_RUNTIME != 0;
#else
  return false;
#endif
}

[[nodiscard]] humanoid::ros2::messages::CommandResultMessage
rejectedCommand(humanoid::core::CommandId id, std::string message) {
  humanoid::ros2::messages::CommandResultMessage result{};
  result.id = id;
  result.status = std::string{humanoid::core::toString(humanoid::core::CommandStatus::Rejected)};
  result.message = std::move(message);
  result.success = false;
  return result;
}

} // namespace

ROS2Bridge::ROS2Bridge(ROS2BridgeDependencies dependencies, ROS2BridgeOptions options)
    : dependencies_(std::move(dependencies)), options_(std::move(options)) {}

ROS2Bridge::~ROS2Bridge() noexcept { Stop(); }

bool ROS2Bridge::Start() {
  std::lock_guard lock{mutex_};
  if (running_) {
    return false;
  }

  running_ = true;
  if (options_.subscribeTelemetry && dependencies_.telemetryService != nullptr) {
    const auto subscription_id =
        dependencies_.telemetryService->Subscribe([this](const humanoid::core::RobotState& state) {
          const auto message = humanoid::ros2::messages::FromRobotState(state);
          std::lock_guard callback_lock{mutex_};
          last_telemetry_state_ = message;
        });

    if (subscription_id != humanoid::services::TelemetryService::kInvalidSubscriptionId) {
      telemetry_subscription_ = subscription_id;
    }
  }

  return true;
}

void ROS2Bridge::Stop() noexcept {
  std::optional<humanoid::services::TelemetryService::SubscriptionId> subscription_id;
  std::shared_ptr<humanoid::services::TelemetryService> telemetry_service;

  {
    std::lock_guard lock{mutex_};
    if (!running_ && !telemetry_subscription_.has_value()) {
      return;
    }

    running_ = false;
    subscription_id = telemetry_subscription_;
    telemetry_subscription_.reset();
    telemetry_service = dependencies_.telemetryService;
  }

  if (subscription_id.has_value() && telemetry_service != nullptr) {
    static_cast<void>(telemetry_service->Unsubscribe(*subscription_id));
  }
}

bool ROS2Bridge::IsRunning() const noexcept {
  std::lock_guard lock{mutex_};
  return running_;
}

ROS2BridgeStatus ROS2Bridge::Status() const {
  std::lock_guard lock{mutex_};
  ROS2BridgeStatus status{};
  status.running = running_;
  status.ros2RuntimeAvailable = ros2RuntimeAvailable();
  status.nodeName = options_.nodeName;
  status.publisherCount = humanoid::ros2::topics::publisherTopicCount();
  status.subscriberCount = humanoid::ros2::topics::subscriberTopicCount();
  status.serviceCount = humanoid::ros2::services::serviceCount();
  status.actionCount = humanoid::ros2::actions::actionCount();
  return status;
}

humanoid::ros2::messages::RobotStateMessage ROS2Bridge::ReadRobotState() const {
  std::shared_ptr<humanoid::core::RobotStateManager> state_manager;
  std::shared_ptr<humanoid::core::RobotAdapter> adapter;
  {
    std::lock_guard lock{mutex_};
    state_manager = dependencies_.robotStateManager;
    adapter = dependencies_.robotAdapter;
  }

  if (state_manager != nullptr) {
    return humanoid::ros2::messages::FromRobotState(state_manager->GetState());
  }

  if (adapter != nullptr) {
    return humanoid::ros2::messages::FromRobotState(adapter->GetRobotState());
  }

  return humanoid::ros2::messages::RobotStateMessage{};
}

humanoid::ros2::messages::CommandResultMessage
ROS2Bridge::SubmitCommand(const humanoid::ros2::messages::CommandMessage& command) {
  std::shared_ptr<humanoid::core::CommandDispatcher> dispatcher;
  {
    std::lock_guard lock{mutex_};
    dispatcher = dependencies_.commandDispatcher;
  }

  if (dispatcher == nullptr) {
    return rejectedCommand(command.id, "CommandDispatcher dependency is not configured");
  }

  auto result = humanoid::ros2::messages::FromCommandResult(
      dispatcher->Execute(humanoid::ros2::messages::ToCommand(command)));
  result.id = command.id;
  return result;
}

std::optional<humanoid::ros2::messages::RobotStateMessage> ROS2Bridge::LastTelemetryState() const {
  std::lock_guard lock{mutex_};
  return last_telemetry_state_;
}

} // namespace humanoid::ros2::bridge
