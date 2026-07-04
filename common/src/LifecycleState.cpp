#include <humanoid/common/LifecycleState.hpp>

namespace humanoid::common {

std::string_view toString(LifecycleState state) noexcept {
  switch (state) {
  case LifecycleState::kUnconfigured:
    return "unconfigured";
  case LifecycleState::kConfigured:
    return "configured";
  case LifecycleState::kInactive:
    return "inactive";
  case LifecycleState::kActive:
    return "active";
  case LifecycleState::kFaulted:
    return "faulted";
  }

  return "unknown";
}

} // namespace humanoid::common
