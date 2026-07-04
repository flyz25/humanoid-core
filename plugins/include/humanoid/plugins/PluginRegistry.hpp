#pragma once

/**
 * @file PluginRegistry.hpp
 * @brief Defines the thread-safe plugin metadata registry.
 */

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <humanoid/plugins/IPluginRegistrar.hpp>
#include <humanoid/plugins/PluginLifecycleState.hpp>
#include <humanoid/plugins/PluginMetadata.hpp>

namespace humanoid::plugins {

/**
 * @brief Host-visible record for a registered plugin.
 */
struct PluginRecord final {
  /**
   * @brief Registered plugin metadata.
   */
  PluginMetadata metadata{};

  /**
   * @brief Current host-visible lifecycle state.
   */
  PluginLifecycleState lifecycle_state{PluginLifecycleState::kRegistered};
};

/**
 * @brief Thread-safe registry for plugin metadata and lifecycle state.
 *
 * PluginRegistry owns no plugin implementation and performs no dynamic loading.
 * It stores registration metadata and lifecycle state supplied by plugin
 * infrastructure code.
 */
class PluginRegistry final : public IPluginRegistrar {
public:
  /**
   * @brief Constructs an empty plugin registry.
   */
  PluginRegistry() = default;

  /**
   * @brief Destroys the plugin registry.
   */
  ~PluginRegistry() override = default;

  PluginRegistry(const PluginRegistry&) = delete;
  PluginRegistry& operator=(const PluginRegistry&) = delete;
  PluginRegistry(PluginRegistry&&) = delete;
  PluginRegistry& operator=(PluginRegistry&&) = delete;

  /**
   * @brief Registers plugin metadata after validation and compatibility checks.
   *
   * @param metadata Plugin metadata to register.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status
  RegisterPlugin(PluginMetadata metadata) override;

  /**
   * @brief Removes a plugin registration.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status
  UnregisterPlugin(std::string_view plugin_id) override;

  /**
   * @brief Updates lifecycle state for a registered plugin.
   *
   * @param plugin_id Stable plugin identifier.
   * @param lifecycle_state New lifecycle state.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status
  SetLifecycleState(std::string_view plugin_id,
                    PluginLifecycleState lifecycle_state) override;

  /**
   * @brief Reports whether a plugin is registered.
   *
   * @param plugin_id Stable plugin identifier.
   * @return True when the plugin is registered.
   */
  [[nodiscard]] bool Contains(std::string_view plugin_id) const;

  /**
   * @brief Returns registered metadata for a plugin.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Plugin metadata when registered.
   */
  [[nodiscard]] std::optional<PluginMetadata>
  Metadata(std::string_view plugin_id) const;

  /**
   * @brief Returns lifecycle state for a plugin.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Lifecycle state when registered.
   */
  [[nodiscard]] std::optional<PluginLifecycleState>
  LifecycleState(std::string_view plugin_id) const;

  /**
   * @brief Returns all registered plugin records.
   *
   * @return Snapshot of all registered plugin records.
   */
  [[nodiscard]] std::vector<PluginRecord> Plugins() const;

  /**
   * @brief Returns the number of registered plugins.
   *
   * @return Registered plugin count.
   */
  [[nodiscard]] std::size_t PluginCount() const;

private:
  /**
   * @brief Converts a string view plugin identifier into a map key.
   *
   * @param plugin_id Stable plugin identifier.
   * @return String key.
   */
  [[nodiscard]] static std::string MakeKey(std::string_view plugin_id);

  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, PluginRecord> plugins_;
};

} // namespace humanoid::plugins
