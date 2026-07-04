#include <humanoid/plugins/PluginFactory.hpp>

#include <mutex>
#include <utility>

namespace humanoid::plugins {

namespace {

[[nodiscard]] humanoid::common::Status InvalidArgument(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status InternalError(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                         std::move(message));
}

} // namespace

PluginFactory::PluginFactory() : PluginFactory(std::make_shared<PluginRegistry>()) {}

PluginFactory::PluginFactory(std::shared_ptr<PluginRegistry> registry) noexcept
    : registry_(std::move(registry)) {}

humanoid::common::Status PluginFactory::RegisterPlugin(PluginMetadata metadata,
                                                       PluginCreator creator) {
  if (!registry_) {
    return FailedPrecondition("plugin registry is not available");
  }

  if (!creator) {
    return InvalidArgument("plugin creator is empty");
  }

  if (!metadata.IsValid()) {
    return InvalidArgument("plugin metadata is incomplete or invalid");
  }

  const std::string plugin_id = metadata.plugin_id;
  std::unique_lock<std::shared_mutex> lock{mutex_};
  if (factories_.find(plugin_id) != factories_.end()) {
    return FailedPrecondition("plugin creator is already registered");
  }

  const humanoid::common::Status registry_status = registry_->RegisterPlugin(metadata);
  if (!registry_status.isOk()) {
    return registry_status;
  }

  try {
    factories_.emplace(plugin_id, FactoryEntry{std::move(metadata), std::move(creator), 0U});
  } catch (...) {
    (void)registry_->UnregisterPlugin(plugin_id);
    return InternalError("failed to store plugin creator");
  }

  return humanoid::common::Status::ok();
}

humanoid::common::Status PluginFactory::UnregisterPlugin(std::string_view plugin_id) {
  if (!registry_) {
    return FailedPrecondition("plugin registry is not available");
  }

  if (plugin_id.empty()) {
    return InvalidArgument("plugin identifier is empty");
  }

  const std::string key = MakeKey(plugin_id);
  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = factories_.find(key);
  if (iterator == factories_.end()) {
    return FailedPrecondition("plugin creator is not registered");
  }

  if (iterator->second.active_instances > 0U) {
    return FailedPrecondition("plugin instances are still active");
  }

  const humanoid::common::Status registry_status = registry_->UnregisterPlugin(key);
  if (!registry_status.isOk()) {
    return registry_status;
  }

  factories_.erase(iterator);
  return humanoid::common::Status::ok();
}

PluginCreationResult PluginFactory::CreatePlugin(std::string_view plugin_id) {
  if (!registry_) {
    return PluginCreationResult{FailedPrecondition("plugin registry is not available"), nullptr};
  }

  if (plugin_id.empty()) {
    return PluginCreationResult{InvalidArgument("plugin identifier is empty"), nullptr};
  }

  const std::string key = MakeKey(plugin_id);
  PluginCreator creator;
  try {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(key);
    if (iterator == factories_.end()) {
      return PluginCreationResult{FailedPrecondition("plugin creator is not registered"), nullptr};
    }
    creator = iterator->second.creator;
    ++iterator->second.active_instances;
  } catch (...) {
    return PluginCreationResult{InternalError("failed to prepare plugin creator"), nullptr};
  }

  const auto release_active_instance = [this, &key]() {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(key);
    if (iterator != factories_.end() && iterator->second.active_instances > 0U) {
      --iterator->second.active_instances;
    }
  };

  std::unique_ptr<IPlugin> plugin;
  try {
    plugin = creator();
  } catch (...) {
    release_active_instance();
    (void)registry_->SetLifecycleState(key, PluginLifecycleState::kFailed);
    return PluginCreationResult{InternalError("plugin creator threw an exception"), nullptr};
  }

  if (!plugin) {
    release_active_instance();
    (void)registry_->SetLifecycleState(key, PluginLifecycleState::kFailed);
    return PluginCreationResult{InternalError("plugin creator returned null"), nullptr};
  }

  if (plugin->Metadata().plugin_id != key) {
    release_active_instance();
    (void)registry_->SetLifecycleState(key, PluginLifecycleState::kFailed);
    return PluginCreationResult{
        InternalError("created plugin metadata does not match requested identifier"), nullptr};
  }

  const humanoid::common::Status lifecycle_status =
      registry_->SetLifecycleState(key, PluginLifecycleState::kLoaded);
  if (!lifecycle_status.isOk()) {
    release_active_instance();
    return PluginCreationResult{lifecycle_status, nullptr};
  }

  return PluginCreationResult{humanoid::common::Status::ok(), std::move(plugin)};
}

humanoid::common::Status PluginFactory::DestroyPlugin(std::unique_ptr<IPlugin>& plugin) {
  if (!registry_) {
    return FailedPrecondition("plugin registry is not available");
  }

  if (!plugin) {
    return InvalidArgument("plugin instance is null");
  }

  const std::string plugin_id = plugin->Metadata().plugin_id;
  if (plugin_id.empty()) {
    return InvalidArgument("plugin identifier is empty");
  }

  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(plugin_id);
    if (iterator == factories_.end() || iterator->second.active_instances == 0U) {
      return FailedPrecondition("plugin instance is not tracked by this factory");
    }
  }

  const humanoid::common::Status shutdown_status = plugin->Shutdown();
  if (!shutdown_status.isOk()) {
    (void)registry_->SetLifecycleState(plugin_id, PluginLifecycleState::kFailed);
    return shutdown_status;
  }

  {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(plugin_id);
    if (iterator != factories_.end() && iterator->second.active_instances > 0U) {
      --iterator->second.active_instances;
    }
  }

  const humanoid::common::Status lifecycle_status =
      registry_->SetLifecycleState(plugin_id, PluginLifecycleState::kShutdown);
  plugin.reset();
  return lifecycle_status;
}

std::vector<PluginRecord> PluginFactory::EnumeratePlugins() const {
  if (!registry_) {
    return {};
  }

  return registry_->EnumeratePlugins();
}

std::size_t PluginFactory::RegisteredPluginCount() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return factories_.size();
}

std::shared_ptr<PluginRegistry> PluginFactory::Registry() const noexcept { return registry_; }

std::string PluginFactory::MakeKey(std::string_view plugin_id) { return std::string{plugin_id}; }

} // namespace humanoid::plugins
