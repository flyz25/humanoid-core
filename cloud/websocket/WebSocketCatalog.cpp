#include <humanoid/cloud/websocket/WebSocketCatalog.h>

namespace humanoid::cloud::websocket {

std::vector<WebSocketStream> DefaultWebSocketStreams() {
  return {
      {"robot_state", "/ws/v1/robot-state", "RobotState"},
      {"telemetry", "/ws/v1/telemetry", "Telemetry"},
      {"mission_status", "/ws/v1/mission-status", "MissionStatus"},
      {"behavior_tree_status", "/ws/v1/behavior-tree-status", "BehaviorTreeStatus"},
      {"planner_status", "/ws/v1/planner-status", "PlannerStatus"},
      {"perception_results", "/ws/v1/perception-results", "PerceptionResults"},
      {"runtime_status", "/ws/v1/runtime-status", "RuntimeStatus"},
      {"diagnostics", "/ws/v1/diagnostics", "Diagnostics"},
  };
}

} // namespace humanoid::cloud::websocket
