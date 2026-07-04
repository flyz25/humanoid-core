#pragma once

/**
 * @file MotionManager.hpp
 * @brief Defines the motion manager.
 */

#include <memory>
#include <optional>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/motion/MotionController.hpp>
#include <humanoid/motion/MotionMode.hpp>

namespace humanoid::motion {

/**
 * @brief Coordinates access to an injected motion controller interface.
 */
class MotionManager final {
public:
  /**
   * @brief Constructs an empty motion manager.
   */
  MotionManager() = default;

  /**
   * @brief Constructs a motion manager with a controller.
   *
   * @param controller Motion controller interface.
   */
  explicit MotionManager(std::shared_ptr<MotionController> controller);

  /**
   * @brief Sets the active motion controller.
   *
   * @param controller Motion controller interface.
   * @return Success when the controller is non-null.
   */
  common::Status setController(std::shared_ptr<MotionController> controller);

  /**
   * @brief Clears the active motion controller.
   */
  void clearController() noexcept;

  /**
   * @brief Reports whether a controller is attached.
   *
   * @return True when a controller interface is attached.
   */
  [[nodiscard]] bool hasController() const noexcept;

  /**
   * @brief Returns the controller lifecycle state.
   *
   * @return Active controller state, or unconfigured when no controller is attached.
   */
  [[nodiscard]] common::LifecycleState lifecycleState() const noexcept;

  /**
   * @brief Returns the active motion mode.
   *
   * @return Motion mode when a controller is attached.
   */
  [[nodiscard]] std::optional<MotionMode> motionMode() const noexcept;

  /**
   * @brief Requests a motion mode transition.
   *
   * @param mode Requested motion mode.
   * @return Operation status.
   */
  common::Status setMotionMode(MotionMode mode);

  /**
   * @brief Requests position hold behavior.
   *
   * @return Operation status.
   */
  common::Status holdPosition();

  /**
   * @brief Requests controlled motion stop behavior.
   *
   * @return Operation status.
   */
  common::Status stopMotion();

private:
  std::shared_ptr<MotionController> controller_;
};

} // namespace humanoid::motion
