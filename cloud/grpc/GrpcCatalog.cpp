#include <humanoid/cloud/grpc/GrpcCatalog.h>

namespace humanoid::cloud::grpc {

std::vector<GrpcMethod> DefaultGrpcCatalog() {
  return {
      {"RobotControlService", "Connect", false},  {"RobotControlService", "Disconnect", false},
      {"MissionService", "ExecuteMission", true}, {"TelemetryService", "StreamTelemetry", true},
      {"PlannerService", "Plan", false},          {"RuntimeService", "StreamRuntimeStatus", true},
      {"FleetService", "ListRobots", false},      {"FleetService", "StreamFleetStatus", true},
  };
}

} // namespace humanoid::cloud::grpc
