#pragma once

/**
 * @file Time.hpp
 * @brief Defines clock helper functions for framework timing.
 */

#include <chrono>

namespace humanoid::utilities {

/**
 * @brief Alias for the monotonic clock used by framework infrastructure.
 */
using SteadyClock = std::chrono::steady_clock;

/**
 * @brief Alias for monotonic timestamps.
 */
using SteadyTimePoint = std::chrono::time_point<SteadyClock>;

/**
 * @brief Returns the current monotonic time.
 *
 * @return Current steady-clock timestamp.
 */
[[nodiscard]] SteadyTimePoint now() noexcept;

/**
 * @brief Computes elapsed time since a monotonic timestamp.
 *
 * @param start Start timestamp.
 * @return Duration between start and the current monotonic time.
 */
[[nodiscard]] std::chrono::nanoseconds elapsedSince(SteadyTimePoint start) noexcept;

} // namespace humanoid::utilities
