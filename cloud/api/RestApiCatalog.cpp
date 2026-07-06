#include <humanoid/cloud/api/RestApiCatalog.h>

#include <algorithm>

namespace humanoid::cloud::api {

RestApiCatalog::RestApiCatalog()
    : endpoints_{
          {"robot.list", "v1", HttpMethod::Get, "/api/v1/robots", "Robot", true},
          {"robot.connect", "v1", HttpMethod::Post, "/api/v1/robots/{id}/connect", "Robot", true},
          {"robot.disconnect", "v1", HttpMethod::Post, "/api/v1/robots/{id}/disconnect", "Robot",
           true},
          {"mission.list", "v1", HttpMethod::Get, "/api/v1/missions", "Mission", true},
          {"mission.start", "v1", HttpMethod::Post, "/api/v1/missions/{id}/start", "Mission", true},
          {"bt.load", "v1", HttpMethod::Post, "/api/v1/behavior-trees", "BehaviorTree", true},
          {"planner.goal", "v1", HttpMethod::Post, "/api/v1/planner/goals", "Planner", true},
          {"telemetry.latest", "v1", HttpMethod::Get, "/api/v1/telemetry/latest", "Telemetry",
           true},
          {"perception.results", "v1", HttpMethod::Get, "/api/v1/perception/results", "Perception",
           true},
          {"fleet.status", "v1", HttpMethod::Get, "/api/v1/fleet/status", "Fleet", true},
          {"runtime.status", "v1", HttpMethod::Get, "/api/v1/runtime/status", "Runtime", true},
          {"health.check", "v1", HttpMethod::Get, "/api/v1/health", "Health", false},
          {"diagnostics.list", "v1", HttpMethod::Get, "/api/v1/diagnostics", "Diagnostics", true},
          {"configuration.get", "v1", HttpMethod::Get, "/api/v1/configuration", "Configuration",
           true},
      } {}

const std::vector<RestEndpoint>& RestApiCatalog::Endpoints() const noexcept { return endpoints_; }

std::vector<RestEndpoint> RestApiCatalog::FindBySubsystem(std::string_view subsystem) const {
  std::vector<RestEndpoint> matches;
  std::copy_if(
      endpoints_.begin(), endpoints_.end(), std::back_inserter(matches),
      [subsystem](const RestEndpoint& endpoint) { return endpoint.subsystem == subsystem; });
  return matches;
}

} // namespace humanoid::cloud::api
