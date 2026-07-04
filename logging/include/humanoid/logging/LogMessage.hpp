#pragma once

/**
 * @file LogMessage.hpp
 * @brief Defines the immutable log message value type.
 */

#include <chrono>
#include <string>

#include <humanoid/logging/LogLevel.hpp>

namespace humanoid::logging {

/**
 * @brief Carries structured log data from framework code to a log sink.
 */
class LogMessage final {
public:
  /**
   * @brief Constructs a log message.
   *
   * @param level Message severity.
   * @param component Component that emitted the message.
   * @param text Human-readable log text.
   * @param timestamp System-clock timestamp for the message.
   */
  LogMessage(LogLevel level, std::string component, std::string text,
             std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

  /**
   * @brief Returns the message severity.
   *
   * @return Log level.
   */
  [[nodiscard]] LogLevel level() const noexcept;

  /**
   * @brief Returns the emitting component name.
   *
   * @return Component name.
   */
  [[nodiscard]] const std::string& component() const noexcept;

  /**
   * @brief Returns the message text.
   *
   * @return Log text.
   */
  [[nodiscard]] const std::string& text() const noexcept;

  /**
   * @brief Returns the message timestamp.
   *
   * @return System-clock timestamp.
   */
  [[nodiscard]] std::chrono::system_clock::time_point timestamp() const noexcept;

private:
  LogLevel level_;
  std::string component_;
  std::string text_;
  std::chrono::system_clock::time_point timestamp_;
};

} // namespace humanoid::logging
