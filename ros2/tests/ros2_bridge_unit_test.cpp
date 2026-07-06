#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/perception/DetectionResult.h>
#include <humanoid/ros2/bridge/ROS2Bridge.h>

namespace {

void Require(bool condition, const char* expression, int line) {
  if (!condition) {
    std::cerr << "Requirement failed at line " << line << ": " << expression << '\n';
    std::abort();
  }
}

#define HUMANOID_REQUIRE(condition) Require((condition), #condition, __LINE__)

void VerifyBridgeLifecycleAndStateTranslation() {
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::core::RobotState state{};
  state.connection.connected = true;
  state.power.batteryLevel = 72.5F;
  state.motion.standing = true;
  state.velocity.linearX = 0.25F;
  state.pose.positionZ = 1.1F;
  state.health.faultCode = 0;
  state_manager->UpdateState(state);

  humanoid::ros2::bridge::ROS2BridgeDependencies dependencies{};
  dependencies.robotStateManager = state_manager;
  humanoid::ros2::bridge::ROS2Bridge bridge{std::move(dependencies)};

  HUMANOID_REQUIRE(!bridge.IsRunning());
  HUMANOID_REQUIRE(bridge.Start());
  HUMANOID_REQUIRE(bridge.IsRunning());
  HUMANOID_REQUIRE(!bridge.Start());

  const auto status = bridge.Status();
  HUMANOID_REQUIRE(status.running);
  HUMANOID_REQUIRE(status.publisherCount == humanoid::ros2::topics::publisherTopicCount());
  HUMANOID_REQUIRE(status.subscriberCount == humanoid::ros2::topics::subscriberTopicCount());
  HUMANOID_REQUIRE(status.serviceCount == humanoid::ros2::services::serviceCount());
  HUMANOID_REQUIRE(status.actionCount == humanoid::ros2::actions::actionCount());

  const auto message = bridge.ReadRobotState();
  HUMANOID_REQUIRE(message.connected);
  HUMANOID_REQUIRE(message.batteryLevel == 72.5F);
  HUMANOID_REQUIRE(message.standing);
  HUMANOID_REQUIRE(message.linearX == 0.25F);
  HUMANOID_REQUIRE(message.positionZ == 1.1F);

  bridge.Stop();
  HUMANOID_REQUIRE(!bridge.IsRunning());
}

void VerifyCommandTranslationAndRejection() {
  humanoid::ros2::bridge::ROS2Bridge bridge{{}};

  humanoid::ros2::messages::CommandMessage command{};
  command.id = 42U;
  command.type = "Move";
  command.priority = "Critical";
  command.timeoutMs = 250;
  command.payload.emplace("vx", "0.5");
  command.payload.emplace("enabled", "true");
  command.metadata.emplace("source", "ros2-test");

  const auto translated = humanoid::ros2::messages::ToCommand(command);
  HUMANOID_REQUIRE(translated.id == 42U);
  HUMANOID_REQUIRE(translated.type == humanoid::core::CommandType::Move);
  HUMANOID_REQUIRE(translated.priority == humanoid::core::CommandPriority::Critical);
  HUMANOID_REQUIRE(translated.timeout.count() == 250);
  HUMANOID_REQUIRE(translated.metadata.at("source") == "ros2-test");

  const auto rejected = bridge.SubmitCommand(command);
  HUMANOID_REQUIRE(rejected.id == 42U);
  HUMANOID_REQUIRE(rejected.status == "Rejected");
  HUMANOID_REQUIRE(!rejected.success);
}

void VerifyDetectionTranslation() {
  humanoid::perception::DetectionResult detection{};
  detection.id = "det-1";
  detection.type = humanoid::perception::DetectionType::Object;
  detection.confidence = 0.91;
  detection.label = "person";
  detection.boundingBox = humanoid::perception::BoundingBox2D{10.0, 20.0, 30.0, 40.0};
  detection.position = humanoid::perception::Position3D{1.0, 2.0, 3.0, "map"};
  detection.trackingId = "track-7";

  const auto message = humanoid::ros2::messages::FromDetectionResult(detection);
  HUMANOID_REQUIRE(message.id == "det-1");
  HUMANOID_REQUIRE(message.type == "Object");
  HUMANOID_REQUIRE(message.confidence == 0.91);
  HUMANOID_REQUIRE(message.label == "person");
  HUMANOID_REQUIRE(message.width == 30.0);
  HUMANOID_REQUIRE(message.hasPosition);
  HUMANOID_REQUIRE(message.positionZ == 3.0);
  HUMANOID_REQUIRE(message.coordinateFrame == "map");
  HUMANOID_REQUIRE(message.trackingId == "track-7");
}

} // namespace

int main() {
  VerifyBridgeLifecycleAndStateTranslation();
  VerifyCommandTranslationAndRejection();
  VerifyDetectionTranslation();
  std::cout << "ROS2 bridge unit test passed\n";
  return EXIT_SUCCESS;
}
