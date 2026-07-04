#include <humanoid/configuration/ConfigManager.hpp>

#include <utility>

namespace humanoid::configuration {

ConfigManager::ConfigManager(std::shared_ptr<const Configuration> configuration)
    : configuration_(std::move(configuration)) {}

common::Status ConfigManager::setConfiguration(std::shared_ptr<const Configuration> configuration) {
  if (!configuration) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "configuration provider is null");
  }

  configuration_ = std::move(configuration);
  return common::Status::ok();
}

void ConfigManager::clear() noexcept { configuration_.reset(); }

bool ConfigManager::hasConfiguration() const noexcept { return static_cast<bool>(configuration_); }

bool ConfigManager::contains(std::string_view key) const {
  return configuration_ ? configuration_->contains(key) : false;
}

std::optional<ConfigValue> ConfigManager::value(std::string_view key) const {
  if (!configuration_) {
    return std::nullopt;
  }

  return configuration_->value(key);
}

std::optional<std::filesystem::path> ConfigManager::sourcePath() const {
  if (!configuration_) {
    return std::nullopt;
  }

  return configuration_->sourcePath();
}

} // namespace humanoid::configuration
