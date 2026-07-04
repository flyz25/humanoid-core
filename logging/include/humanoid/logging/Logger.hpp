#pragma once

/**
 * @file Logger.hpp
 * @brief Defines the logger interfaces exposed to applications and managers.
 */

#include <humanoid/common/Status.hpp>
#include <humanoid/logging/LogLevel.hpp>
#include <humanoid/logging/LogMessage.hpp>

namespace humanoid::logging {

/**
 * @brief Abstract logger contract used by framework components.
 */
class Logger {
public:
  /**
   * @brief Destroys the logger interface.
   */
  virtual ~Logger() = default;

  /**
   * @brief Emits a structured log message.
   *
   * @param message Message to emit.
   * @return Status of the logging operation.
   */
  virtual common::Status log(const LogMessage& message) = 0;

  /**
   * @brief Reports whether a severity level is enabled.
   *
   * @param level Severity to evaluate.
   * @return True when a message at this level can be emitted.
   */
  [[nodiscard]] virtual bool isEnabled(LogLevel level) const noexcept = 0;
};

/**
 * @brief Named logger interface for dependency injection.
 */
class ILogger : public Logger {
public:
  /**
   * @brief Destroys the logger interface.
   */
  ~ILogger() override = default;
};

} // namespace humanoid::logging
