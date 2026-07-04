#pragma once

/**
 * @file IPluginLoader.hpp
 * @brief Defines the host-side plugin loading abstraction.
 */

#include <filesystem>
#include <string_view>

#include <humanoid/common/Status.hpp>
#include <humanoid/plugins/IPluginRegistrar.hpp>

namespace humanoid::plugins {

/**
 * @brief Interface for future platform-specific plugin loaders.
 *
 * The interface defines the host contract for dynamic plugin loading without
 * implementing platform-specific shared-library behavior in Milestone 4.1.
 */
class IPluginLoader {
public:
  /**
   * @brief Destroys the plugin loader interface.
   */
  virtual ~IPluginLoader() = default;

  /**
   * @brief Loads a plugin package or shared library.
   *
   * @param plugin_path Path to a plugin package, manifest, or shared library.
   * @param registrar Host registrar used during plugin initialization.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  Load(const std::filesystem::path& plugin_path, IPluginRegistrar& registrar) = 0;

  /**
   * @brief Unloads a previously loaded plugin.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  Unload(std::string_view plugin_id) = 0;
};

} // namespace humanoid::plugins
