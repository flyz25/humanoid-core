#pragma once

/**
 * @file DelayStep.h
 * @brief Defines a deterministic mission delay step.
 */

#include <chrono>

namespace humanoid::mission {

/**
 * @brief Duration type used by mission delay steps.
 */
using DelayStepDuration = std::chrono::milliseconds;

/**
 * @brief Represents a fixed-duration delay in mission execution.
 *
 * A delay step contains no robot command and has no vendor dependency. The
 * mission executor may use it to pause progression between command steps while
 * still honoring cancellation and stop requests.
 */
struct DelayStep final {
  /**
   * @brief Duration to delay before the mission continues.
   *
   * Negative durations are invalid and must be rejected before execution.
   */
  DelayStepDuration duration{DelayStepDuration::zero()};

  /**
   * @brief Constructs a zero-duration delay step.
   */
  constexpr DelayStep() noexcept = default;

  /**
   * @brief Constructs a delay step with an explicit duration.
   *
   * @param duration_value Delay duration.
   */
  explicit constexpr DelayStep(DelayStepDuration duration_value) noexcept
      : duration(duration_value) {}

  /**
   * @brief Reports whether this delay step is valid.
   *
   * @return True when the delay duration is nonnegative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return duration >= DelayStepDuration::zero();
  }
};

} // namespace humanoid::mission
