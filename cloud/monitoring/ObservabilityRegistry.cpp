#include <humanoid/cloud/monitoring/ObservabilityRegistry.h>

#include <chrono>
#include <mutex>
#include <utility>

namespace humanoid::cloud::monitoring {
namespace {

[[nodiscard]] humanoid::cloud::CloudTimestamp now() noexcept {
  return humanoid::cloud::CloudTimestamp{std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch())};
}

} // namespace

void ObservabilityRegistry::RecordMetric(MetricSample sample) {
  if (sample.timestamp == humanoid::cloud::CloudTimestamp{}) {
    sample.timestamp = now();
  }

  std::unique_lock lock{mutex_};
  metrics_.push_back(std::move(sample));
}

void ObservabilityRegistry::RecordTrace(TraceSpan span) {
  std::unique_lock lock{mutex_};
  traces_.push_back(std::move(span));
}

void ObservabilityRegistry::SetHealth(HealthSnapshot health) {
  std::unique_lock lock{mutex_};
  health_ = std::move(health);
}

std::vector<MetricSample> ObservabilityRegistry::Metrics() const {
  std::shared_lock lock{mutex_};
  return metrics_;
}

std::vector<TraceSpan> ObservabilityRegistry::Traces() const {
  std::shared_lock lock{mutex_};
  return traces_;
}

HealthSnapshot ObservabilityRegistry::Health() const {
  std::shared_lock lock{mutex_};
  return health_;
}

} // namespace humanoid::cloud::monitoring
