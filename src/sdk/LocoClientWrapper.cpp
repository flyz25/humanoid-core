#include <sdk/LocoClientWrapper.h>

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

namespace humanoid::sdk {
namespace {

constexpr float kMillisecondsPerSecond = 1000.0F;

/**
 * @brief Creates a successful adapter result.
 *
 * @param message Diagnostic message.
 * @return Successful result.
 */
adapters::Result Success(std::string message) {
  return adapters::Result{adapters::ErrorCode::kSuccess, std::move(message)};
}

/**
 * @brief Creates an error adapter result.
 *
 * @param code Error code.
 * @param message Diagnostic message.
 * @return Error result.
 */
adapters::Result Failure(adapters::ErrorCode code, std::string message) {
  return adapters::Result{code, std::move(message)};
}

/**
 * @brief Converts a Unitree SDK return code to an adapter result.
 *
 * @param sdk_return Return code from Unitree SDK2.
 * @param command Command name.
 * @return Adapter result.
 */
adapters::Result FromSdkReturn(std::int32_t sdk_return, const char* command) {
  if (sdk_return == 0) {
    return Success(std::string{command} + " succeeded");
  }

  std::ostringstream message;
  message << command << " failed with Unitree SDK2 return code " << sdk_return;
  return Failure(adapters::ErrorCode::kUnknown, message.str());
}

/**
 * @brief Invokes a Unitree SDK command and translates all failures to Result.
 *
 * @tparam Operation Callable returning the SDK integer status code.
 * @param command Command name.
 * @param exception_code Error code used when the SDK throws.
 * @param operation SDK operation to invoke.
 * @return Adapter result.
 */
template <typename Operation>
adapters::Result InvokeSdkCommand(const char* command, adapters::ErrorCode exception_code,
                                  Operation operation) {
  try {
    return FromSdkReturn(static_cast<std::int32_t>(operation()), command);
  } catch (const std::exception& exception) {
    return Failure(exception_code,
                   std::string{command} + " failed with Unitree SDK2 exception: " +
                       exception.what());
  } catch (...) {
    return Failure(exception_code,
                   std::string{command} + " failed with an unknown Unitree SDK2 exception");
  }
}

/**
 * @brief Converts a timeout to the seconds value expected by Unitree SDK2.
 *
 * @param timeout Timeout in milliseconds.
 * @return Timeout in seconds.
 */
float TimeoutSeconds(std::chrono::milliseconds timeout) {
  return static_cast<float>(timeout.count()) / kMillisecondsPerSecond;
}

/**
 * @brief Reports whether a velocity component is finite.
 *
 * @param value Velocity component.
 * @return True when the value is finite.
 */
bool IsFinite(float value) noexcept { return std::isfinite(value); }

} // namespace

class LocoClientWrapper::Impl final {
public:
  /**
   * @brief Constructs the Unitree SDK client implementation.
   */
  Impl() = default;

  /**
   * @brief Stops active motion before destruction.
   */
  ~Impl() {
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

  /**
   * @brief Unitree SDK2 locomotion client.
   */
  unitree::robot::g1::LocoClient client_;

  /**
   * @brief Protects all Unitree SDK calls.
   */
  mutable std::mutex mutex_;

  /**
   * @brief True after Unitree SDK2 initialization completed.
   */
  bool initialized_{false};

  /**
   * @brief True after communication was verified.
   */
  bool connected_{false};
};

LocoClientWrapper::LocoClientWrapper() : impl_(std::make_unique<Impl>()) {}

LocoClientWrapper::~LocoClientWrapper() = default;

adapters::Result LocoClientWrapper::Initialize(const adapters::RobotConfig& config) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (impl_->initialized_) {
    return Success("Unitree G1 LocoClient already initialized");
  }

  if (config.network_interface.empty()) {
    return Failure(adapters::ErrorCode::kConnectionFailed, "network interface is empty");
  }

  if (config.timeout.count() <= 0) {
    return Failure(adapters::ErrorCode::kTimeout, "timeout must be greater than zero");
  }

  if (config.domain_id > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    return Failure(adapters::ErrorCode::kConnectionFailed, "DDS domain id exceeds supported range");
  }

