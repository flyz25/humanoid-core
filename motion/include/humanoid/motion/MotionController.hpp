#pragma once

/**
 * @file MotionController.hpp
 * @brief Defines the abstract motion controller interface.
 */

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/motion/MotionMode.hpp>

namespace humanoid::motion {

/**
 * @brief Abstract boundary for humanoid motion control.
 */
class MotionController {
public:
  /**
   * @brief Destroys the motion controller interface.
   */
  virtual ~MotionController() = default;

  /**
   * @brief Returns the controller lifecycle state.
   *
   * @return Controller lifecycle state.
   */
  [[nodiscard]] virtual common::LifecycleState lifecycleState() const noexcept = 0;

  /**
   * @brief Returns the active motion mode.
   *
   * @return Active motion mode.
   */
  [[nodiscard]] virtual MotionMode motionMode() const noexcept = 0;

  /**
   * @brief Requests a motion mode transition.
   *
   * @param mode Requested motion mode.
   * @return Operation status.
   */
  virtual common::Status setMotionMode(MotionMode mode) = 0;

  /**
   * @brief Requests position hold behavior.
   *
   * @return Operation status.
   */
  virtual common::Status holdPosition() = 0;

  /**
   * @brief Requests controlled motion stop behavior.
   *
   * @return Operation status.
   */
  virtual common::Status stopMotion() = 0;
};

} // namespace humanoid::motion
