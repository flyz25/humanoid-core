#pragma once

/**
 * @file TimeoutPolicy.h
 * @brief Defines mission step timeout policy.
 */

#include <chrono>

namespace humanoid::mission {

/**
 * @brief Duration type used by mission timeout policy.
 */
using TimeoutDuration = std::chrono::milliseconds;

/**
 * @brief Controls timeout behavior for a mission step.
 *
 * A zero timeout disables this policy. Negative values are invalid. The
 * executor combines this policy with the legacy per-step timeout by applying
 * the shortest enabled timeout.
 */
struct TimeoutPolicy final {
  /**
   * @brief Maximum duration allowed for the step; zero disables this policy.
   */
  TimeoutDuration timeout{TimeoutDuration::zero()};

  /**
   * @brief Requests mission abort when this timeout expires.
   */
  bool abortOnTimeout{true};

  /**
   * @brief Constructs a disabled timeout policy.
   */
  constexpr TimeoutPolicy() noexcept = default;

  /**
   * @brief Constructs a timeout policy with an explicit timeout duration.
   *
   * @param timeout_value Maximum step duration.
   */
  explicit constexpr TimeoutPolicy(TimeoutDuration timeout_value) noexcept
      : timeout(timeout_value) {}

  /**
   * @brief Reports whether timeout enforcement is requested.
   *
   * @return True when the timeout duration is greater than zero.
   */
  [[nodiscard]] constexpr bool isEnabled() const noexcept {
    return timeout > TimeoutDuration::zero();
  }

  /**
   * @brief Reports whether the policy is valid.
   *
   * @return True when timeout is nonnegative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return timeout >= TimeoutDuration::zero();
  }
};

} // namespace humanoid::mission
