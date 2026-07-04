#pragma once

/**
 * @file SafetyController.hpp
 * @brief Defines the abstract safety controller interface.
 */

#include <string_view>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/safety/SafetyState.hpp>

namespace humanoid::safety {

/**
 * @brief Abstract boundary for safety control.
 */
class SafetyController {
public:
  /**
   * @brief Destroys the safety controller interface.
   */
  virtual ~SafetyController() = default;

  /**
   * @brief Returns the controller lifecycle state.
   *
   * @return Controller lifecycle state.
   */
  [[nodiscard]] virtual common::LifecycleState lifecycleState() const noexcept = 0;

  /**
   * @brief Returns the active safety state.
   *
   * @return Safety state.
   */
  [[nodiscard]] virtual SafetyState safetyState() const noexcept = 0;

  /**
   * @brief Reports whether robot motion is currently permitted.
   *
   * @return True when motion is allowed.
   */
  [[nodiscard]] virtual bool isMotionAllowed() const noexcept = 0;

  /**
   * @brief Engages a safety stop.
   *
   * @param reason Human-readable reason for the stop request.
   * @return Operation status.
   */
  virtual common::Status engageStop(std::string_view reason) = 0;

  /**
   * @brief Releases a previously engaged safety stop.
   *
   * @return Operation status.
   */
  virtual common::Status releaseStop() = 0;
};

} // namespace humanoid::safety
