#pragma once

/**
 * @file IPlugin.hpp
 * @brief Defines the base lifecycle interface implemented by plugins.
 */

#include <humanoid/common/Status.hpp>
#include <humanoid/plugins/IPluginRegistrar.hpp>
#include <humanoid/plugins/PluginMetadata.hpp>

namespace humanoid::plugins {

/**
 * @brief Base interface for all humanoid-core plugins.
 *
 * Plugins expose metadata and lifecycle operations only. Concrete plugin types
 * must keep vendor SDKs, middleware, and implementation-specific types behind
 * plugin-owned boundaries.
 */
class IPlugin {
public:
  /**
   * @brief Destroys the plugin interface.
   */
  virtual ~IPlugin() = default;

  /**
   * @brief Returns immutable plugin metadata.
   *
   * @return Plugin metadata owned by the plugin instance.
   */
  [[nodiscard]] virtual const PluginMetadata& Metadata() const noexcept = 0;

  /**
   * @brief Initializes the plugin and registers it with the host.
   *
   * @param registrar Host registrar used by the plugin for registration.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status
  Initialize(IPluginRegistrar& registrar) = 0;

  /**
   * @brief Starts plugin runtime operation after successful initialization.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Start() = 0;

  /**
   * @brief Stops plugin runtime operation while preserving initialized state.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Stop() = 0;

  /**
   * @brief Releases plugin resources before unloading.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Shutdown() = 0;
};

} // namespace humanoid::plugins
