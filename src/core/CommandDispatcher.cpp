#include <humanoid/core/CommandDispatcher.h>

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <future>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>

#include <humanoid/core/CommandQueue.h>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/core/RobotStateManager.hpp>

namespace humanoid::core {
namespace {

constexpr std::string_view kLinearXKey{"linear_x"};
constexpr std::string_view kLinearYKey{"linear_y"};
constexpr std::string_view kAngularZKey{"angular_z"};
constexpr std::size_t kDispatcherMaximumQueueSize{1024U};

[[nodiscard]] CommandResult Result(CommandStatus status, std::string message) {
  CommandResult result;
  result.status = status;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] bool IsKnown(CommandType type) noexcept {
  switch (type) {
  case CommandType::Stand:
  case CommandType::Sit:
  case CommandType::Walk:
  case CommandType::Stop:
  case CommandType::Move:
  case CommandType::Rotate:
  case CommandType::Velocity:
  case CommandType::EmergencyStop:
  case CommandType::HandOpen:
  case CommandType::HandClose:
  case CommandType::Gesture:
  case CommandType::PlayAudio:
  case CommandType::StopAudio:
  case CommandType::SetVolume:
  case CommandType::MuteAudio:
  case CommandType::Custom:
    return true;
  }

  return false;
}

[[nodiscard]] bool IsKnown(CommandPriority priority) noexcept {
  switch (priority) {
  case CommandPriority::Low:
  case CommandPriority::Normal:
  case CommandPriority::High:
  case CommandPriority::Critical:
    return true;
  }

  return false;
}

[[nodiscard]] std::optional<double> NumericPayloadValue(const CommandPayload& payload,
                                                        std::string_view key) {
  const auto value = payload.find(key);
  if (value == payload.end()) {
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

[[nodiscard]] bool IsFiniteFloat(double value) noexcept {
  constexpr double kLowestFloat = static_cast<double>(std::numeric_limits<float>::lowest());
  constexpr double kHighestFloat = static_cast<double>(std::numeric_limits<float>::max());
  return std::isfinite(value) && value >= kLowestFloat && value <= kHighestFloat;
}

[[nodiscard]] bool HasValidNumber(const CommandPayload& payload, std::string_view key) {
  const std::optional<double> value = NumericPayloadValue(payload, key);
  return value.has_value() && IsFiniteFloat(*value);
}

[[nodiscard]] std::optional<CommandResult> ValidatePayload(const Command& command) {
  switch (command.type) {
  case CommandType::Stand:
  case CommandType::Sit:
  case CommandType::Stop:
  case CommandType::EmergencyStop:
  case CommandType::HandOpen:
  case CommandType::HandClose:
  case CommandType::MuteAudio:
    if (!command.payload.empty()) {
      return Result(CommandStatus::Rejected, "Command type does not accept payload parameters");
    }
    return std::nullopt;

  case CommandType::Walk:
  case CommandType::Move:
  case CommandType::Velocity:
    if (command.payload.size() != 3U || !HasValidNumber(command.payload, kLinearXKey) ||
        !HasValidNumber(command.payload, kLinearYKey) ||
        !HasValidNumber(command.payload, kAngularZKey)) {
      return Result(
          CommandStatus::Rejected,
          "Velocity commands require finite linear_x, linear_y, and angular_z values");
    }
    return std::nullopt;

  case CommandType::Rotate:
    if (command.payload.size() != 1U || !HasValidNumber(command.payload, kAngularZKey)) {
      return Result(CommandStatus::Rejected, "Rotate requires one finite angular_z payload value");
    }
    return std::nullopt;

  case CommandType::Gesture:
    if (command.payload.size() != 1U ||
        command.payload.find("gesture") == command.payload.end() ||
        !std::holds_alternative<std::string>(command.payload.find("gesture")->second)) {
      return Result(CommandStatus::Rejected, "Gesture requires one string gesture payload value");
    }
    return std::nullopt;

  case CommandType::PlayAudio:
    if (command.payload.find("app_name") == command.payload.end() ||
        command.payload.find("stream_id") == command.payload.end() ||
        command.payload.find("pcm_data") == command.payload.end() ||
        !std::holds_alternative<std::string>(command.payload.find("app_name")->second) ||
        !std::holds_alternative<std::string>(command.payload.find("stream_id")->second) ||
        !std::holds_alternative<std::string>(command.payload.find("pcm_data")->second)) {
      return Result(
          CommandStatus::Rejected,
          "PlayAudio requires string app_name, stream_id, and pcm_data payload values");
    }
    return std::nullopt;

  case CommandType::StopAudio:
    if (command.payload.size() != 1U ||
        command.payload.find("app_name") == command.payload.end() ||
        !std::holds_alternative<std::string>(command.payload.find("app_name")->second)) {
      return Result(CommandStatus::Rejected, "StopAudio requires one string app_name payload value");
    }
    return std::nullopt;

  case CommandType::SetVolume: {
    const auto value = command.payload.find("volume");
    if (command.payload.size() != 1U || value == command.payload.end() ||
        !std::holds_alternative<std::int64_t>(value->second)) {
      return Result(CommandStatus::Rejected, "SetVolume requires one integer volume payload value");
    }
    const auto volume = std::get<std::int64_t>(value->second);
    if (volume < 0 || volume > 100) {
      return Result(CommandStatus::Rejected, "SetVolume volume must be in range [0, 100]");
    }
    return std::nullopt;
  }

  case CommandType::Custom:
    return std::nullopt;
  }

  return Result(CommandStatus::Rejected, "Command type is invalid");
}

[[nodiscard]] std::optional<CommandResult> ValidateCommand(const Command& command) {
  if (command.id == 0U) {
    return Result(CommandStatus::Rejected, "Command identifier must be nonzero");
  }
  if (command.timeout < CommandTimeout::zero()) {
    return Result(CommandStatus::Rejected, "Command timeout must not be negative");
  }
  if (!IsKnown(command.type)) {
    return Result(CommandStatus::Rejected, "Command type is invalid");
  }
  if (!IsKnown(command.priority)) {
    return Result(CommandStatus::Rejected, "Command priority is invalid");
  }
  if (command.hasTimeout()) {
    const CommandTimestamp now =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
    if (command.timestamp == CommandTimestamp{}) {
      return Result(CommandStatus::Rejected,
                    "Timed commands require a monotonic creation timestamp");
    }
    if (command.timestamp > now) {
      return Result(CommandStatus::Rejected, "Command timestamp must not be in the future");
    }
    if (now - command.timestamp >= command.timeout) {
      return Result(CommandStatus::Timeout, "Command expired before execution");
    }
  }

  return ValidatePayload(command);
}

[[nodiscard]] bool HasExpired(const Command& command) noexcept {
  if (!command.hasTimeout() || command.timestamp == CommandTimestamp{}) {
    return false;
  }

  const CommandTimestamp now =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return now >= command.timestamp && now - command.timestamp >= command.timeout;
}

[[nodiscard]] SafetyValidator LegacySafetyValidator() noexcept {
  SafetyValidatorOptions options;
  options.requireBatteryStateForActuatorCommands = false;
  options.enforceBatteryRange = false;
  options.rejectRobotFaults = false;
  options.enforceMotionStateConsistency = false;
  options.requireStandingForBaseMotion = false;
  return SafetyValidator{options};
}

[[nodiscard]] std::future<CommandResult> ReadyFuture(CommandResult result) {
  std::promise<CommandResult> promise;
  std::future<CommandResult> future = promise.get_future();
  promise.set_value(std::move(result));
  return future;
}

} // namespace

class CommandDispatcher::Impl final {
public:
  explicit Impl(std::shared_ptr<RobotAdapter> adapter)
      : Impl(std::move(adapter), nullptr, CommandCapabilitySet::LegacyAdapterDefaults(),
             LegacySafetyValidator(), false) {}

  Impl(std::shared_ptr<RobotAdapter> adapter,
       std::shared_ptr<const RobotStateManager> state_manager, CommandCapabilitySet capabilities,
       SafetyValidator safety_validator)
      : Impl(std::move(adapter), std::move(state_manager), capabilities, safety_validator, true) {}

  Impl(std::shared_ptr<RobotAdapter> adapter,
       std::shared_ptr<const RobotStateManager> state_manager, CommandCapabilitySet capabilities,
       SafetyValidator safety_validator, bool capabilities_overridden)
      : adapter_(std::move(adapter)), state_manager_(std::move(state_manager)),
        capabilities_(capabilities), safety_validator_(safety_validator),
        capabilities_overridden_(capabilities_overridden),
        queue_([this](const Command& command) { return Dispatch(command); },
               CommandQueueOptions{kDispatcherMaximumQueueSize, 1U}) {}

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

  [[nodiscard]] CommandResult Execute(const Command& command) {
    if (const std::optional<CommandResult> validation = ValidateCommand(command)) {
      return *validation;
    }

    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      if (!accepting_) {
        return Result(CommandStatus::Rejected, "Command dispatcher is shut down");
      }
      if (synchronous_ids_.contains(command.id) || queue_.Contains(command.id)) {
        return Result(CommandStatus::Rejected, "Command identifier is already in flight");
      }

      try {
        synchronous_ids_.insert(command.id);
      } catch (const std::exception& exception) {
        return Result(CommandStatus::Failed,
                      std::string{"Unable to track command: "} + exception.what());
      } catch (...) {
        return Result(CommandStatus::Failed, "Unable to track command");
      }
    }

    const SynchronousRegistration registration{*this, command.id};
    CommandResult result;
    try {
      result = Dispatch(command);
    } catch (const std::exception& exception) {
      result.status = CommandStatus::Failed;
      try {
        result.message = std::string{"Command dispatch failed: "} + exception.what();
      } catch (...) {
      }
    } catch (...) {
      result.status = CommandStatus::Failed;
      try {
        result.message = "Command dispatch failed with an unknown error";
      } catch (...) {
      }
    }
    return result;
  }

  [[nodiscard]] std::future<CommandResult> ExecuteAsync(Command command) {
    if (const std::optional<CommandResult> validation = ValidateCommand(command)) {
      return ReadyFuture(*validation);
    }

    std::lock_guard<std::mutex> lock{state_mutex_};
    if (!accepting_) {
      return ReadyFuture(Result(CommandStatus::Rejected, "Command dispatcher is shut down"));
    }
    if (synchronous_ids_.contains(command.id)) {
      return ReadyFuture(
          Result(CommandStatus::Rejected, "Command identifier is already in flight"));
    }
    return queue_.Enqueue(std::move(command));
  }

  [[nodiscard]] CommandResult Cancel(CommandId command_id) {
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      if (synchronous_ids_.contains(command_id)) {
        return Result(CommandStatus::Rejected,
                      "A synchronously executing command cannot be cancelled");
      }
    }
    return queue_.Cancel(command_id);
  }

