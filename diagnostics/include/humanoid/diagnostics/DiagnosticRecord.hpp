#pragma once

/**
 * @file DiagnosticRecord.hpp
 * @brief Defines structured diagnostic records.
 */

#include <chrono>
#include <string>

#include <humanoid/diagnostics/DiagnosticStatus.hpp>

namespace humanoid::diagnostics {

/**
 * @brief Represents one diagnostic observation.
 */
class DiagnosticRecord final {
public:
  /**
   * @brief Constructs a diagnostic record.
   *
   * @param name Stable diagnostic name.
   * @param status Diagnostic status.
   * @param message Human-readable diagnostic message.
   * @param timestamp System-clock timestamp for the observation.
   */
  DiagnosticRecord(
      std::string name, DiagnosticStatus status, std::string message,
      std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

  /**
   * @brief Returns the diagnostic name.
   *
   * @return Diagnostic name.
   */
  [[nodiscard]] const std::string& name() const noexcept;

  /**
   * @brief Returns the diagnostic status.
   *
   * @return Diagnostic status.
   */
  [[nodiscard]] DiagnosticStatus status() const noexcept;

  /**
   * @brief Returns the diagnostic message.
   *
   * @return Diagnostic message.
   */
  [[nodiscard]] const std::string& message() const noexcept;

  /**
   * @brief Returns the observation timestamp.
   *
   * @return System-clock timestamp.
   */
  [[nodiscard]] std::chrono::system_clock::time_point timestamp() const noexcept;

private:
  std::string name_;
  DiagnosticStatus status_;
  std::string message_;
  std::chrono::system_clock::time_point timestamp_;
};

} // namespace humanoid::diagnostics
