#include <adapters/unitree/UnitreeG1Adapter.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <humanoid/logging/LogMessage.hpp>
#include <humanoid/logging/Logger.hpp>

#include <sdk/LocoClientWrapper.h>

namespace humanoid::adapters::unitree {
namespace {

constexpr float kMaxLinearVelocityMetersPerSecond = 1.5F;
constexpr float kMaxYawVelocityRadiansPerSecond = 2.0F;

/**
 * @brief Creates a successful result.
 *
 * @param message Diagnostic message.
 * @return Successful result.
 */
Result Success(std::string message) { return Result{ErrorCode::kSuccess, std::move(message)}; }

/**
 * @brief Creates an error result.
 *
 * @param code Error code.
 * @param message Diagnostic message.
 * @return Error result.
 */
Result Failure(ErrorCode code, std::string message) { return Result{code, std::move(message)}; }

humanoid::common::Status ToStatus(const Result& result) {
  if (result.Succeeded()) {
    return humanoid::common::Status::ok();
  }

  switch (result.code) {
  case ErrorCode::kSuccess:
    return humanoid::common::Status::ok();
  case ErrorCode::kSDKUnavailable:
  case ErrorCode::kConnectionFailed:
  case ErrorCode::kTimeout:
    return humanoid::common::Status::error(humanoid::common::StatusCode::kUnavailable,
                                           result.message);
  case ErrorCode::kRobotFault:
    return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                           result.message);
  case ErrorCode::kUnknown:
    return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                           result.message);
  }

  return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                         result.message);
}

humanoid::core::CommandResult ToCommandResult(const Result& result) {
  humanoid::core::CommandResult command_result;
  command_result.message = result.message;
  if (result.Succeeded()) {
    command_result.status = humanoid::core::CommandStatus::Completed;
  } else if (result.code == ErrorCode::kTimeout) {
    command_result.status = humanoid::core::CommandStatus::Timeout;
  } else {
    command_result.status = humanoid::core::CommandStatus::Failed;
  }
  return command_result;
}

/**
 * @brief Reports whether a string contains only decimal digits.
 *
 * @param value String to inspect.
 * @return True when every character is a digit.
 */
bool IsDigits(const std::string& value) {
  return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char character) {
    return std::isdigit(character) != 0;
  });
}

/**
 * @brief Parses an IPv4 octet.
 *
 * @param value Octet text.
 * @param octet Parsed octet.
 * @return True when the octet is valid.
 */
bool ParseIpv4Octet(const std::string& value, int& octet) {
  if (!IsDigits(value)) {
    return false;
  }

  try {
    octet = std::stoi(value);
  } catch (...) {
    return false;
  }

  return octet >= 0 && octet <= 255;
}

/**
 * @brief Validates dotted-decimal IPv4 text.
 *
 * @param ip IP address text.
 * @return True when the address is valid.
 */
bool IsValidIpv4Address(const std::string& ip) {
  std::istringstream stream{ip};
  std::string token;
  int octet_count = 0;

  while (std::getline(stream, token, '.')) {
    int octet = 0;
    if (!ParseIpv4Octet(token, octet)) {
      return false;
    }
    ++octet_count;
  }

  return octet_count == 4;
}

/**
 * @brief Validates bounded velocity commands.
 *
 * @param vx Forward velocity.
 * @param vy Lateral velocity.
 * @param omega Yaw velocity.
 * @return True when the command is valid.
 */
bool IsValidVelocity(float vx, float vy, float omega) noexcept {
  return std::isfinite(vx) && std::isfinite(vy) && std::isfinite(omega) &&
         std::fabs(vx) <= kMaxLinearVelocityMetersPerSecond &&
         std::fabs(vy) <= kMaxLinearVelocityMetersPerSecond &&
         std::fabs(omega) <= kMaxYawVelocityRadiansPerSecond;
}

} // namespace