  [[nodiscard]] CommandResult Shutdown() {
    {
      std::unique_lock<std::mutex> lock{state_mutex_};
      if (shutdown_complete_) {
        return Result(CommandStatus::Completed, "Command dispatcher is shut down");
      }
      if (shutdown_in_progress_) {
        state_condition_.wait(lock, [this]() { return shutdown_complete_; });
        return Result(CommandStatus::Completed, "Command dispatcher is shut down");
      }
      shutdown_in_progress_ = true;
      accepting_ = false;
    }

    CommandResult queue_result = queue_.Shutdown();

    {
      std::unique_lock<std::mutex> lock{state_mutex_};
      state_condition_.wait(lock, [this]() { return synchronous_ids_.empty(); });
      shutdown_complete_ = true;
      shutdown_in_progress_ = false;
    }
    state_condition_.notify_all();

    if (!queue_result.isSuccess()) {
      return queue_result;
    }
    return Result(CommandStatus::Completed, "Command dispatcher shut down");
  }

private:
  class SynchronousRegistration final {
  public:
    SynchronousRegistration(Impl& owner, CommandId command_id) noexcept
        : owner_(owner), command_id_(command_id) {}

    ~SynchronousRegistration() { owner_.ReleaseSynchronous(command_id_); }

    SynchronousRegistration(const SynchronousRegistration&) = delete;
    SynchronousRegistration& operator=(const SynchronousRegistration&) = delete;
    SynchronousRegistration(SynchronousRegistration&&) = delete;
    SynchronousRegistration& operator=(SynchronousRegistration&&) = delete;

