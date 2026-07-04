#pragma once

/**
 * @file PluginFactory.hpp
 * @brief Defines a thread-safe factory for registered plugin creators.
 */

#include <cstddef>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/plugins/IPlugin.hpp>
#include <humanoid/plugins/PluginMetadata.hpp>
#include <humanoid/plugins/PluginRegistry.hpp>

namespace humanoid::plugins {

/**
 * @brief Result returned by plugin creation operations.
 */
struct PluginCreationResult final {
  /**
   * @brief Operation status.
   */
  humanoid::common::Status status{};

  /**
   * @brief Created plugin instance when status is successful.
   */
  std::unique_ptr<IPlugin> plugin{};
};

/**
 * @brief Thread-safe factory for plugin instance creation and destruction.
 *
 * PluginFactory stores creator functions and delegates metadata and lifecycle
 * visibility to an injected `PluginRegistry`. It owns no global state and does
 * not perform dynamic shared-library loading.
 */
class PluginFactory final {
public:
  /**
   * @brief Callable used to create a plugin instance.
   */
  using PluginCreator = std::function<std::unique_ptr<IPlugin>()>;

  /**
   * @brief Constructs a factory with an internally owned registry.
   */
  PluginFactory();

  /**
   * @brief Constructs a factory using an injected registry.
   *
   * @param registry Registry used for metadata and lifecycle visibility.
   */
  explicit PluginFactory(std::shared_ptr<PluginRegistry> registry) noexcept;

  /**
   * @brief Destroys the plugin factory.
   */
  ~PluginFactory() = default;

  PluginFactory(const PluginFactory&) = delete;
  PluginFactory& operator=(const PluginFactory&) = delete;
  PluginFactory(PluginFactory&&) = delete;
  PluginFactory& operator=(PluginFactory&&) = delete;

  /**
   * @brief Registers metadata and a creator for a plugin type.
   *
   * @param metadata Plugin metadata.
   * @param creator Callable that returns a new plugin instance.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterPlugin(PluginMetadata metadata,
                                                        PluginCreator creator);

  /**
   * @brief Unregisters a plugin type.
   *
   * Unregistration fails while instances created by this factory are still
   * active.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterPlugin(std::string_view plugin_id);

  /**
   * @brief Creates a plugin instance.
   *
   * @param plugin_id Stable plugin identifier.
   * @return Creation result containing status and optional plugin instance.
   */
  [[nodiscard]] PluginCreationResult CreatePlugin(std::string_view plugin_id);

  /**
   * @brief Destroys a plugin instance created by this factory.
   *
   * The plugin is shut down before ownership is released. If shutdown fails, the
   * plugin remains owned by the caller.
   *
   * @param plugin Plugin instance to destroy.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status DestroyPlugin(std::unique_ptr<IPlugin>& plugin);

  /**
   * @brief Enumerates registered plugin metadata and lifecycle state.
   *
   * @return Snapshot of registered plugin records.
   */
  [[nodiscard]] std::vector<PluginRecord> EnumeratePlugins() const;

  /**
   * @brief Returns the number of registered plugin creators.
   *
   * @return Registered creator count.
   */
  [[nodiscard]] std::size_t RegisteredPluginCount() const;

  /**
   * @brief Returns the injected registry used by this factory.
   *
   * @return Shared plugin registry.
   */
  [[nodiscard]] std::shared_ptr<PluginRegistry> Registry() const noexcept;

private:
  struct FactoryEntry final {
    PluginMetadata metadata{};
    PluginCreator creator{};
    std::size_t active_instances{0};
  };

  /**
   * @brief Converts a plugin identifier into a map key.
   *
   * @param plugin_id Stable plugin identifier.
   * @return String key.
   */
  [[nodiscard]] static std::string MakeKey(std::string_view plugin_id);

  std::shared_ptr<PluginRegistry> registry_;
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, FactoryEntry> factories_;
};

} // namespace humanoid::plugins
