#pragma once

/**
 * @file SdkConverter.h
 * @brief Defines conversions from Unitree SDK abstraction data to framework data.
 */

#include <string>
#include <string_view>

#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/adapters/Result.h>
#include <humanoid/core/RobotState.hpp>

#include "SdkTypes.h"

namespace humanoid::plugins::unitree::sdk {

/**
 * @brief Converts an SDK abstraction result to the legacy adapter result type.
 *
 * @param result SDK abstraction result.
 * @return Adapter result.
 */
[[nodiscard]] humanoid::adapters::Result ToAdapterResult(const SdkResult& result);

/**
 * @brief Converts SDK abstraction connection state to the legacy adapter connection state.
 *
 * @param state SDK abstraction connection state.
 * @return Adapter connection state.
 */
[[nodiscard]] humanoid::adapters::RobotConnectionState
ToAdapterConnectionState(SdkConnectionState state) noexcept;

/**
 * @brief Converts normalized SDK robot state into the core RobotState model.
 *
 * @param state SDK abstraction robot state.
 * @return Vendor-independent core robot state.
 */
[[nodiscard]] humanoid::core::RobotState ToCoreRobotState(const SdkRobotState& state) noexcept;

/**
 * @brief Converts an SDK abstraction error code to stable text.
 *
 * @param code SDK abstraction error code.
 * @return Stable text representation.
 */
[[nodiscard]] std::string_view toString(SdkErrorCode code) noexcept;

/**
 * @brief Converts an SDK abstraction connection state to stable text.
 *
 * @param state SDK abstraction connection state.
 * @return Stable text representation.
 */
[[nodiscard]] std::string_view toString(SdkConnectionState state) noexcept;

/**
 * @brief Converts an SDK abstraction motion mode to stable text.
 *
 * @param mode SDK abstraction motion mode.
 * @return Stable text representation.
 */
[[nodiscard]] std::string_view toString(SdkMotionMode mode) noexcept;

} // namespace humanoid::plugins::unitree::sdk
