#include <adapters/unitree/UnitreeG1Adapter.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <sstream>
#include <string>
#include <utility>

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

Result UnitreeG1Adapter::Initialize() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Initialize", "command received");

  Result validation = ValidateConfig();
  if (!validation.Succeeded()) {
    Log(logging::LogLevel::kError, "Initialize", validation.message);
    return validation;
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
      return result;
    } catch (...) {
      Result result = Failure(ErrorCode::kSDKUnavailable,
                              "Unitree SDK wrapper construction failed with an unknown exception");
      MarkFailureIfNeeded(result);
      Log(logging::LogLevel::kError, "Initialize", result.message);
      return result;
    }
  }

  if (state_manager_) {
    client_->SetRobotStateManager(state_manager_);
  }

  Result result = client_->Initialize(config_);
  if (result.Succeeded()) {
    initialized_ = true;
    connected_ = false;
    connection_state_ = RobotConnectionState::kInitialized;
  } else {
    MarkFailureIfNeeded(result);
  }

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Initialize",
      result.message);
  return result;
}

Result UnitreeG1Adapter::Connect() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Connect", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Connect", result.message);
    return result;
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
    connection_state_ = RobotConnectionState::kConnected;
  } else {
    MarkFailureIfNeeded(result);
  }

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Connect",
      result.message);
  return result;
}

Result UnitreeG1Adapter::Disconnect() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Disconnect", "command received");

  if (!client_) {
    Result result = Failure(ErrorCode::kSDKUnavailable, "Unitree SDK wrapper is unavailable");
    Log(logging::LogLevel::kError, "Disconnect", result.message);
    return result;
  }

  Result result = client_->Disconnect();
  if (result.Succeeded()) {
    connected_ = false;
    connection_state_ = RobotConnectionState::kDisconnected;
  } else {
    MarkFailureIfNeeded(result);
  }

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Disconnect",
      result.message);
  return result;
}

Result UnitreeG1Adapter::Shutdown() {
  std::lock_guard<std::mutex> lock{mutex_};
  Log(logging::LogLevel::kInfo, "Shutdown", "command received");

  if (!client_) {
    initialized_ = false;
    connected_ = false;
    connection_state_ = RobotConnectionState::kShutdown;
    Result result = Success("Unitree adapter already shut down");
    Log(logging::LogLevel::kInfo, "Shutdown", result.message);
    return result;
  }

  Result result = client_->Shutdown();
  initialized_ = false;
  connected_ = false;
  connection_state_ =
      result.Succeeded() ? RobotConnectionState::kShutdown : RobotConnectionState::kFaulted;

  Log(result.Succeeded() ? logging::LogLevel::kInfo : logging::LogLevel::kError, "Shutdown",
      result.message);
  return result;
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
  connection_state_ = RobotConnectionState::kFaulted;
  Log(result.Succeeded() ? logging::LogLevel::kCritical : logging::LogLevel::kError,
      "EmergencyStop", result.message);
  return result;
}

RobotStateResult UnitreeG1Adapter::GetRobotState() const {
  std::lock_guard<std::mutex> lock{mutex_};

  RobotState state;
  state.vendor = "Unitree";
  state.model = "G1";
  state.connection_state = connection_state_;
  state.initialized = initialized_;
  state.connected = connected_;

  return RobotStateResult{Success("robot state returned"), std::move(state)};
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
  connection_state_ = RobotConnectionState::kFaulted;
}

} // namespace humanoid::adapters::unitree
