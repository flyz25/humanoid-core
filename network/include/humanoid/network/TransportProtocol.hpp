#pragma once

/**
 * @file TransportProtocol.hpp
 * @brief Defines supported transport protocol categories.
 */

#include <string_view>

namespace humanoid::network {

/**
 * @brief Identifies the network transport category.
 */
enum class TransportProtocol { kTcp, kUdp, kDds };

/**
 * @brief Converts a transport protocol to a stable string representation.
 *
 * @param protocol Protocol to convert.
 * @return Human-readable protocol name.
 */
[[nodiscard]] std::string_view toString(TransportProtocol protocol) noexcept;

} // namespace humanoid::network
