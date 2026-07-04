#pragma once

/**
 * @file GestureController.hpp
 * @brief Defines the abstract gesture controller interface.
 */

#include <string_view>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>

namespace humanoid::gesture {

/**
 * @brief Abstract boundary for named humanoid gestures.
 */
class GestureController {
public:
  /**
   * @brief Destroys the gesture controller interface.
   */
  virtual ~GestureController() = default;

  /**
   * @brief Returns the controller lifecycle state.
   *
   * @return Controller lifecycle state.
   */
  [[nodiscard]] virtual common::LifecycleState lifecycleState() const noexcept = 0;

  /**
   * @brief Reports whether a gesture is supported.
   *
   * @param gesture_id Stable gesture identifier.
   * @return True when the controller supports the gesture.
   */
  [[nodiscard]] virtual bool supportsGesture(std::string_view gesture_id) const = 0;

  /**
   * @brief Starts a named gesture.
   *
   * @param gesture_id Stable gesture identifier.
   * @return Operation status.
   */
  virtual common::Status startGesture(std::string_view gesture_id) = 0;

  /**
   * @brief Stops the active gesture.
   *
   * @return Operation status.
   */
  virtual common::Status stopGesture() = 0;
};

} // namespace humanoid::gesture
