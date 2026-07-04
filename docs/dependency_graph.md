# Dependency Graph

The dependency graph must remain acyclic and point toward stable framework
interfaces.

```text
Applications
  -> humanoid::humanoid_core
  -> humanoid::robot_factory
  -> humanoid::adapter_interfaces

humanoid::humanoid_core
  -> humanoid::core
  -> humanoid::common
  -> humanoid::utilities
  -> humanoid::logging
  -> humanoid::configuration
  -> humanoid::robot
  -> humanoid::motion
  -> humanoid::gesture
  -> humanoid::safety
  -> humanoid::diagnostics
  -> humanoid::network
  -> humanoid::adapter_interfaces
  -> humanoid::robot_factory
  -> humanoid::telemetry_service

humanoid::plugins
  -> humanoid::common

humanoid::unitree_g1_plugin
  -> humanoid::plugins
  -> humanoid::humanoid_core

humanoid::robot_factory
  -> humanoid::adapter_interfaces

humanoid::telemetry_service
  -> humanoid::core

humanoid::core::CoreContext
  -> injected RobotStateManager

Robot state flow
  -> humanoid::core::RobotState
  -> humanoid::core::RobotStateManager
  -> humanoid::telemetry_service
  -> subscriber callbacks

humanoid::unitree_adapter
  -> humanoid::adapter_interfaces
  -> humanoid::logging
  -> humanoid::unitree_loco_client

humanoid::unitree_loco_client
  -> humanoid::adapter_interfaces
  -> humanoid::unitree_sdk_abstraction

humanoid::unitree_sdk_abstraction
  -> unitree_sdk2

unitree_sdk2
  -> ddsc
  -> ddscxx
  -> Threads
```

## Allowed Directions

- Applications may depend on managers, interfaces, and factory registry.
- Factory registry may depend on factory interfaces.
- Factories may create concrete adapters.
- Plugin hosts may depend on `humanoid::plugins`.
- Plugin registry and factory infrastructure may depend on `common`.
- Concrete plugin packages may depend on `humanoid::plugins` and public
  framework interfaces.
- Adapters may depend on SDK wrapper facades.
- SDK wrapper facades may depend on vendor SDK abstraction targets.
- Vendor SDK abstraction targets may depend on vendor SDKs.
- Managers may depend on module interfaces and `common`.
- Runtime services may depend on core state models and managers.
- Runtime services receive core services through dependency injection.
- `common` depends only on the C++ standard library.

## Forbidden Dependencies

- Applications including vendor SDK headers.
- Applications directly constructing concrete robot adapters.
- Managers depending on concrete adapters or vendor SDKs.
- Core modules depending on Unitree SDK2, ROS2, OpenCV, AI runtimes, GUI
  frameworks, mission engines, planners, navigation, or behavior trees.
- Core framework modules depending on concrete plugins.
- `humanoid::humanoid_core` linking concrete plugins or plugin implementations.
- SDK-free plugin skeletons including vendor SDK headers.
- SDK wrapper facades including vendor SDK headers directly.
- Vendor SDK types in public interfaces.
- Global singleton access as a framework dependency pattern.
