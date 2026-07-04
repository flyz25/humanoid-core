#include <humanoid/plugins/PluginRegistry.hpp>

#include <mutex>
#include <utility>

#include <humanoid/common/Status.hpp>
#include <humanoid/common/Version.hpp>

namespace humanoid::plugins {

namespace {

[[nodiscard]] humanoid::common::Status InvalidArgument(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(
      humanoid::common::StatusCode::kFailedPrecondition, std::move(message));
}

} // namespace

humanoid::common::Status PluginRegistry::RegisterPlugin(PluginMetadata metadata) {
  if (!metadata.IsValid()) {
    return InvalidArgument("plugin metadata is incomplete or has invalid compatibility range");
  }

  if (!metadata.IsCompatibleWith(humanoid::common::apiVersion())) {
    return FailedPrecondition("plugin is not compatible with this framework API version");
  }

  const std::string plugin_id = metadata.plugin_id;
  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto [unused_iterator, inserted] = plugins_.emplace(
      plugin_id,
      PluginRecord{std::move(metadata), PluginLifecycleState::kRegistered});
  (void)unused_iterator;

  if (!inserted) {
    return FailedPrecondition("plugin is already registered");
  }

  return humanoid::common::Status::ok();
}

humanoid::common::Status PluginRegistry::UnregisterPlugin(std::string_view plugin_id) {
  if (plugin_id.empty()) {
    return InvalidArgument("plugin identifier is empty");
  }

  std::unique_lock<std::shared_mutex> lock{mutex_};
  if (plugins_.erase(MakeKey(plugin_id)) == 0U) {
    return FailedPrecondition("plugin is not registered");
  }

  return humanoid::common::Status::ok();
}

humanoid::common::Status
PluginRegistry::SetLifecycleState(std::string_view plugin_id,
                                  PluginLifecycleState lifecycle_state) {
  if (plugin_id.empty()) {
    return InvalidArgument("plugin identifier is empty");
  }

  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = plugins_.find(MakeKey(plugin_id));
  if (iterator == plugins_.end()) {
    return FailedPrecondition("plugin is not registered");
  }

  iterator->second.lifecycle_state = lifecycle_state;
  return humanoid::common::Status::ok();
}

bool PluginRegistry::Contains(std::string_view plugin_id) const {
  if (plugin_id.empty()) {
    return false;
  }

  std::shared_lock<std::shared_mutex> lock{mutex_};
  return plugins_.find(MakeKey(plugin_id)) != plugins_.end();
}

std::optional<PluginMetadata>
PluginRegistry::Metadata(std::string_view plugin_id) const {
  if (plugin_id.empty()) {
    return std::nullopt;
  }

  std::shared_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = plugins_.find(MakeKey(plugin_id));
  if (iterator == plugins_.end()) {
    return std::nullopt;
  }

  return iterator->second.metadata;
}

std::optional<PluginLifecycleState>
PluginRegistry::LifecycleState(std::string_view plugin_id) const {
  if (plugin_id.empty()) {
    return std::nullopt;
  }

  std::shared_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = plugins_.find(MakeKey(plugin_id));
  if (iterator == plugins_.end()) {
    return std::nullopt;
  }

  return iterator->second.lifecycle_state;
}

std::vector<PluginRecord> PluginRegistry::Plugins() const {
  return EnumeratePlugins();
}

std::vector<PluginRecord> PluginRegistry::EnumeratePlugins() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  std::vector<PluginRecord> records;
  records.reserve(plugins_.size());

  for (const auto& [plugin_id, record] : plugins_) {
    (void)plugin_id;
    records.push_back(record);
  }

  return records;
}

std::size_t PluginRegistry::PluginCount() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return plugins_.size();
}

std::string PluginRegistry::MakeKey(std::string_view plugin_id) {
  return std::string{plugin_id};
}

} // namespace humanoid::plugins
