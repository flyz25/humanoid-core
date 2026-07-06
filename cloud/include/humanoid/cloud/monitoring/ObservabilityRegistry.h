#pragma once

/**
 * @file ObservabilityRegistry.h
 * @brief Defines optional observability, metrics, tracing, and health records.
 */

#include <shared_mutex>
#include <string>
#include <vector>

#include <humanoid/cloud/CloudTypes.h>

namespace humanoid::cloud::monitoring {

/**
 * @brief Numeric metric sample.
 */
struct MetricSample final {
  /** @brief Metric name. */
  std::string name;

  /** @brief Metric value. */
  double value{0.0};

  /** @brief Metric tags. */
  humanoid::cloud::Metadata tags;

  /** @brief Sample timestamp. */
  humanoid::cloud::CloudTimestamp timestamp{};
};

/**
 * @brief Trace span record.
 */
struct TraceSpan final {
  /** @brief Trace identifier. */
  std::string traceId;

  /** @brief Span identifier. */
  std::string spanId;

  /** @brief Operation name. */
  std::string operation;

  /** @brief Duration in milliseconds. */
  double durationMs{0.0};
};

/**
 * @brief Health endpoint snapshot.
 */
struct HealthSnapshot final {
  /** @brief True when the platform reports healthy status. */
  bool healthy{true};

  /** @brief Human-readable status summary. */
  std::string message{"healthy"};
};

/**
 * @brief Thread-safe observability registry.
 */
class ObservabilityRegistry final {
public:
  /** @brief Records a metric sample. */
  void RecordMetric(MetricSample sample);

  /** @brief Records a trace span. */
  void RecordTrace(TraceSpan span);

  /** @brief Updates health endpoint state. */
  void SetHealth(HealthSnapshot health);

  /** @brief Returns metric samples. */
  [[nodiscard]] std::vector<MetricSample> Metrics() const;

  /** @brief Returns trace spans. */
  [[nodiscard]] std::vector<TraceSpan> Traces() const;

  /** @brief Returns latest health state. */
  [[nodiscard]] HealthSnapshot Health() const;

private:
  mutable std::shared_mutex mutex_;
  std::vector<MetricSample> metrics_;
  std::vector<TraceSpan> traces_;
  HealthSnapshot health_{};
};

} // namespace humanoid::cloud::monitoring
