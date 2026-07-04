#pragma once

/**
 * @file LocoClientWrapper.h
 * @brief Defines the legacy Unitree G1 locomotion wrapper facade.
 */

#include <memory>

#include <humanoid/adapters/IRobotAdapter.h>

namespace humanoid::sdk {

/**
 * @brief RAII facade over the Unitree SDK abstraction layer.
 *
 * `LocoClientWrapper` preserves the Milestone 2 adapter-facing API while
 * delegating all SDK interaction to `plugins/unitree/sdk/SdkWrapper`. It does
 * not include Unitree SDK2 headers and does not own vendor SDK types directly.
 */
class LocoClientWrapper final {
public:
  /**
   * @brief Constructs an uninitialized wrapper.
   */
  LocoClientWrapper();

  /**
   * @brief Stops active motion and releases wrapper-owned state.
   */
  ~LocoClientWrapper();

  LocoClientWrapper(const LocoClientWrapper&) = delete;
  LocoClientWrapper& operator=(const LocoClientWrapper&) = delete;
  LocoClientWrapper(LocoClientWrapper&&) = delete;
  LocoClientWrapper& operator=(LocoClientWrapper&&) = delete;

  /**
   * @brief Initializes Unitree SDK2 communication and LocoClient.
   *
   * @param config Robot communication configuration.
   * @return Command result.
   */
  adapters::Result Initialize(const adapters::RobotConfig& config);

  /**
   * @brief Verifies communication with the robot locomotion service.
   *
   * @return Command result.
   */
  adapters::Result Connect();

  /**
   * @brief Stops active motion and marks communication disconnected.
   *
   * @return Command result.
   */
  adapters::Result Disconnect();

  /**
   * @brief Sends a velocity command.
   *
   * @param vx Forward velocity in meters per second.
   * @param vy Lateral velocity in meters per second.
   * @param omega Yaw velocity in radians per second.
   * @return Command result.
   */
  adapters::Result Move(float vx, float vy, float omega);

  /**
   * @brief Stops active velocity motion.
   *
   * @return Command result.
   */
  adapters::Result Stop();

  /**
   * @brief Requests stand-up behavior.
   *
   * @return Command result.
   */
  adapters::Result StandUp();

  /**
   * @brief Requests balance standing mode.
   *
   * @return Command result.
   */
  adapters::Result BalanceStand();

  /**
   * @brief Stops motion and commands damp mode.
   *
   * @return Command result.
   */
  adapters::Result EmergencyStop();

  /**
   * @brief Stops active motion and releases wrapper state.
   *
   * @return Command result.
   */
  adapters::Result Shutdown();

  /**
   * @brief Reports whether the SDK client has been initialized.
   *
   * @return True when initialized.
   */
  [[nodiscard]] bool IsInitialized() const noexcept;

  /**
   * @brief Reports whether communication has been verified.
   *
   * @return True when connected.
   */
  [[nodiscard]] bool IsConnected() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::sdk
