#pragma once

/**
 * @file RetryPolicy.h
 * @brief Defines mission step retry policy.
 */

#include <chrono>
#include <cstdint>

namespace humanoid::mission {

/**
 * @brief Count type for retry attempts.
 */
using RetryAttemptCount = std::uint32_t;

/**
 * @brief Duration type used between retry attempts.
 */
using RetryDelay = std::chrono::milliseconds;

/**
 * @brief Controls repeated attempts after a failed step execution.
 *
 * `maxAttempts` is the total number of attempts, including the initial attempt.
 * A value of one means retry is disabled. The policy does not contain command
 * logic or robot-specific behavior.
 */
struct RetryPolicy final {
  /**
   * @brief Total attempts allowed for this step, including the first attempt.
   */
  RetryAttemptCount maxAttempts{1U};

  /**
   * @brief Delay inserted before each retry attempt.
   *
   * Negative values are invalid and must be rejected before execution.
   */
  RetryDelay delayBetweenAttempts{RetryDelay::zero()};

  /**
   * @brief Allows retry after a command failure.
   */
  bool retryOnFailure{true};

  /**
   * @brief Allows retry after a command timeout.
   */
  bool retryOnTimeout{true};

  /**
   * @brief Constructs a policy that performs one attempt.
   */
  constexpr RetryPolicy() noexcept = default;

  /**
   * @brief Constructs a retry policy with an explicit attempt count.
   *
   * @param attempts Total attempts allowed, including the initial attempt.
   */
  explicit constexpr RetryPolicy(RetryAttemptCount attempts) noexcept : maxAttempts(attempts) {}

  /**
   * @brief Reports whether retry behavior is enabled.
   *
   * @return True when more than one attempt is allowed.
   */
  [[nodiscard]] constexpr bool isEnabled() const noexcept { return maxAttempts > 1U; }

  /**
   * @brief Reports whether the policy is valid.
   *
   * @return True when at least one attempt is allowed and delay is nonnegative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return maxAttempts > 0U && delayBetweenAttempts >= RetryDelay::zero();
  }
};

} // namespace humanoid::mission
