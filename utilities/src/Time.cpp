#include <humanoid/utilities/Time.hpp>

namespace humanoid::utilities {

SteadyTimePoint now() noexcept { return SteadyClock::now(); }

std::chrono::nanoseconds elapsedSince(SteadyTimePoint start) noexcept {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(now() - start);
}

} // namespace humanoid::utilities
