#include <humanoid/plugins/PluginMetadata.hpp>

namespace humanoid::plugins {

bool PluginMetadata::IsValid() const noexcept {
  return !plugin_id.empty() && !name.empty() && !vendor.empty() &&
         compatibility.IsValid();
}

bool PluginMetadata::IsCompatibleWith(
    humanoid::common::SemanticVersion framework_version) const noexcept {
  return compatibility.IsCompatibleWith(framework_version);
}

} // namespace humanoid::plugins
