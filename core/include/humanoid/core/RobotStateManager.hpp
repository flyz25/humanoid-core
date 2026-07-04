#pragma once

/**
 * @file RobotStateManager.hpp
 * @brief Defines a thread-safe manager for the latest robot state snapshot.
 */

#include <cstdint>
#include <shared_mutex>

#include <humanoid/core/RobotState.hpp>

namespace humanoid::core {

/**
 * @brief Owns the latest vendor-independent robot state snapshot.
 *
 * RobotStateManager is a small synchronization boundary for state producers and
 * consumers. Writers update or reset the full state under `std::unique_lock`;
 * readers access snapshots or selected fields under `std::shared_lock`.
 *
 * The manager performs no heap allocation, no I/O, and no vendor-specific work.
 * Public calls synchronize only on the internal shared mutex.
 */
class RobotStateManager final {
public:
  /**
   * @brief Constructs a manager containing a default robot state.
   */
  RobotStateManager() noexcept = default;

  /**
   * @brief Destroys the manager.
   */
  ~RobotStateManager() = default;

  RobotStateManager(const RobotStateManager&) = delete;
  RobotStateManager& operator=(const RobotStateManager&) = delete;
  RobotStateManager(RobotStateManager&&) = delete;
  RobotStateManager& operator=(RobotStateManager&&) = delete;

  /**
   * @brief Replaces the latest robot state snapshot.
   *
   * @param state New robot state snapshot.
   */
  void UpdateState(const RobotState& state);

  /**
   * @brief Returns a copy of the latest robot state snapshot.
   *
   * @return Latest robot state snapshot.
   */
  [[nodiscard]] RobotState GetState() const;

  /**
   * @brief Resets the managed state to a default robot state.
   */
  void Reset();

  /**
   * @brief Reports whether the latest state is connected.
   *
   * @return True when the latest state reports an established connection.
   */
  [[nodiscard]] bool IsConnected() const;

  /**
   * @brief Returns the latest battery level.
   *
   * @return Battery charge level in percent.
   */
  [[nodiscard]] float BatteryLevel() const;

  /**
   * @brief Reports whether the latest state has emergency stop active.
   *
   * @return True when emergency stop is active.
   */
  [[nodiscard]] bool EmergencyStop() const;

  /**
   * @brief Returns the latest normalized robot fault code.
   *
   * @return Fault code; zero means no reported fault.
   */
  [[nodiscard]] std::int32_t FaultCode() const;

private:
  mutable std::shared_mutex mutex_;
  RobotState state_{};
};

} // namespace humanoid::core
