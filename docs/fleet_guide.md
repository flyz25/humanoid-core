# Fleet Guide

`humanoid::cloud::fleet::FleetManager` provides thread-safe fleet metadata for
enterprise deployments.

## Supported Operations

- Robot registration and unregistration.
- Heartbeat and health state updates.
- Robot group assignment.
- Fleet status summaries.
- Mission distribution records.
- Capability metadata on each registered robot.

The manager performs no robot communication. Robot lifecycle actions still flow
through humanoid-core interfaces, command dispatch, mission execution, and
runtime services.

## Example

```cpp
humanoid::cloud::fleet::FleetManager fleet;
humanoid::cloud::fleet::RobotRecord robot{};
robot.robotId = "robot-001";
robot.vendor = "Unitree";
robot.model = "G1";
fleet.RegisterRobot(robot);
fleet.Heartbeat(robot.robotId, true);
```
