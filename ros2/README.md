# ROS2 Integration

The `ros2/` module provides the optional ROS2 ecosystem boundary for
humanoid-core. It depends on humanoid-core; humanoid-core does not depend on
ROS2.

## Build Behavior

`ENABLE_ROS2` defaults to `ON`. When `rclcpp` is not available, CMake builds the
ROS2 bridge contracts, examples, tests, interface assets, launch files, and RViz
configuration without enabling runtime ROS2 node endpoints.

```bash
cmake -S . -B build -DENABLE_ROS2=ON
cmake --build build
```

Disable ROS2 runtime discovery with:

```bash
cmake -S . -B build -DENABLE_ROS2=OFF
```

## Dependency Direction

```text
ROS2 applications
  -> ROS2Bridge
    -> humanoid-core interfaces
      -> plugin architecture
        -> SDK wrappers
```

Core headers must never include `rclcpp`, generated ROS2 message headers, DDS
headers, or vendor SDK headers.

## Directories

- `bridge/`: C++ bridge implementation and DTO conversion.
- `include/humanoid/ros2/`: public ROS2 bridge contracts.
- `messages/`: `.msg`, `.srv`, and `.action` interface definitions.
- `topics/`: topic catalog.
- `services/`: service catalog.
- `actions/`: action catalog.
- `launch/`: ROS2 launch entry points.
- `rviz/`: RViz visualization configuration.
- `examples/`: hardware-free bridge examples.
- `tests/`: bridge contract validation.

## Runtime Adapter Strategy

The current bridge exposes framework-owned DTOs and endpoint catalogs. A
downstream ROS2 runtime package may translate generated ROS2 messages into these
DTOs and call `humanoid::ros2::bridge::ROS2Bridge`. This keeps ROS2 removable
from the framework build while preserving stable integration contracts.
