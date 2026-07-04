#pragma once

/**
 * @file IPluginRegistrar.hpp
 * @brief Defines the host registration interface exposed to plugins.
 */

#include <string_view>

#include <humanoid/common/Status.hpp>
#include <humanoid/plugins/PluginLifecycleState.hpp>
#include <humanoid/plugins/PluginMetadata.hpp>

namespace humanoid::plugins {

/**
 * @brief Registration boundary provided by the host to plugin instances.
 */
class IPluginRegistrar {
public:
  /**
   * @brief Destroys the registrar interface.
   */
  virtual ~IPluginRegistrar() = default;

  /**
   * @brief Registers plugin metadata with the host.
   *
   * @param metadata Plugin metadata to register.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  RegisterPlugin(PluginMetadata metadata) = 0;

  /**
   * @brief Removes a plugin registration from the host.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  UnregisterPlugin(std::string_view plugin_id) = 0;

  /**
   * @brief Updates the host-visible lifecycle state for a registered plugin.
   *
   * @param plugin_id Stable plugin identifier.
   * @param lifecycle_state New lifecycle state.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  SetLifecycleState(std::string_view plugin_id,
                    PluginLifecycleState lifecycle_state) = 0;
};

} // namespace humanoid::plugins
