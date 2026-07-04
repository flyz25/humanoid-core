#pragma once

/**
 * @file PluginLifecycleState.hpp
 * @brief Defines lifecycle states for humanoid-core plugins.
 */

#include <string_view>

namespace humanoid::plugins {

/**
 * @brief Represents the host-visible lifecycle state of a plugin.
 */
enum class PluginLifecycleState {
  /**
   * @brief Plugin metadata has been discovered but the plugin is not registered.
   */
  kDiscovered,

  /**
   * @brief Plugin metadata has been registered with the host.
   */
  kRegistered,

  /**
   * @brief Plugin binary has been loaded by a plugin loader.
   */
  kLoaded,

  /**
   * @brief Plugin instance has completed initialization.
   */
  kInitialized,

  /**
   * @brief Plugin has started runtime operation.
   */
  kStarted,

  /**
   * @brief Plugin has stopped runtime operation.
   */
  kStopped,

  /**
   * @brief Plugin has completed shutdown and released runtime resources.
   */
  kShutdown,

  /**
   * @brief Plugin lifecycle operation failed.
   */
  kFailed
};

/**
 * @brief Converts a plugin lifecycle state to a stable string.
 *
 * @param state Lifecycle state to convert.
 * @return Stable string representation.
 */
[[nodiscard]] std::string_view toString(PluginLifecycleState state) noexcept;

} // namespace humanoid::plugins
