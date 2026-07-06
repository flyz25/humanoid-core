#pragma once

/**
 * @file ROS2MessageTypes.h
 * @brief Defines ROS2-facing message DTOs used by the optional bridge layer.
 *
 * These types are not generated ROS messages and intentionally include no ROS2
 * headers. They are stable framework-owned transfer objects used by bridge
 * code, tests, and examples. A ROS2 runtime adapter may translate these DTOs to
 * generated `.msg`, `.srv`, and `.action` types inside the optional ROS2
 * package boundary.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/RobotState.hpp>
#include <humanoid/perception/DetectionResult.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/runtime/ExecutionState.h>

namespace humanoid::ros2::messages {

/**
 * @brief Monotonic timestamp used by ROS2 bridge DTOs.
 */
using BridgeTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Robot state DTO published by the ROS2 bridge.
 */
struct RobotStateMessage final {
  /** @brief True when robot communication is established. */
  bool connected{false};

  /** @brief Battery level in percent. */
  float batteryLevel{0.0F};

  /** @brief True when the robot is charging. */
  bool charging{false};

  /** @brief True when the robot reports a standing posture. */
  bool standing{false};

  /** @brief True when the robot reports walking motion. */
  bool walking{false};

  /** @brief True when the robot reports a seated posture. */
  bool sitting{false};

  /** @brief Forward linear velocity in meters per second. */
  float linearX{0.0F};

  /** @brief Lateral linear velocity in meters per second. */
  float linearY{0.0F};

  /** @brief Yaw angular velocity in radians per second. */
  float angularZ{0.0F};

  /** @brief X position in meters. */
  float positionX{0.0F};

  /** @brief Y position in meters. */
  float positionY{0.0F};

  /** @brief Z position in meters. */
  float positionZ{0.0F};

  /** @brief Roll angle in radians. */
  float roll{0.0F};

  /** @brief Pitch angle in radians. */
  float pitch{0.0F};

  /** @brief Yaw angle in radians. */
  float yaw{0.0F};

  /** @brief True when emergency stop is active. */
  bool emergencyStop{false};

  /** @brief Vendor-normalized fault code. */
  std::int32_t faultCode{0};

  /** @brief State sample timestamp. */
  BridgeTimestamp timestamp{};
};

/**
 * @brief Telemetry DTO carrying the latest robot state snapshot.
 */
struct TelemetryMessage final {
  /** @brief Telemetry source identifier. */
  std::string source;

  /** @brief Latest robot state payload. */
  RobotStateMessage robotState;
};

/**
 * @brief Generic command DTO accepted by the ROS2 bridge.
 */
struct CommandMessage final {
  /** @brief Framework command identifier. */
  humanoid::core::CommandId id{0U};

  /** @brief Command type name, for example "Stand" or "Move". */
  std::string type{"Custom"};

  /** @brief Command priority name. */
  std::string priority{"Normal"};

  /** @brief Command timeout in milliseconds; zero disables timeout. */
  std::int64_t timeoutMs{0};

  /** @brief String-valued ROS-facing payload parameters. */
  std::map<std::string, std::string, std::less<>> payload;

  /** @brief Non-operational correlation metadata. */
  std::map<std::string, std::string, std::less<>> metadata;
};

/**
 * @brief Command execution result DTO returned by bridge operations.
 */
struct CommandResultMessage final {
  /** @brief Command identifier associated with the result. */
  humanoid::core::CommandId id{0U};

  /** @brief Command status name. */
  std::string status;

  /** @brief Human-readable result message. */
  std::string message;

  /** @brief True when the command reached `Completed`. */
  bool success{false};
};

/**
 * @brief Detection result DTO published by perception bridge endpoints.
 */
struct DetectionResultMessage final {
  /** @brief Stable detection identifier. */
  std::string id;

  /** @brief Detection category name. */
  std::string type;

  /** @brief Confidence in range [0, 1] when known. */
  double confidence{0.0};

  /** @brief Human-readable class or segment label. */
  std::string label;

  /** @brief Bounding box left coordinate. */
  double x{0.0};

  /** @brief Bounding box top coordinate. */
  double y{0.0};