UnitreeG1Adapter::UnitreeG1Adapter(RobotConfig config, std::shared_ptr<logging::ILogger> logger)
    : config_(std::move(config)), logger_(std::move(logger)) {}

UnitreeG1Adapter::UnitreeG1Adapter(RobotConfig config,
                                   std::shared_ptr<core::RobotStateManager> state_manager,
                                   std::shared_ptr<logging::ILogger> logger)
    : config_(std::move(config)), state_manager_(std::move(state_manager)),
      logger_(std::move(logger)) {}

UnitreeG1Adapter::UnitreeG1Adapter(RobotConfig config,
                                   std::unique_ptr<sdk::LocoClientWrapper> client,
                                   std::shared_ptr<logging::ILogger> logger)
    : config_(std::move(config)), client_(std::move(client)), logger_(std::move(logger)) {}

UnitreeG1Adapter::UnitreeG1Adapter(RobotConfig config,
                                   std::unique_ptr<sdk::LocoClientWrapper> client,
                                   std::shared_ptr<core::RobotStateManager> state_manager,
                                   std::shared_ptr<logging::ILogger> logger)
    : config_(std::move(config)), client_(std::move(client)),
      state_manager_(std::move(state_manager)), logger_(std::move(logger)) {}

UnitreeG1Adapter::~UnitreeG1Adapter() noexcept {
  try {
    static_cast<void>(Shutdown());
  } catch (...) {
  }
}

humanoid::common::Status UnitreeG1Adapter::Initialize() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Initialize", "command received");

  Result validation = ValidateConfig();
  if (!validation.Succeeded()) {
    Log(logging::LogLevel::kError, "Initialize", validation.message);
    return ToStatus(validation);
  }

  if (!client_) {
    try {
      client_ = std::make_unique<sdk::LocoClientWrapper>();
    } catch (const std::exception& exception) {
      Result result =
          Failure(ErrorCode::kSDKUnavailable,
                  std::string{"Unitree SDK wrapper construction failed: "} + exception.what());
      MarkFailureIfNeeded(result);
      Log(logging::LogLevel::kError, "Initialize", result.message);
      return ToStatus(result);
    } catch (...) {
      Result result = Failure(ErrorCode::kSDKUnavailable,
                              "Unitree SDK wrapper construction failed with an unknown exception");
      MarkFailureIfNeeded(result);
      Log(logging::LogLevel::kError, "Initialize", result.message);
      return ToStatus(result);
    }
  }

  if (state_manager_) {
    client_->SetRobotStateManager(state_manager_);
  }

  Result result = client_->Initialize(config_);
  if (result.Succeeded()) {
    initialized_ = true;
    connected_ = false;
  } else {
    MarkFailureIfNeeded(result);
  }

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Initialize",
      result.message);
  return ToStatus(result);
}

humanoid::common::Status UnitreeG1Adapter::Connect() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Connect", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Connect", result.message);
    return ToStatus(result);
  }

  Result result = client_->Connect();
  Result communication_result = client_->StartCommunication();
  if (!communication_result.Succeeded()) {
    Log(logging::LogLevel::kError, "Connect", communication_result.message);
  }

  if (result.Succeeded() && !communication_result.Succeeded()) {
    result = communication_result;
  }

  if (result.Succeeded()) {
    initialized_ = true;
    connected_ = true;
  } else {
    MarkFailureIfNeeded(result);
  }

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Connect",
      result.message);
  return ToStatus(result);
}

humanoid::common::Status UnitreeG1Adapter::Disconnect() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Disconnect", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Disconnect", result.message);
    return ToStatus(result);
  }

  Result result = client_->Disconnect();
  if (result.Succeeded()) {
    connected_ = false;
  } else {
    MarkFailureIfNeeded(result);
  }

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Disconnect",
      result.message);
  return ToStatus(result);
}

