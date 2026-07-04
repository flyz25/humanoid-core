#pragma once

/**
 * @file LogSink.hpp
 * @brief Defines the logging sink interface.
 */

#include <humanoid/common/Status.hpp>
#include <humanoid/logging/LogLevel.hpp>
#include <humanoid/logging/LogMessage.hpp>

namespace humanoid::logging {

/**
 * @brief Abstract destination for log messages.
 *
 * Implementations may write to console, files, DDS topics, or remote services.
 * This interface intentionally does not prescribe a backend.
 */
class LogSink {
public:
  /**
   * @brief Destroys the sink interface.
   */
  virtual ~LogSink() = default;

  /**
   * @brief Writes a log message.
   *
   * @param message Structured log message.
   * @return Status of the write attempt.
   */
  virtual common::Status write(const LogMessage& message) = 0;

  /**
   * @brief Reports whether the sink accepts a severity level.
   *
   * @param level Severity to evaluate.
   * @return True when the sink accepts the level.
   */
  [[nodiscard]] virtual bool accepts(LogLevel level) const noexcept = 0;
};

} // namespace humanoid::logging
