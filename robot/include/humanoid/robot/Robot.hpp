#pragma once

/**
 * @file Robot.hpp
 * @brief Defines the abstract robot interface.
 */

#include <string_view>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>

namespace humanoid::robot {

/**
 * @brief Abstract representation of a humanoid robot controlled by the framework.
 */
class Robot {
public:
  /**
   * @brief Destroys the robot interface.
   */
  virtual ~Robot() = default;

  /**
   * @brief Returns the robot name.
   *
   * @return Stable robot name.
   */
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;

  /**
   * @brief Returns the current lifecycle state.
   *
   * @return Robot lifecycle state.
   */
  [[nodiscard]] virtual common::LifecycleState lifecycleState() const noexcept = 0;

  /**
   * @brief Configures the robot abstraction.
   *
   * @return Operation status.
   */
  virtual common::Status configure() = 0;

  /**
   * @brief Activates the robot abstraction.
   *
   * @return Operation status.
   */
  virtual common::Status activate() = 0;

  /**
   * @brief Deactivates the robot abstraction.
   *
   * @return Operation status.
   */
  virtual common::Status deactivate() = 0;

  /**
   * @brief Shuts down the robot abstraction.
   *
   * @return Operation status.
   */
  virtual common::Status shutdown() = 0;
};

} // namespace humanoid::robot
