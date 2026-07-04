#include <humanoid/plugins/PluginLifecycleState.hpp>

namespace humanoid::plugins {

std::string_view toString(PluginLifecycleState state) noexcept {
  switch (state) {
  case PluginLifecycleState::kDiscovered:
    return "discovered";
  case PluginLifecycleState::kRegistered:
    return "registered";
  case PluginLifecycleState::kLoaded:
    return "loaded";
  case PluginLifecycleState::kInitialized:
    return "initialized";
  case PluginLifecycleState::kStarted:
    return "started";
  case PluginLifecycleState::kStopped:
    return "stopped";
  case PluginLifecycleState::kShutdown:
    return "shutdown";
  case PluginLifecycleState::kFailed:
    return "failed";
  }

  return "unknown";
}

} // namespace humanoid::plugins
