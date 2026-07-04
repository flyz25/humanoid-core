#pragma once

/**
 * @file SdkWrapper.h
 * @brief Defines the only Unitree SDK2 wrapper boundary.
 */

#include <functional>
#include <memory>

#include <humanoid/core/RobotState.hpp>

#include "SdkTypes.h"

namespace humanoid::plugins::unitree::sdk {

/**
 * @brief RAII boundary around Unitree SDK2.
 *
 * `SdkWrapper` is the only production component allowed to include Unitree SDK2
 * headers through its implementation file. All callers interact through
 * framework-owned types declared in `SdkTypes.h`.
 */
class SdkWrapper final {
public:
  /**
   * @brief Callback invoked after read-only SDK state synchronization.
   */
  using StateUpdateCallback = std::function<void(const humanoid::core::RobotState&)>;

  /**
   * @brief Constructs an uninitialized SDK wrapper.
   */
  SdkWrapper();

  /**
   * @brief Stops communication monitoring and releases wrapper-owned SDK state.
   */
  ~SdkWrapper() noexcept;

  SdkWrapper(const SdkWrapper&) = delete;
  SdkWrapper& operator=(const SdkWrapper&) = delete;
  SdkWrapper(SdkWrapper&&) = delete;
  SdkWrapper& operator=(SdkWrapper&&) = delete;

  /**
   * @brief Initializes Unitree SDK2 transport and locomotion client.
   *
   * @param configuration SDK abstraction configuration.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Initialize(const SdkConfiguration& configuration);

  /**
   * @brief Verifies robot discovery through the locomotion service.
   *
   * @return Discovery result.
   */
  [[nodiscard]] SdkDiscoveryResult DiscoverRobot();

  /**
   * @brief Establishes or verifies robot communication.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Connect();

  /**
   * @brief Stops read-only communication monitoring and marks robot communication disconnected.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Disconnect();

  /**
   * @brief Starts read-only heartbeat and state synchronization.
   *
   * The worker uses read-only SDK queries only. It does not command motion,
   * posture, hands, audio, or any robot actuator behavior.
   *
   * @param options Communication timing options.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult StartCommunication(const SdkCommunicationOptions& options = {});

  /**
   * @brief Stops the read-only communication worker.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult StopCommunication();

  /**
   * @brief Performs one read-only heartbeat/state synchronization cycle.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult SynchronizeState();

  /**
   * @brief Sends a velocity command.
   *
   * @param vx Forward velocity in meters per second.
   * @param vy Lateral velocity in meters per second.
   * @param omega Yaw velocity in radians per second.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Move(float vx, float vy, float omega);

  /**
   * @brief Stops active velocity motion.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Stop();

  /**
   * @brief Requests Unitree stand-up behavior.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult StandUp();

  /**
   * @brief Requests Unitree balance stand behavior.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult BalanceStand();

  /**
   * @brief Stops motion and commands damp mode.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult EmergencyStop();

  /**
   * @brief Releases SDK resources.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Shutdown();

  /**
   * @brief Returns the latest normalized state known by the SDK abstraction.
   *
   * @return Normalized robot state.
   */
  [[nodiscard]] SdkRobotState ReadRobotState() const;

  /**
   * @brief Installs a callback invoked when SDK state is synchronized.
   *
   * @param callback State update callback; an empty callback disables notifications.
   */
  void SetStateUpdateCallback(StateUpdateCallback callback);

  /**
   * @brief Returns a snapshot of the read-only communication worker status.
   *
   * @return Communication worker status.
   */
  [[nodiscard]] SdkCommunicationStatus CommunicationStatus() const;

  /**
   * @brief Reports whether SDK transport and client are initialized.
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

  /**
   * @brief Reports whether the read-only communication worker is running.
   *
   * @return True when the worker is active.
   */
  [[nodiscard]] bool IsCommunicationRunning() const noexcept;

private:
  class Impl;
  void RunCommunicationLoop() noexcept;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::plugins::unitree::sdk
