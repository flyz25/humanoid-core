#include <humanoid/cloud/CloudPlatform.h>
#include <humanoid/cloud/grpc/GrpcCatalog.h>
#include <humanoid/cloud/websocket/WebSocketCatalog.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "cloud platform test failed: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

} // namespace

int main() {
  humanoid::cloud::CloudPlatform platform;
  Require(platform.Start(), "platform should start once");
  Require(platform.Status().running, "platform should report running");
  Require(platform.Status().restEndpointCount >= 14U, "REST catalog should expose endpoints");

  const auto robot_endpoints = platform.RestApi()->FindBySubsystem("Robot");
  Require(robot_endpoints.size() == 3U, "robot subsystem endpoint count");

  const auto health_endpoints = platform.RestApi()->FindBySubsystem("Health");
  Require(!health_endpoints.empty(), "health endpoint should exist");
  Require(!health_endpoints.front().requiresAuthentication, "health endpoint should be public");

  humanoid::cloud::fleet::RobotRecord robot{};
  robot.robotId = "g1-stage-001";
  robot.vendor = "Unitree";
  robot.model = "G1";
  robot.capabilities = {"stand", "telemetry"};
  Require(platform.Fleet()->RegisterRobot(robot).ok(), "robot registration");
  Require(platform.Fleet()->Heartbeat("g1-stage-001", true).ok(), "robot heartbeat");
  Require(platform.Fleet()->AddRobotToGroup("stage", "g1-stage-001").ok(), "group add");
  Require(platform.Fleet()->DistributeMission("stage", "mission-greeting").ok(),
          "mission distribution");
  Require(platform.Fleet()->Status().robotCount == 1U, "fleet status robot count");

  humanoid::cloud::auth::Principal principal{};
  principal.principalId = "operator";
  principal.roles = {"operator"};
  Require(platform.Auth()->RegisterApiKey("local-key", principal).ok(), "api key register");
  Require(platform.Auth()->GrantPermission("operator", "robot.connect").ok(), "permission grant");
  Require(platform.Auth()->AuthorizeApiKey("local-key", "robot.connect"), "auth success");
  Require(!platform.Auth()->AuthorizeApiKey("local-key", "ota.rollback"), "auth rejection");
  Require(platform.Auth()->AuditLog().size() == 2U, "audit records");

  humanoid::cloud::ota::PackageDescriptor package{};
  package.packageId = "mission-pack";
  package.version = "1.0.0";
  package.type = humanoid::cloud::ota::PackageType::Mission;
  package.verificationDigest = "sha256:test";
  package.rollbackSupported = true;
  Require(platform.Ota()->RegisterPackage(package).ok(), "package register");
  Require(platform.Ota()->PlanUpdate("mission-pack", {"g1-stage-001"}).ok(), "update plan");
  Require(platform.Ota()->VerifyPackage("mission-pack", "sha256:test").ok(), "package verify");
  Require(platform.Ota()->Rollback("mission-pack").ok(), "rollback");

  humanoid::cloud::monitoring::MetricSample metric{};
  metric.name = "fleet.robots.online";
  metric.value = 1.0;
  platform.Observability()->RecordMetric(metric);
  platform.Observability()->RecordTrace({"trace-1", "span-1", "cloud.test", 0.5});
  platform.Observability()->SetHealth({true, "healthy"});
  Require(platform.Observability()->Metrics().size() == 1U, "metrics");
  Require(platform.Observability()->Traces().size() == 1U, "traces");
  Require(platform.Observability()->Health().healthy, "health snapshot");

  const auto grpc_catalog = humanoid::cloud::grpc::DefaultGrpcCatalog();
  Require(!grpc_catalog.empty(), "grpc catalog");
  const auto websocket_streams = humanoid::cloud::websocket::DefaultWebSocketStreams();
  Require(websocket_streams.size() >= 8U, "websocket streams");

  platform.Stop();
  Require(!platform.Status().running, "platform should stop");
  return EXIT_SUCCESS;
}
