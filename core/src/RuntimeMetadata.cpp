#include <humanoid/core/RuntimeMetadata.hpp>

#include <utility>

namespace humanoid::core {

RuntimeMetadata::RuntimeMetadata(std::string application_name, std::filesystem::path root_path)
    : application_name_(std::move(application_name)), root_path_(std::move(root_path)) {}

const std::string& RuntimeMetadata::applicationName() const noexcept { return application_name_; }

const std::filesystem::path& RuntimeMetadata::rootPath() const noexcept { return root_path_; }

common::SemanticVersion RuntimeMetadata::apiVersion() const noexcept {
  return common::apiVersion();
}

} // namespace humanoid::core
