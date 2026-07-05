#pragma once

/**
 * @file LoopPolicy.h
 * @brief Defines mission step loop policy.
 */

#include <cstdint>

namespace humanoid::mission {

/**
 * @brief Count type for mission loop iterations.
 */
using LoopIterationCount = std::uint32_t;

/**
 * @brief Controls repeated execution of one mission step.
 *
 * The policy is local to a step and remains vendor independent. A value of one
 * means the step executes once. Values greater than one repeat the same step.
 */
struct LoopPolicy final {
  /**
   * @brief Total number of step executions requested.
   *
   * A value of zero is invalid because it makes step behavior ambiguous. Use
   * `MissionStep::skip` when a step should be bypassed intentionally.
   */
  LoopIterationCount iterations{1U};

  /**
   * @brief Constructs a single-iteration loop policy.
   */
  constexpr LoopPolicy() noexcept = default;

  /**
   * @brief Constructs a loop policy with an explicit iteration count.
   *
   * @param iteration_count Total number of executions requested.
   */
  explicit constexpr LoopPolicy(LoopIterationCount iteration_count) noexcept
      : iterations(iteration_count) {}

  /**
   * @brief Reports whether looping is requested.
   *
   * @return True when the step should run more than once.
   */
  [[nodiscard]] constexpr bool isEnabled() const noexcept { return iterations > 1U; }

  /**
   * @brief Reports whether the policy is valid.
   *
   * @return True when at least one iteration is requested.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept { return iterations > 0U; }
};

} // namespace humanoid::mission
