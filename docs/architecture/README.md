# Architecture

humanoid-core follows Clean Architecture with strict dependency direction:

```text
Applications
  -> Managers
    -> Interfaces
      -> Robot Factory
        -> Robot Adapter
          -> SDK Wrapper
            -> Vendor SDK
```

Applications depend on interfaces and managers. Managers depend only on abstract
interfaces. Robot adapters implement framework interfaces and isolate vendor
SDKs, simulators, middleware, or embedded transport details.

## Unitree G1 Communication Layer

Milestone 2 adds an optional Unitree G1 EDU communication adapter without
changing the core architecture:

```text
Application
  -> RobotFactoryRegistry
    -> IRobotFactory
      -> IRobotAdapter
        -> UnitreeG1Adapter
          -> LocoClientWrapper
            -> Unitree SDK2
```

`IRobotAdapter` is the application-facing dependency. `UnitreeRobotFactory`
creates `UnitreeG1Adapter` through the generic factory interface.
`UnitreeG1Adapter` translates generic commands such as `Move`, `Stop`, `StandUp`,
`BalanceStand`, and `EmergencyStop`. `LocoClientWrapper` is the only layer that
includes Unitree SDK2 headers and owns `unitree::robot::g1::LocoClient`.

The Unitree adapter is optional at build time. When `UnitreeSDK2` is not found,
the SDK-free core and adapter interface still build.

The core foundation modules do not implement ROS2, DDS participants, AI, OpenCV,
GUI workflows, mission engines, or event controllers. Unitree SDK2 integration
is isolated in the optional adapter and SDK wrapper targets.

## Robot State and Telemetry Layer

Milestone 3 adds a vendor-independent state path without changing the adapter or
factory architecture:

```text
Robot adapter or state producer
  -> RobotState
    -> RobotStateManager
      -> TelemetryService
        -> Subscriber callbacks
```

`RobotState` is the canonical value-type snapshot for connection, power, motion,
velocity, pose, orientation, health, and timestamp data. `RobotStateManager`
stores the latest snapshot behind `std::shared_mutex`, allowing shared readers
and exclusive writers. `TelemetryService` receives a
`std::shared_ptr<const RobotStateManager>` through dependency injection and
publishes copied snapshots to subscribed callbacks.

This layer remains SDK-free and vendor independent. It does not perform robot
communication, command execution, planning, navigation, AI, behavior trees, or
mission orchestration.

## Plugin Infrastructure

Milestone 4.1 adds plugin infrastructure as a separate exported module:

```text
Application or plugin host
  -> humanoid::plugins
    -> humanoid::common
```

The aggregate core target `humanoid::humanoid_core` does not link against
`humanoid::plugins`, and no core module depends on concrete plugins. Future
plugin hosts may use `humanoid::plugins::IPlugin`,
`humanoid::plugins::IPluginRegistrar`, `humanoid::plugins::IPluginLoader`, and
`humanoid::plugins::PluginRegistry` to manage metadata, version compatibility,
registration, and lifecycle state.

Milestone 4.1 does not implement vendor plugins, dynamic shared-library loading,
manifest parsing, or robot communication plugins.

## Module Ownership

- `common`: dependency-free lifecycle, status, and version primitives.
- `plugins`: plugin interfaces, metadata, version compatibility, lifecycle
  states, and thread-safe registration registry.
- `utilities`: small implementation-agnostic helpers.
- `logging`: logger and sink interfaces plus sink routing infrastructure.
- `configuration`: read-only configuration interfaces and provider ownership.
- `robot`: robot and robot adapter abstractions.
- `motion`: motion controller abstractions.
- `gesture`: named gesture controller abstractions.
- `safety`: safety state and safety controller abstractions.
- `diagnostics`: diagnostic records and diagnostic controller abstractions.
- `network`: transport metadata and network manager abstraction.
- `core`: package metadata, application-facing interface context, and generic
  robot state model.
- `include/humanoid/adapters`: public robot adapter and factory contracts.
- `src/adapters`: adapter plugin implementations that depend on public adapter
  contracts and keep vendor SDK headers out of application-facing interfaces.
- `src/factory`: robot factory registry.
- `src/sdk`: vendor SDK wrappers hidden behind adapter implementations.
- `src/services`: vendor-independent runtime services such as telemetry
  publication.

## Adapter Rule

Vendor SDK headers must never be included by application code, manager headers,
or manager source files. Vendor SDK dependencies belong in adapter packages that
implement `IRobotAdapter` or other module interfaces.