  private:
    Impl& owner_;
    CommandId command_id_;
  };

  void ReleaseSynchronous(CommandId command_id) {
    {
      std::lock_guard<std::mutex> lock{state_mutex_};
      synchronous_ids_.erase(command_id);
    }
    state_condition_.notify_all();
  }

  [[nodiscard]] CommandResult Dispatch(const Command& command) {
    if (!adapter_) {
      return Result(CommandStatus::Rejected, "Robot adapter dependency is unavailable");
    }
    if (HasExpired(command)) {
      return Result(CommandStatus::Timeout, "Command expired before adapter execution");
    }

    CommandResult adapter_result;
    {
      std::lock_guard<std::mutex> lock{adapter_mutex_};
      if (HasExpired(command)) {
        return Result(CommandStatus::Timeout, "Command expired before adapter execution");
      }

      const SafetyValidationContext safety_context = BuildSafetyContextLocked();
      CommandResult safety_result = safety_validator_.Validate(command, safety_context);
      if (!safety_result.isSuccess()) {
        return safety_result;
      }

      adapter_result = adapter_->ExecuteCommand(command);
    }

    if (HasExpired(command)) {
      return Result(CommandStatus::Timeout, "Command completed after its timeout");
    }
    return adapter_result;
  }

  [[nodiscard]] SafetyValidationContext BuildSafetyContextLocked() const {
    SafetyValidationContext context;
    context.capabilities =
        capabilities_overridden_ || !adapter_ ? capabilities_ : adapter_->GetCommandCapabilities();
    context.capabilitiesAvailable = true;

    if (state_manager_) {
      context.robotState = state_manager_->GetState();
      context.robotStateAvailable = true;
      context.batteryStateAvailable = true;
      return context;
    }

    context.robotStateAvailable = false;
    context.batteryStateAvailable = false;
    try {
      context.robotState = adapter_->GetRobotState();
      context.robotStateAvailable = true;
      context.batteryStateAvailable =
          adapter_->GetCapabilities().supportsPowerState || context.robotState.power.batteryLevel > 0.0F;
    } catch (...) {
      context.robotStateAvailable = false;
    }

    return context;
  }

