#pragma once

/**
 * @file PluginMetadata.hpp
 * @brief Defines descriptive and compatibility metadata for plugins.
 */

#include <filesystem>
#include <string>

#include <humanoid/common/Version.hpp>
#include <humanoid/plugins/PluginVersionCompatibility.hpp>

namespace humanoid::plugins {

/**
 * @brief Describes a plugin before and after registration with the host.
 */
struct PluginMetadata final {
  /**
   * @brief Stable plugin identifier, for example "org.example.simulator".
   */
  std::string plugin_id;

  /**
   * @brief Human-readable plugin name.
   */
  std::string name;

  /**
   * @brief Organization or vendor responsible for the plugin.
   */
  std::string vendor;

  /**
   * @brief Human-readable plugin purpose.
   */
  std::string description;

  /**
   * @brief Semantic version of the plugin package.
   */
  humanoid::common::SemanticVersion version{0, 1, 0};

  /**
   * @brief Inclusive framework API compatibility range declared by the plugin.
   */
  PluginVersionCompatibility compatibility{};

  /**
   * @brief Optional manifest path used by future dynamic plugin loaders.
   */
  std::filesystem::path manifest_path{};

  /**
   * @brief Reports whether required metadata fields are present and consistent.
   *
   * @return True when the metadata can be registered.
   */
  [[nodiscard]] bool IsValid() const noexcept;

  /**
   * @brief Reports whether this plugin supports a framework API version.
   *
   * @param framework_version Framework API version to check.
   * @return True when the framework API version is compatible.
   */
  [[nodiscard]] bool
  IsCompatibleWith(humanoid::common::SemanticVersion framework_version) const noexcept;
};

} // namespace humanoid::plugins
