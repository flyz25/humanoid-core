#pragma once

/**
 * @file SafetyManager.hpp
 * @brief Defines the safety manager.
 */

#include <memory>
#include <string_view>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/safety/SafetyController.hpp>
#include <humanoid/safety/SafetyState.hpp>

namespace humanoid::safety {

/**
 * @brief Coordinates access to an injected safety controller interface.
 */
class SafetyManager final {
public:
  /**
   * @brief Constructs an empty safety manager.
   */
  SafetyManager() = default;

  /**
   * @brief Constructs a safety manager with a controller.
   *
   * @param controller Safety controller interface.
   */
  explicit SafetyManager(std::shared_ptr<SafetyController> controller);

  /**
   * @brief Sets the active safety controller.
   *
   * @param controller Safety controller interface.
   * @return Success when the controller is non-null.
   */
  common::Status setController(std::shared_ptr<SafetyController> controller);

  /**
   * @brief Clears the active safety controller.
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
   * @brief Returns the active safety state.
   *
   * @return Safety state, or unknown when no controller is attached.
   */
  [[nodiscard]] SafetyState safetyState() const noexcept;

  /**
   * @brief Reports whether motion is permitted.
   *
   * @return True when the active controller permits motion.
   */
  [[nodiscard]] bool isMotionAllowed() const noexcept;

  /**
   * @brief Engages a safety stop through the active controller.
   *
   * @param reason Human-readable reason for the stop request.
   * @return Operation status.
   */
  common::Status engageStop(std::string_view reason);

  /**
   * @brief Releases a safety stop through the active controller.
   *
   * @return Operation status.
   */
  common::Status releaseStop();

private:
  std::shared_ptr<SafetyController> controller_;
};

} // namespace humanoid::safety
