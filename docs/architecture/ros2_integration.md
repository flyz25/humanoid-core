# ROS2 Integration Architecture

Milestone 11 introduces ROS2 as an optional ecosystem integration layer.

## Dependency Rule

```text
ROS2 application or launch file
  -> ROS2Bridge
    -> humanoid-core interfaces
      -> plugin architecture
        -> SDK abstraction
          -> vendor SDK
```

Forbidden directions:

- humanoid-core -> ROS2
- core headers -> `rclcpp`
- core headers -> generated ROS2 message headers
- core headers -> DDS middleware headers
- ROS2 bridge -> vendor SDK

## Optional Build

`ENABLE_ROS2` controls ROS2 runtime discovery. If ROS2 is unavailable, CMake
continues building the framework, bridge contracts, interface assets, examples,
tests, install rules, and packages. Runtime node adapters remain disabled.

## Bridge Boundary

`ROS2Bridge` exposes thread-safe lifecycle, robot-state readout, command
submission, telemetry snapshot caching, and endpoint catalog reporting. It uses
dependency injection for framework services and owns no singletons or global
objects.

## Interface Assets

The `ros2/messages` directory contains reusable `.msg`, `.srv`, and `.action`
definitions for downstream ROS2 packages. They mirror framework concepts without
replacing the canonical humanoid-core models.

## RViz and Launch

RViz configuration and launch files are provided as integration assets. They do
not hardcode robot vendors, SDK paths, or physical robot assumptions.
