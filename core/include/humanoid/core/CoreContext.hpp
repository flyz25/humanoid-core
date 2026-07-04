#pragma once

/**
 * @file CoreContext.hpp
 * @brief Defines the application-facing core context.
 */

#include <memory>

#include <humanoid/configuration/Configuration.hpp>
#include <humanoid/logging/Logger.hpp>

namespace humanoid::core {

/**
 * @brief Holds interface dependencies shared by application-level code.
 */
class CoreContext final {
public:
  /**
   * @brief Constructs an empty core context.
   */
  CoreContext() = default;

  /**
   * @brief Constructs a core context from injected interfaces.
   *
   * @param logger Logger interface.
   * @param configuration Configuration provider interface.
   */
  CoreContext(std::shared_ptr<logging::ILogger> logger,
              std::shared_ptr<const configuration::Configuration> configuration);

  /**
   * @brief Sets the logger interface.
   *
   * @param logger Logger interface.
   */
  void setLogger(std::shared_ptr<logging::ILogger> logger) noexcept;

  /**
   * @brief Sets the configuration interface.
   *
   * @param configuration Configuration provider interface.
   */
  void setConfiguration(std::shared_ptr<const configuration::Configuration> configuration) noexcept;

  /**
   * @brief Reports whether a logger interface is available.
   *
   * @return True when a logger is set.
   */
  [[nodiscard]] bool hasLogger() const noexcept;

  /**
   * @brief Reports whether a configuration interface is available.
   *
   * @return True when a configuration provider is set.
   */
  [[nodiscard]] bool hasConfiguration() const noexcept;

  /**
   * @brief Returns the logger interface.
   *
   * @return Shared logger interface.
   */
  [[nodiscard]] std::shared_ptr<logging::ILogger> logger() const noexcept;

  /**
   * @brief Returns the configuration provider interface.
   *
   * @return Shared configuration provider interface.
   */
  [[nodiscard]] std::shared_ptr<const configuration::Configuration> configuration() const noexcept;

private:
  std::shared_ptr<logging::ILogger> logger_;
  std::shared_ptr<const configuration::Configuration> configuration_;
};

} // namespace humanoid::core
