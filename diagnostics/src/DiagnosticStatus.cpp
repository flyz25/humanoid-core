#include <humanoid/diagnostics/DiagnosticStatus.hpp>

namespace humanoid::diagnostics {

std::string_view toString(DiagnosticStatus status) noexcept {
  switch (status) {
  case DiagnosticStatus::kOk:
    return "ok";
  case DiagnosticStatus::kWarning:
    return "warning";
  case DiagnosticStatus::kError:
    return "error";
  case DiagnosticStatus::kStale:
    return "stale";
  }

  return "unknown";
}

} // namespace humanoid::diagnostics