  try {
    unitree::robot::ChannelFactory::Instance()->Init(static_cast<int>(config.domain_id),
                                                     config.network_interface);
    impl_->client_.Init();
    impl_->client_.SetTimeout(TimeoutSeconds(config.timeout));
  } catch (const std::exception& exception) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   std::string{"Unitree SDK2 initialization failed: "} + exception.what());
  } catch (...) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree SDK2 initialization failed with an unknown exception");
  }

  impl_->initialized_ = true;
  impl_->connected_ = false;
  return Success("Unitree G1 LocoClient initialized");
}

adapters::Result LocoClientWrapper::Connect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree G1 LocoClient is not initialized");
  }

  int fsm_id = 0;
  adapters::Result result =
      InvokeSdkCommand("Connect", adapters::ErrorCode::kConnectionFailed, [this, &fsm_id]() {
        return impl_->client_.GetFsmId(fsm_id);
      });
  impl_->connected_ = result.Succeeded();
  return result;
}

adapters::Result LocoClientWrapper::Disconnect() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Success("Unitree G1 LocoClient is already disconnected");
  }

  adapters::Result result =
      InvokeSdkCommand("Disconnect", adapters::ErrorCode::kConnectionFailed,
                       [this]() { return impl_->client_.StopMove(); });
  impl_->connected_ = false;
  return result;
}

adapters::Result LocoClientWrapper::Move(float vx, float vy, float omega) {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree G1 LocoClient is not connected");
  }

  if (!IsFinite(vx) || !IsFinite(vy) || !IsFinite(omega)) {
    return Failure(adapters::ErrorCode::kUnknown, "velocity command contains a non-finite value");
  }

  return InvokeSdkCommand("Move", adapters::ErrorCode::kRobotFault,
                          [this, vx, vy, omega]() { return impl_->client_.Move(vx, vy, omega); });
}

adapters::Result LocoClientWrapper::Stop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree G1 LocoClient is not initialized");
  }

  return InvokeSdkCommand("Stop", adapters::ErrorCode::kRobotFault,
                          [this]() { return impl_->client_.StopMove(); });
}

adapters::Result LocoClientWrapper::StandUp() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree G1 LocoClient is not connected");
  }

  return InvokeSdkCommand("StandUp", adapters::ErrorCode::kRobotFault,
                          [this]() { return impl_->client_.StandUp(); });
}

adapters::Result LocoClientWrapper::BalanceStand() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_ || !impl_->connected_) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree G1 LocoClient is not connected");
  }

  return InvokeSdkCommand("BalanceStand", adapters::ErrorCode::kRobotFault,
                          [this]() { return impl_->client_.BalanceStand(); });
}

adapters::Result LocoClientWrapper::EmergencyStop() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    return Failure(adapters::ErrorCode::kConnectionFailed,
                   "Unitree G1 LocoClient is not initialized");
  }

  adapters::Result stop_result =
      InvokeSdkCommand("EmergencyStop.StopMove", adapters::ErrorCode::kRobotFault,
                       [this]() { return impl_->client_.StopMove(); });
  adapters::Result damp_result =
      InvokeSdkCommand("EmergencyStop.Damp", adapters::ErrorCode::kRobotFault,
                       [this]() { return impl_->client_.Damp(); });
  impl_->connected_ = false;

  if (!stop_result.Succeeded()) {
    return stop_result;
  }

  return damp_result;
}

adapters::Result LocoClientWrapper::Shutdown() {
  std::lock_guard<std::mutex> lock{impl_->mutex_};

  if (!impl_->initialized_) {
    impl_->connected_ = false;
    return Success("Unitree G1 LocoClient already shut down");
  }

  adapters::Result result =
      InvokeSdkCommand("Shutdown", adapters::ErrorCode::kRobotFault,
                       [this]() { return impl_->client_.StopMove(); });
  impl_->connected_ = false;
  impl_->initialized_ = false;
  return result;
}

bool LocoClientWrapper::IsInitialized() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->initialized_;
}

bool LocoClientWrapper::IsConnected() const noexcept {
  std::lock_guard<std::mutex> lock{impl_->mutex_};
  return impl_->connected_;
}

} // namespace humanoid::sdk