  /** @brief Bounding box width. */
  double width{0.0};

  /** @brief Bounding box height. */
  double height{0.0};

  /** @brief True when position fields contain a 3D position. */
  bool hasPosition{false};

  /** @brief X coordinate of optional 3D position. */
  double positionX{0.0};

  /** @brief Y coordinate of optional 3D position. */
  double positionY{0.0};

  /** @brief Z coordinate of optional 3D position. */
  double positionZ{0.0};

  /** @brief Coordinate frame for optional 3D position. */
  std::string coordinateFrame;

  /** @brief Optional tracking identifier. */
  std::string trackingId;

  /** @brief Detection timestamp. */
  BridgeTimestamp timestamp{};
};

/**
 * @brief Planner status DTO published by planner bridge endpoints.
 */
struct PlannerStatusMessage final {
  /** @brief Planner or pipeline identifier. */
  std::string plannerId;

  /** @brief Goal identifier. */
  std::string goalId;

  /** @brief Planner status name. */
  std::string status;

  /** @brief Human-readable diagnostics summary. */
  std::string diagnostics;
};

/**
 * @brief Mission status DTO published by mission bridge endpoints.
 */
struct MissionStatusMessage final {
  /** @brief Mission identifier. */
  std::string missionId;

  /** @brief Mission status name. */
  std::string status;

  /** @brief Current step identifier when known. */
  std::string currentStepId;
};

/**
 * @brief Behavior tree status DTO published by BT bridge endpoints.
 */
struct BehaviorTreeStatusMessage final {
  /** @brief Behavior tree identifier. */
  std::string treeId;

  /** @brief Current behavior tree status name. */
  std::string status;

  /** @brief Currently active node identifier when known. */
  std::string activeNodeId;
};

/**
 * @brief Runtime status DTO published by execution runtime bridge endpoints.
 */
struct RuntimeStatusMessage final {
  /** @brief Runtime execution identifier. */
  std::uint64_t executionId{0U};

  /** @brief Runtime lifecycle state. */
  humanoid::runtime::ExecutionState state{humanoid::runtime::ExecutionState::Created};

  /** @brief True when cooperative cancellation has been requested. */
  bool cancellationRequested{false};
};

/**
 * @brief Sensor frame DTO for ROS2-facing sensor stream boundaries.
 */
struct SensorFrameMessage final {
  /** @brief Sensor identifier. */
  std::string sensorId;

  /** @brief Sensor type name. */
  std::string sensorType;

  /** @brief Frame sequence number. */
  std::uint64_t sequenceNumber{0U};

  /** @brief Frame timestamp. */
  BridgeTimestamp timestamp{};
};

/**
 * @brief Diagnostics DTO published by health bridge endpoints.
 */
struct DiagnosticsMessage final {
  /** @brief Diagnostic severity name. */
  std::string level;

  /** @brief Component or subsystem identifier. */
  std::string component;

  /** @brief Human-readable diagnostic summary. */
  std::string message;
};

/**
 * @brief Converts a framework robot state into a ROS2 bridge DTO.
 *
 * @param state Framework robot state.
 * @return ROS2 bridge state DTO.
 */
[[nodiscard]] RobotStateMessage FromRobotState(const humanoid::core::RobotState& state) noexcept;

/**
 * @brief Converts a framework detection result into a ROS2 bridge DTO.
 *
 * @param detection Framework detection result.
 * @return ROS2 bridge detection DTO.
 */
[[nodiscard]] DetectionResultMessage
FromDetectionResult(const humanoid::perception::DetectionResult& detection);

/**
 * @brief Converts a ROS2 bridge command DTO into a framework command.
 *
 * @param message Bridge command DTO.
 * @return Framework command value.
 */
[[nodiscard]] humanoid::core::Command ToCommand(const CommandMessage& message);

/**
 * @brief Converts a framework command result into a ROS2 bridge result DTO.
 *
 * @param result Framework command result.
 * @return ROS2 bridge command result DTO.
 */
[[nodiscard]] CommandResultMessage FromCommandResult(const humanoid::core::CommandResult& result);

} // namespace humanoid::ros2::messages
