#pragma once

/**
 * @file LifecycleState.hpp
 * @brief Defines lifecycle states shared by framework interfaces.
 */

#include <string_view>

namespace humanoid::common {

/**
 * @brief Represents the runtime lifecycle state of a framework component.
 */
enum class LifecycleState { kUnconfigured, kConfigured, kInactive, kActive, kFaulted };

/**
 * @brief Converts a lifecycle state to a stable string representation.
 *
 * @param state Lifecycle state to convert.
 * @return Human-readable state name.
 */
[[nodiscard]] std::string_view toString(LifecycleState state) noexcept;

} // namespace humanoid::common
