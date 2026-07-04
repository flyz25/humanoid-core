#pragma once

/**
 * @file LoggerManager.hpp
 * @brief Defines the logging manager that routes messages to sink interfaces.
 */

#include <cstddef>
#include <memory>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/logging/LogLevel.hpp>
#include <humanoid/logging/LogSink.hpp>
#include <humanoid/logging/Logger.hpp>

namespace humanoid::logging {

/**
 * @brief Manages log sink interfaces and implements the logger contract.
 *
 * LoggerManager is infrastructure, not a backend. It owns no console, file, or
 * network logging implementation.
 */
class LoggerManager final : public ILogger {
public:
  /**
   * @brief Constructs a logger manager.
   *
   * @param minimum_level Lowest severity accepted by the manager.
   */
  explicit LoggerManager(LogLevel minimum_level = LogLevel::kInfo);

  /**
   * @brief Registers a log sink.
   *
   * @param sink Sink interface to register.
   * @return Success when the sink is non-null and registered.
   */
  common::Status addSink(std::shared_ptr<LogSink> sink);

  /**
   * @brief Removes every registered sink.
   */
  void clearSinks() noexcept;

  /**
   * @brief Returns the number of registered sinks.
   *
   * @return Sink count.
   */
  [[nodiscard]] std::size_t sinkCount() const noexcept;

  /**
   * @brief Updates the minimum accepted severity.
   *
   * @param level New minimum severity.
   */
  void setMinimumLevel(LogLevel level) noexcept;

  /**
   * @brief Returns the minimum accepted severity.
   *
   * @return Minimum severity.
   */
  [[nodiscard]] LogLevel minimumLevel() const noexcept;

  /**
   * @brief Emits a structured log message to matching sinks.
   *
   * @param message Message to emit.
   * @return Success when all matching sinks accept the message successfully.
   */
  common::Status log(const LogMessage& message) override;

  /**
   * @brief Reports whether the manager can emit a severity level.
   *
   * @param level Severity to evaluate.
   * @return True when at least one sink accepts the level and the level passes manager filtering.
   */
  [[nodiscard]] bool isEnabled(LogLevel level) const noexcept override;

private:
  /**
   * @brief Reports whether a level passes the manager severity filter.
   *
   * @param level Severity to evaluate.
   * @return True when the level is at least the configured minimum.
   */
  [[nodiscard]] bool passesMinimumLevel(LogLevel level) const noexcept;

  std::vector<std::shared_ptr<LogSink>> sinks_;
  LogLevel minimum_level_;
};

} // namespace humanoid::logging
