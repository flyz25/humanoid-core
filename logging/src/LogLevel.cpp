#include <humanoid/logging/LogLevel.hpp>

namespace humanoid::logging {

std::string_view toString(LogLevel level) noexcept {
  switch (level) {
  case LogLevel::kTrace:
    return "trace";
  case LogLevel::kDebug:
    return "debug";
  case LogLevel::kInfo:
    return "info";
  case LogLevel::kWarning:
    return "warning";
  case LogLevel::kError:
    return "error";
  case LogLevel::kCritical:
    return "critical";
  }

  return "unknown";
}

} // namespace humanoid::logging
