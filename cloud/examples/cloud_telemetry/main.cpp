#include <humanoid/cloud/monitoring/ObservabilityRegistry.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::monitoring::ObservabilityRegistry observability;
  humanoid::cloud::monitoring::MetricSample sample{};
  sample.name = "telemetry.robot_state.messages";
  sample.value = 42.0;

  observability.RecordMetric(sample);
  if (observability.Metrics().size() != 1U) {
    return EXIT_FAILURE;
  }

  std::cout << "Telemetry metrics: " << observability.Metrics().size() << '\n';
  return EXIT_SUCCESS;
}
