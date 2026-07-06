# ROS2 Integration API

The ROS2 integration layer is optional and lives entirely under `ros2/`.

## Public Headers

- `humanoid/ros2/bridge/ROS2Bridge.h`
- `humanoid/ros2/messages/ROS2MessageTypes.h`
- `humanoid/ros2/topics/TopicCatalog.h`
- `humanoid/ros2/services/ServiceCatalog.h`
- `humanoid/ros2/actions/ActionCatalog.h`

## Bridge

`humanoid::ros2::bridge::ROS2Bridge` is a thread-safe facade over injected
humanoid-core dependencies. It accepts dependencies through
`ROS2BridgeDependencies`; null dependencies are reported as rejected operations
instead of causing undefined behavior.

The bridge owns no global state and performs no implicit ROS2 initialization.
Runtime ROS2 nodes should wrap the bridge and translate generated ROS2 messages
to the framework-owned DTOs.

## Topics

The topic catalog covers robot state, telemetry, battery, joint state, pose,
velocity, diagnostics, planner status, mission status, behavior tree status,
perception results, detection results, runtime status, command input, mission
requests, planner goals, emergency stop, and robot mode.

## Services

The service catalog covers robot connection lifecycle, mission lifecycle,
behavior tree loading, planner goal loading, sensor control, and health checks.

## Actions

The action catalog covers mission execution, behavior tree execution, navigate,
wave, greeting, inspection, and custom actions.

## Message DTOs

`ROS2MessageTypes.h` defines DTOs for robot state, telemetry, command, command
result, detection result, planner status, mission status, behavior tree status,
runtime status, sensor frame, and diagnostics. These DTOs contain no ROS2
headers and are designed for explicit translation at the optional ROS2 runtime
boundary.
