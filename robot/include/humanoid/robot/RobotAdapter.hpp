#pragma once

/**
 * @file RobotAdapter.hpp
 * @brief Defines the robot adapter interface.
 */

#include <string_view>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>

namespace humanoid::robot {

/**
 * @brief Abstract boundary for vendor SDK or simulator adapters.
 *
 * Implementations may target Unitree G1, Unitree H1, Unitree H2, simulators,
 * or test robots while keeping those dependencies outside core applications.
 */
class IRobotAdapter {
public:
  /**
   * @brief Destroys the adapter interface.
   */
  virtual ~IRobotAdapter() = default;

  /**
   * @brief Returns the adapter name.
   *
   * @return Stable adapter name.
   */
  [[nodiscard]] virtual std::string_view adapterName() const noexcept = 0;

  /**
   * @brief Returns the robot model family handled by the adapter.
   *
   * @return Stable robot model name.
   */
  [[nodiscard]] virtual std::string_view robotModel() const noexcept = 0;

  /**
   * @brief Returns the adapter lifecycle state.
   *
   * @return Adapter lifecycle state.
   */
  [[nodiscard]] virtual common::LifecycleState lifecycleState() const noexcept = 0;

  /**
   * @brief Initializes adapter resources.
   *
   * @return Operation status.
   */
  virtual common::Status initialize() = 0;

  /**
   * @brief Starts adapter activity.
   *
   * @return Operation status.
   */
  virtual common::Status start() = 0;

  /**
   * @brief Stops adapter activity.
   *
   * @return Operation status.
   */
  virtual common::Status stop() = 0;

  /**
   * @brief Releases adapter resources.
   *
   * @return Operation status.
   */
  virtual common::Status shutdown() = 0;
};

} // namespace humanoid::robot
