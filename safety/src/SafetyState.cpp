#include <humanoid/safety/SafetyState.hpp>

namespace humanoid::safety {

std::string_view toString(SafetyState state) noexcept {
  switch (state) {
  case SafetyState::kUnknown:
    return "unknown";
  case SafetyState::kNominal:
    return "nominal";
  case SafetyState::kProtectiveStop:
    return "protective_stop";
  case SafetyState::kEmergencyStop:
    return "emergency_stop";
  case SafetyState::kFaulted:
    return "faulted";
  }

  return "unknown";
}

} // namespace humanoid::safety