humanoid::common::Status UnitreeG1Adapter::Shutdown() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Shutdown", "command received");

  if (!client_) {
    initialized_ = false;
    connected_ = false;
    Result result = Success("Unitree adapter already shut down");
    Log(logging::LogLevel::kInfo, "Shutdown", result.message);
    return ToStatus(result);
  }

  Result result = client_->Shutdown();
  initialized_ = false;
  connected_ = false;

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Shutdown",
      result.message);
  return ToStatus(result);
}

Result UnitreeG1Adapter::StandUp() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "StandUp", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "StandUp", result.message);
    return result;
  }

  Result result = client_->StandUp();
  MarkFailureIfNeeded(result);
  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "StandUp",
      result.message);
  return result;
}

Result UnitreeG1Adapter::Sit() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Sit", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Sit", result.message);
    return result;
  }

  Result result = client_->Sit();
  MarkFailureIfNeeded(result);
  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Sit",
      result.message);
  return result;
}

Result UnitreeG1Adapter::BalanceStand() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "BalanceStand", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "BalanceStand", result.message);
    return result;
  }

  Result result = client_->BalanceStand();
  MarkFailureIfNeeded(result);
  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "BalanceStand",
      result.message);
  return result;
}

Result UnitreeG1Adapter::Move(float vx, float vy, float omega) {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Move", "command received");

  if (!IsValidVelocity(vx, vy, omega)) {
    Result result = Failure(ErrorCode::kUnknown, "velocity command is invalid or out of range");
    Log(logging::LogLevel::kError, "Move", result.message);
    return result;
  }

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Move", result.message);
    return result;
  }

  Result result = client_->Move(vx, vy, omega);
  MarkFailureIfNeeded(result);
  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Move",
      result.message);
  return result;
}

Result UnitreeG1Adapter::Stop() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Stop", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Stop", result.message);
    return result;
  }

  Result result = client_->Stop();
  MarkFailureIfNeeded(result);
  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Stop",
      result.message);
  return result;
}

Result UnitreeG1Adapter::EmergencyStop() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kCritical, "EmergencyStop", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "EmergencyStop", result.message);
    return result;
  }

  Result result = client_->EmergencyStop();
  connected_ = false;
  Log(result.Succeeded() ? logging::LogLevel::kCritical : logging::LogLevel::kError,
      "EmergencyStop", result.message);
  return result;
}

bool UnitreeG1Adapter::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  return connected_;
}

humanoid::core::RobotState UnitreeG1Adapter::GetRobotState() const {
  std::lock_guard<std::mutex> lock{mutex_};

  humanoid::core::RobotState state;
  state.connection.connected = connected_;
  state.motion.robotMode = initialized_ ? 1 : 0;
  state.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return state;
}

humanoid::core::RobotInformation UnitreeG1Adapter::GetRobotInformation() const {
  humanoid::core::RobotInformation information;
  information.vendor = "Unitree";
  information.model = "G1";
  information.adapterName = "UnitreeG1Adapter";
  information.serialNumber = config_.serial_number;
  information.firmwareVersion = config_.firmware;
  return information;
}

humanoid::core::RobotCapabilities UnitreeG1Adapter::GetCapabilities() const {
  humanoid::core::RobotCapabilities capabilities;
  capabilities.supportsLifecycle = true;
  capabilities.supportsConnectionManagement = true;
  capabilities.supportsStateFeedback = true;
  capabilities.supportsRobotInformation = true;
  capabilities.supportsPeriodicUpdate = true;
  capabilities.supportsPowerState = true;
  capabilities.supportsHealthState = true;
  capabilities.supportsCommandExecution = true;
  return capabilities;
}

humanoid::core::CommandCapabilitySet UnitreeG1Adapter::GetCommandCapabilities() const {
  humanoid::core::CommandCapabilitySet capabilities;
  capabilities.stand = true;
  capabilities.sit = true;
  capabilities.walk = true;
  capabilities.stop = true;
  capabilities.move = true;
  capabilities.rotate = true;
  capabilities.velocity = true;
  capabilities.emergencyStop = true;
  return capabilities;
}

