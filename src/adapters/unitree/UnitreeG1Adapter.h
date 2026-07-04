#pragma once

/**
 * @file UnitreeG1Adapter.h
 * @brief Defines the Unitree G1 robot adapter.
 */

#include <memory>
#include <mutex>

#include <humanoid/adapters/IRobotAdapter.h>

namespace humanoid::logging {
class ILogger;
enum class LogLevel;
} // namespace humanoid::logging

namespace humanoid::core {
class RobotStateManager;
} // namespace humanoid::core

namespace humanoid::sdk {
class LocoClientWrapper;
} // namespace humanoid::sdk

namespace humanoid::adapters::unitree {

/**
 * @brief Unitree G1 implementation of the generic robot adapter interface.
 */
class UnitreeG1Adapter final : public IRobotAdapter {
public:
  /**
   * @brief Constructs a Unitree G1 adapter with an owned SDK wrapper.
   *
   * @param config Robot configuration.
   * @param logger Optional logger interface.
   */
  UnitreeG1Adapter(RobotConfig config, std::shared_ptr<logging::ILogger> logger);

  /**
   * @brief Constructs a Unitree G1 adapter with an injected robot state manager.
   *
   * @param config Robot configuration.
   * @param state_manager Optional state manager updated by read-only SDK communication.
   * @param logger Optional logger interface.
   */
  UnitreeG1Adapter(RobotConfig config, std::shared_ptr<core::RobotStateManager> state_manager,
                   std::shared_ptr<logging::ILogger> logger);

  /**
   * @brief Constructs a Unitree G1 adapter with an injected SDK wrapper.
   *
   * @param config Robot configuration.
   * @param client SDK wrapper owned by the adapter.
   * @param logger Optional logger interface.
   */
  UnitreeG1Adapter(RobotConfig config, std::unique_ptr<sdk::LocoClientWrapper> client,
                   std::shared_ptr<logging::ILogger> logger);

  /**
   * @brief Constructs a Unitree G1 adapter with injected SDK and state dependencies.
   *
   * @param config Robot configuration.
   * @param client SDK wrapper owned by the adapter.
   * @param state_manager Optional state manager updated by read-only SDK communication.
   * @param logger Optional logger interface.
   */
  UnitreeG1Adapter(RobotConfig config, std::unique_ptr<sdk::LocoClientWrapper> client,
                   std::shared_ptr<core::RobotStateManager> state_manager,
                   std::shared_ptr<logging::ILogger> logger);

  /**
   * @brief Stops communication monitoring and releases adapter resources.
   */
  ~UnitreeG1Adapter() noexcept override;

  UnitreeG1Adapter(const UnitreeG1Adapter&) = delete;
  UnitreeG1Adapter& operator=(const UnitreeG1Adapter&) = delete;
  UnitreeG1Adapter(UnitreeG1Adapter&&) = delete;
  UnitreeG1Adapter& operator=(UnitreeG1Adapter&&) = delete;

  /**
   * @brief Initializes adapter resources.
   *
   * @return Operation result.
   */
  Result Initialize() override;

  /**
   * @brief Establishes or verifies robot communication.
   *
   * @return Operation result.
   */
  Result Connect() override;

  /**
   * @brief Disconnects robot communication.
   *
   * @return Operation result.
   */
  Result Disconnect() override;

  /**
   * @brief Releases adapter resources.
   *
   * @return Operation result.
   */
  Result Shutdown() override;

  /**
   * @brief Commands the robot to stand up.
   *
   * @return Operation result.
   */
  Result StandUp() override;

  /**
   * @brief Commands balance stand mode.
   *
   * @return Operation result.
   */
  Result BalanceStand() override;

  /**
   * @brief Sends a velocity command.
   *
   * @param vx Forward velocity in meters per second.
   * @param vy Lateral velocity in meters per second.
   * @param omega Yaw velocity in radians per second.
   * @return Operation result.
   */
  Result Move(float vx, float vy, float omega) override;

  /**
   * @brief Stops active motion.
   *
   * @return Operation result.
   */
  Result Stop() override;

  /**
   * @brief Requests emergency stop.
   *
   * @return Operation result.
   */
  Result EmergencyStop() override;

  /**
   * @brief Returns generic robot state.
   *
   * @return State query result.
   */
  [[nodiscard]] RobotStateResult GetRobotState() const override;

private:
  /**
   * @brief Validates robot configuration.
   *
   * @return Validation result.
   */
  [[nodiscard]] Result ValidateConfig() const;

  /**
   * @brief Logs a command event.
   *
   * @param level Severity level.
   * @param command Command name.
   * @param message Diagnostic message.
   */
  void Log(logging::LogLevel level, const char* command, const std::string& message) const;

  /**
   * @brief Updates local state after a failed SDK operation.
   *
   * @param result Operation result.
   */
  void MarkFailureIfNeeded(const Result& result) noexcept;

  mutable std::mutex mutex_;
  RobotConfig config_;
  std::unique_ptr<sdk::LocoClientWrapper> client_;
  std::shared_ptr<core::RobotStateManager> state_manager_;
  std::shared_ptr<logging::ILogger> logger_;
  RobotConnectionState connection_state_{RobotConnectionState::kUninitialized};
  bool initialized_{false};
  bool connected_{false};
};

} // namespace humanoid::adapters::unitree