  std::shared_ptr<RobotAdapter> adapter_;
  std::shared_ptr<const RobotStateManager> state_manager_;
  CommandCapabilitySet capabilities_{};
  SafetyValidator safety_validator_{};
  bool capabilities_overridden_{false};
  std::mutex adapter_mutex_;
  std::mutex state_mutex_;
  std::condition_variable state_condition_;
  std::unordered_set<CommandId> synchronous_ids_;
  bool accepting_{true};
  bool shutdown_in_progress_{false};
  bool shutdown_complete_{false};
  CommandQueue queue_;
};

CommandDispatcher::CommandDispatcher(std::shared_ptr<RobotAdapter> adapter)
    : impl_(std::make_unique<Impl>(std::move(adapter))) {}

CommandDispatcher::CommandDispatcher(std::shared_ptr<RobotAdapter> adapter,
                                     std::shared_ptr<const RobotStateManager> state_manager,
                                     CommandCapabilitySet capabilities,
                                     SafetyValidator safety_validator)
    : impl_(std::make_unique<Impl>(std::move(adapter), std::move(state_manager), capabilities,
                                   safety_validator)) {}

CommandDispatcher::~CommandDispatcher() noexcept = default;

CommandResult CommandDispatcher::Execute(const Command& command) { return impl_->Execute(command); }

std::future<CommandResult> CommandDispatcher::ExecuteAsync(Command command) {
  return impl_->ExecuteAsync(std::move(command));
}

CommandResult CommandDispatcher::Cancel(CommandId command_id) { return impl_->Cancel(command_id); }

CommandResult CommandDispatcher::Shutdown() { return impl_->Shutdown(); }

} // namespace humanoid::core
