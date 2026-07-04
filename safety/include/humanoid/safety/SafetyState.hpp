#pragma once

/**
 * @file SafetyState.hpp
 * @brief Defines safety state values.
 */

#include <string_view>

namespace humanoid::safety {

/**
 * @brief Represents the safety state reported by a safety controller.
 */
enum class SafetyState { kUnknown, kNominal, kProtectiveStop, kEmergencyStop, kFaulted };

/**
 * @brief Converts a safety state to a stable string representation.
 *
 * @param state Safety state to convert.
 * @return Human-readable safety state name.
 */
[[nodiscard]] std::string_view toString(SafetyState state) noexcept;

} // namespace humanoid::safety
