#pragma once

/**
 * @file RobotManager.hpp
 * @brief Defines the robot manager.
 */

#include <memory>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/robot/Robot.hpp>
#include <humanoid/robot/RobotAdapter.hpp>

namespace humanoid::robot {

/**
 * @brief Owns robot and adapter interfaces for lifecycle coordination.
 */
class RobotManager final {
public:
  /**
   * @brief Constructs an empty robot manager.
   */
  RobotManager() = default;

  /**
   * @brief Constructs a robot manager with a robot interface.
   *
   * @param robot Robot interface.
   */
  explicit RobotManager(std::shared_ptr<Robot> robot);

  /**
   * @brief Sets the active robot interface.
   *
   * @param robot Robot interface.
   * @return Success when the robot is non-null.
   */
  common::Status setRobot(std::shared_ptr<Robot> robot);

  /**
   * @brief Clears the active robot interface.
   */
  void clearRobot() noexcept;

  /**
   * @brief Reports whether a robot is attached.
   *
   * @return True when a robot interface is attached.
   */
  [[nodiscard]] bool hasRobot() const noexcept;

  /**
   * @brief Returns the robot lifecycle state.
   *
   * @return Active robot state, or unconfigured when no robot is attached.
   */
  [[nodiscard]] common::LifecycleState lifecycleState() const noexcept;

  /**
   * @brief Configures the active robot.
   *
   * @return Operation status.
   */
  common::Status configure();

  /**
   * @brief Activates the active robot.
   *
   * @return Operation status.
   */
  common::Status activate();

  /**
   * @brief Deactivates the active robot.
   *
   * @return Operation status.
   */
  common::Status deactivate();

  /**
   * @brief Shuts down the active robot.
   *
   * @return Operation status.
   */
  common::Status shutdown();

  /**
   * @brief Sets the active robot adapter interface.
   *
   * @param adapter Robot adapter interface.
   * @return Success when the adapter is non-null.
   */
  common::Status setAdapter(std::shared_ptr<IRobotAdapter> adapter);

  /**
   * @brief Clears the active robot adapter interface.
   */
  void clearAdapter() noexcept;

  /**
   * @brief Reports whether an adapter is attached.
   *
   * @return True when an adapter interface is attached.
   */
  [[nodiscard]] bool hasAdapter() const noexcept;

  /**
   * @brief Initializes the active adapter.
   *
   * @return Operation status.
   */
  common::Status initializeAdapter();

  /**
   * @brief Starts the active adapter.
   *
   * @return Operation status.
   */
  common::Status startAdapter();

  /**
   * @brief Stops the active adapter.
   *
   * @return Operation status.
   */
  common::Status stopAdapter();

  /**
   * @brief Shuts down the active adapter.
   *
   * @return Operation status.
   */
  common::Status shutdownAdapter();

private:
  std::shared_ptr<Robot> robot_;
  std::shared_ptr<IRobotAdapter> adapter_;
};

} // namespace humanoid::robot
