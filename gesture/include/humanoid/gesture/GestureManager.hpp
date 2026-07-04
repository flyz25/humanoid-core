#pragma once

/**
 * @file GestureManager.hpp
 * @brief Defines the gesture manager.
 */

#include <memory>
#include <string_view>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/gesture/GestureController.hpp>

namespace humanoid::gesture {

/**
 * @brief Coordinates access to an injected gesture controller interface.
 */
class GestureManager final {
public:
  /**
   * @brief Constructs an empty gesture manager.
   */
  GestureManager() = default;

  /**
   * @brief Constructs a gesture manager with a controller.
   *
   * @param controller Gesture controller interface.
   */
  explicit GestureManager(std::shared_ptr<GestureController> controller);

  /**
   * @brief Sets the active gesture controller.
   *
   * @param controller Gesture controller interface.
   * @return Success when the controller is non-null.
   */
  common::Status setController(std::shared_ptr<GestureController> controller);

  /**
   * @brief Clears the active gesture controller.
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
   * @brief Reports whether a gesture is supported.
   *
   * @param gesture_id Stable gesture identifier.
   * @return True when the active controller supports the gesture.
   */
  [[nodiscard]] bool supportsGesture(std::string_view gesture_id) const;

  /**
   * @brief Starts a named gesture.
   *
   * @param gesture_id Stable gesture identifier.
   * @return Operation status.
   */
  common::Status startGesture(std::string_view gesture_id);

  /**
   * @brief Stops the active gesture.
   *
   * @return Operation status.
   */
  common::Status stopGesture();

private:
  std::shared_ptr<GestureController> controller_;
};

} // namespace humanoid::gesture
