#pragma once

/**
 * @file IRobotAdapter.h
 * @brief Defines the application-facing robot adapter interface.
 */

#include <chrono>
#include <cstdint>
#include <string>

#include <humanoid/adapters/Result.h>

namespace humanoid::adapters {

/**
 * @brief High-level adapter connection state.
 */
enum class RobotConnectionState {
  kUninitialized,
  kInitialized,
  kConnected,
  kDisconnected,
  kShutdown,
  kFaulted
};

/**
 * @brief Robot configuration consumed by factories and adapters.
 */
struct RobotConfig final {
  /**
   * @brief Robot vendor name, for example "Unitree".
   */
  std::string vendor;

  /**
   * @brief Robot model name, for example "G1".
   */
  std::string model;

  /**
   * @brief Robot control IP address.
   */
  std::string ip{"192.168.123.161"};

  /**
   * @brief Network interface used by the vendor SDK transport.
   */
  std::string network_interface{"eth0"};

  /**
   * @brief Vendor SDK transport domain identifier.
   */
  std::uint32_t domain_id{0};

  /**
   * @brief Command timeout.
   */
  std::chrono::milliseconds timeout{500};

  /**
   * @brief Optional robot serial number.
   */
  std::string serial_number;

  /**
   * @brief Optional robot firmware version.
   */
  std::string firmware;
};

/**
 * @brief Generic robot state returned by adapters.
 */
struct RobotState final {
  /**
   * @brief Robot vendor name.
   */
  std::string vendor;

  /**
   * @brief Robot model name.
   */
  std::string model;

  /**
   * @brief Current adapter connection state.
   */
  RobotConnectionState connection_state{RobotConnectionState::kUninitialized};

  /**
   * @brief True after adapter resources are initialized.
   */
  bool initialized{false};

  /**
   * @brief True after communication has been established.
   */
  bool connected{false};
};

/**
 * @brief Combines robot state with a query result.
 */
struct RobotStateResult final {
  /**
   * @brief State query result.
   */
  Result result;

  /**
   * @brief Generic robot state.
   */
  RobotState state;
};

/**
 * @brief Pure abstract robot communication interface.
 */
class IRobotAdapter {
public:
  /**
   * @brief Destroys the adapter interface.
   */
  virtual ~IRobotAdapter() = default;

  /**
   * @brief Initializes adapter resources.
   *
   * @return Operation result.
   */
  virtual Result Initialize() = 0;

  /**
   * @brief Establishes or verifies communication with the robot.
   *
   * @return Operation result.
   */
  virtual Result Connect() = 0;

  /**
   * @brief Disconnects communication with the robot.
   *
   * @return Operation result.
   */
  virtual Result Disconnect() = 0;

  /**
   * @brief Releases adapter resources.
   *
   * @return Operation result.
   */
  virtual Result Shutdown() = 0;

  /**
   * @brief Commands the robot to stand up.
   *
   * @return Operation result.
   */
  virtual Result StandUp() = 0;

  /**
   * @brief Commands balanced standing.
   *
   * @return Operation result.
   */
  virtual Result BalanceStand() = 0;

  /**
   * @brief Sends a velocity command.
   *
   * @param vx Forward velocity in meters per second.
   * @param vy Lateral velocity in meters per second.
   * @param omega Yaw velocity in radians per second.
   * @return Operation result.
   */
  virtual Result Move(float vx, float vy, float omega) = 0;

  /**
   * @brief Stops active motion.
   *
   * @return Operation result.
   */
  virtual Result Stop() = 0;

  /**
   * @brief Requests an emergency stop.
   *
   * @return Operation result.
   */
  virtual Result EmergencyStop() = 0;

  /**
   * @brief Returns generic robot state.
   *
   * @return State query result.
   */
  [[nodiscard]] virtual RobotStateResult GetRobotState() const = 0;
};

} // namespace humanoid::adapters
