#pragma once

/**
 * @file WaitStep.h
 * @brief Defines a deterministic mission wait step.
 */

#include <chrono>

namespace humanoid::mission {

/**
 * @brief Duration type used by mission wait steps.
 */
using WaitStepDuration = std::chrono::milliseconds;

/**
 * @brief Represents a fixed-duration wait in mission execution.
 *
 * The current model intentionally supports deterministic duration-based waits
 * only. Sensor or condition waits belong in a future mission capability and are
 * not encoded in this vendor-independent value type.
 */
struct WaitStep final {
  /**
   * @brief Duration to wait before the mission continues.
   *
   * Negative durations are invalid and must be rejected before execution.
   */
  WaitStepDuration duration{WaitStepDuration::zero()};

  /**
   * @brief Constructs a zero-duration wait step.
   */
  constexpr WaitStep() noexcept = default;

  /**
   * @brief Constructs a wait step with an explicit duration.
   *
   * @param duration_value Wait duration.
   */
  explicit constexpr WaitStep(WaitStepDuration duration_value) noexcept
      : duration(duration_value) {}

  /**
   * @brief Reports whether this wait step is valid.
   *
   * @return True when the wait duration is nonnegative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return duration >= WaitStepDuration::zero();
  }
};

} // namespace humanoid::mission
