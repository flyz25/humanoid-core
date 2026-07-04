#pragma once

/**
 * @file Configuration.hpp
 * @brief Defines the read-only configuration interface.
 */

#include <filesystem>
#include <optional>
#include <string_view>

#include <humanoid/configuration/ConfigValue.hpp>

namespace humanoid::configuration {

/**
 * @brief Abstract read-only configuration provider.
 */
class Configuration {
public:
  /**
   * @brief Destroys the configuration interface.
   */
  virtual ~Configuration() = default;

  /**
   * @brief Reports whether a key exists.
   *
   * @param key Configuration key.
   * @return True when the provider contains the key.
   */
  [[nodiscard]] virtual bool contains(std::string_view key) const = 0;

  /**
   * @brief Returns a configuration value.
   *
   * @param key Configuration key.
   * @return Value when present.
   */
  [[nodiscard]] virtual std::optional<ConfigValue> value(std::string_view key) const = 0;

  /**
   * @brief Returns the backing source path.
   *
   * @return Source path for the provider.
   */
  [[nodiscard]] virtual std::filesystem::path sourcePath() const = 0;
};

} // namespace humanoid::configuration
