#pragma once

/**
 * @file ExecutionContextId.h
 * @brief Defines identifiers used by the vendor-independent execution runtime.
 */

#include <cstdint>

namespace humanoid::runtime {

/**
 * @brief Stable identifier assigned to one execution context.
 */
using ExecutionContextId = std::uint64_t;

/**
 * @brief Mission identifier carried without depending on the mission model.
 */
using ExecutionMissionId = std::uint64_t;

/**
 * @brief Step identifier carried without depending on a specific engine.
 */
using ExecutionStepId = std::uint64_t;

/**
 * @brief Reserved value representing an unassigned execution context ID.
 */
inline constexpr ExecutionContextId kInvalidExecutionContextId = 0U;

/**
 * @brief Reserved value representing no associated mission.
 */
inline constexpr ExecutionMissionId kInvalidExecutionMissionId = 0U;

} // namespace humanoid::runtime
