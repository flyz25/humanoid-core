#pragma once

/**
 * @file ConfigManager.hpp
 * @brief Defines the configuration manager.
 */

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

#include <humanoid/common/Status.hpp>
#include <humanoid/configuration/ConfigValue.hpp>
#include <humanoid/configuration/Configuration.hpp>

namespace humanoid::configuration {

/**
 * @brief Owns the active configuration provider interface for the framework.
 */
class ConfigManager final {
public:
  /**
   * @brief Constructs an empty configuration manager.
   */
  ConfigManager() = default;

  /**
   * @brief Constructs a configuration manager with a provider.
   *
   * @param configuration Configuration provider interface.
   */
  explicit ConfigManager(std::shared_ptr<const Configuration> configuration);

  /**
   * @brief Sets the active configuration provider.
   *
   * @param configuration Configuration provider interface.
   * @return Success when the provider is non-null.
   */
  common::Status setConfiguration(std::shared_ptr<const Configuration> configuration);

  /**
   * @brief Clears the active configuration provider.
   */
  void clear() noexcept;

  /**
   * @brief Reports whether a provider is configured.
   *
   * @return True when a provider is attached.
   */
  [[nodiscard]] bool hasConfiguration() const noexcept;

  /**
   * @brief Reports whether a configuration key exists.
   *
   * @param key Configuration key.
   * @return True when the active provider contains the key.
   */
  [[nodiscard]] bool contains(std::string_view key) const;

  /**
   * @brief Returns a configuration value from the active provider.
   *
   * @param key Configuration key.
   * @return Value when a provider is configured and contains the key.
   */
  [[nodiscard]] std::optional<ConfigValue> value(std::string_view key) const;

  /**
   * @brief Returns the active provider source path.
   *
   * @return Source path when a provider is configured.
   */
  [[nodiscard]] std::optional<std::filesystem::path> sourcePath() const;

private:
  std::shared_ptr<const Configuration> configuration_;
};

} // namespace humanoid::configuration
