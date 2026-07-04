#pragma once

/**
 * @file LogLevel.hpp
 * @brief Defines logging severity levels.
 */

#include <string_view>

namespace humanoid::logging {

/**
 * @brief Represents the severity of a log message.
 */
enum class LogLevel { kTrace, kDebug, kInfo, kWarning, kError, kCritical };

/**
 * @brief Converts a log level to a stable string representation.
 *
 * @param level Log level to convert.
 * @return Human-readable severity name.
 */
[[nodiscard]] std::string_view toString(LogLevel level) noexcept;

} // namespace humanoid::logging
