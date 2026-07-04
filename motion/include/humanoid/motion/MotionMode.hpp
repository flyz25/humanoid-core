#pragma once

/**
 * @file MotionMode.hpp
 * @brief Defines high-level motion modes.
 */

#include <string_view>

namespace humanoid::motion {

/**
 * @brief Identifies the high-level mode requested from a motion controller.
 */
enum class MotionMode { kIdle, kHoldPosition, kJointPosition, kJointVelocity, kCartesianVelocity };

/**
 * @brief Converts a motion mode to a stable string representation.
 *
 * @param mode Motion mode to convert.
 * @return Human-readable mode name.
 */
[[nodiscard]] std::string_view toString(MotionMode mode) noexcept;

} // namespace humanoid::motion
