#pragma once

/**
 * @file DiagnosticStatus.hpp
 * @brief Defines diagnostic status values.
 */

#include <string_view>

namespace humanoid::diagnostics {

/**
 * @brief Represents the outcome of a diagnostic check.
 */
enum class DiagnosticStatus { kOk, kWarning, kError, kStale };

/**
 * @brief Converts a diagnostic status to a stable string representation.
 *
 * @param status Diagnostic status to convert.
 * @return Human-readable diagnostic status name.
 */
[[nodiscard]] std::string_view toString(DiagnosticStatus status) noexcept;

} // namespace humanoid::diagnostics