humanoid::core::CommandResult
UnitreeG1Adapter::ExecuteCommand(const humanoid::core::Command& command) {
  auto number = [&command](std::string_view key) -> double {
    const auto value = command.payload.find(key);
    if (value == command.payload.end()) {
      return 0.0;
    }
    if (std::holds_alternative<double>(value->second)) {
      return std::get<double>(value->second);
    }
    if (std::holds_alternative<std::int64_t>(value->second)) {
      return static_cast<double>(std::get<std::int64_t>(value->second));
    }
    return 0.0;
  };

  switch (command.type) {
  case humanoid::core::CommandType::Stand:
    return ToCommandResult(StandUp());
  case humanoid::core::CommandType::Sit:
    return ToCommandResult(Sit());
  case humanoid::core::CommandType::Walk:
  case humanoid::core::CommandType::Move:
  case humanoid::core::CommandType::Velocity:
    return ToCommandResult(Move(static_cast<float>(number("linear_x")),
                                static_cast<float>(number("linear_y")),
                                static_cast<float>(number("angular_z"))));
  case humanoid::core::CommandType::Rotate:
    return ToCommandResult(Move(0.0F, 0.0F, static_cast<float>(number("angular_z"))));
  case humanoid::core::CommandType::Stop:
    return ToCommandResult(Stop());
  case humanoid::core::CommandType::EmergencyStop:
    return ToCommandResult(EmergencyStop());
  case humanoid::core::CommandType::HandOpen:
  case humanoid::core::CommandType::HandClose:
  case humanoid::core::CommandType::Gesture:
  case humanoid::core::CommandType::PlayAudio:
  case humanoid::core::CommandType::StopAudio:
  case humanoid::core::CommandType::SetVolume:
  case humanoid::core::CommandType::MuteAudio:
  case humanoid::core::CommandType::Custom:
    return humanoid::core::CommandResult{
        humanoid::core::CommandStatus::Rejected,
        "Command type is not supported by this Unitree factory adapter"};
  }

  return humanoid::core::CommandResult{humanoid::core::CommandStatus::Rejected,
                                       "Unknown command type"};
}

humanoid::common::Status UnitreeG1Adapter::Update() {
  std::lock_guard<std::mutex> lock{mutex_};
  if (!client_) {
    return humanoid::common::Status::error(humanoid::common::StatusCode::kUnavailable,
                                           "Unitree SDK wrapper is unavailable");
  }

  return ToStatus(client_->SynchronizeState());
}

Result UnitreeG1Adapter::ValidateConfig() const {
  if (config_.vendor != "Unitree") {
    return Failure(ErrorCode::kUnknown, "robot vendor is not Unitree");
  }

  if (config_.model != "G1") {
    return Failure(ErrorCode::kUnknown, "robot model is not G1");
  }

  if (!IsValidIpv4Address(config_.ip)) {
    return Failure(ErrorCode::kConnectionFailed, "robot IP address is not a valid IPv4 address");
  }

  if (config_.network_interface.empty()) {
    return Failure(ErrorCode::kConnectionFailed, "network interface is empty");
  }

  if (config_.timeout.count() <= 0) {
    return Failure(ErrorCode::kTimeout, "timeout must be greater than zero");
  }

  return Success("robot configuration is valid");
}

void UnitreeG1Adapter::Log(logging::LogLevel level, const char* command,
                           const std::string& message) const {
  if (!logger_) {
    return;
  }

  try {
    std::ostringstream text;
    text << command << ": " << message;
    static_cast<void>(logger_->log(logging::LogMessage{level, "UnitreeG1Adapter", text.str()}));
  } catch (...) {
  }
}

void UnitreeG1Adapter::MarkFailureIfNeeded(const Result& result) noexcept {
  if (result.Succeeded()) {
    return;
  }

  connected_ = false;
}

} // namespace humanoid::adapters::unitree
