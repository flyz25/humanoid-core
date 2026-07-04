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
            -> SdkWrapper
              -> LocoAdapter / HandAdapter / AudioAdapter
                -> Unitree SDK2
```

`IRobotAdapter` is the application-facing dependency. `UnitreeRobotFactory`
creates `UnitreeG1Adapter` through the generic factory interface.
`UnitreeG1Adapter` translates generic commands such as `Move`, `Stop`, `StandUp`,
`BalanceStand`, and `EmergencyStop`. `LocoClientWrapper` is a compatibility
facade over `plugins/unitree/sdk/SdkWrapper`. Only implementation files under
`plugins/unitree/sdk/` include Unitree SDK2 headers and own SDK client objects.

Milestone 4.6 adds read-only SDK2 communication monitoring inside `SdkWrapper`.
Heartbeat, timeout detection, reconnect attempts, and state synchronization use
the SDK locomotion service query path and do not issue movement, posture, hand,
audio, or actuator commands. When `UnitreeRobotFactory` receives an injected
`RobotStateManager`, synchronized state is converted into
`humanoid::core::RobotState` before it leaves the SDK boundary.

The Unitree adapter is optional at build time. When `UnitreeSDK2` is not found,
the SDK-free core and adapter interface still build.

Milestone 4.7 adds SDK-boundary command adapters for locomotion, hand/gesture,
and audio commands. These adapters perform command translation only; they do not
contain business logic, mission execution, planning, AI, or behavior sequencing.

Milestone 4.8 adds integration examples that wire the existing targets from an
application composition root:

```text
Example application
  -> PluginFactory / PluginRegistry
    -> UnitreeG1Plugin
      -> core::RobotAdapter

Example application
  -> RobotFactoryRegistry
    -> UnitreeRobotFactory
      -> IRobotAdapter
        -> LocoClientWrapper
          -> SdkWrapper
```

The examples validate integration without changing dependency direction. Core
libraries do not link concrete plugins, plugin infrastructure does not link
vendor SDKs, and SDK-boundary examples are built only when the Unitree SDK
abstraction target exists.

See `docs/api/plugin_integration.md` for the public plugin integration API
summary and example target list.

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

## Generic Command Model

Milestone 5 adds a vendor-independent command path:

```text
Application or future command producer
  -> CommandDispatcher
    -> Command / CommandType / CommandPriority
      -> IRobotAdapter
  <- CommandResult / CommandStatus
```

Command IDs are assigned by the producer, timestamps use a monotonic clock, and
timeouts use `std::chrono`. Payload values and metadata contain framework-owned
standard-library types only. `CommandDispatcher` validates commands, serializes
adapter access, and supports synchronous and priority-aware asynchronous
forwarding. It depends only on `IRobotAdapter`; it has no SDK headers, concrete
adapter dependencies, mission logic, or global state. See
`docs/api/command_model.md` for the complete public contract.

## Plugin Infrastructure

Milestone 4 adds plugin infrastructure as a separate exported module:

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
registration, and lifecycle state. Plugin hosts may use
`humanoid::plugins::PluginFactory` to register creator callables, create plugin
instances, destroy plugin instances, and enumerate registered plugin records.

Milestone 4.4 adds the SDK-free Unitree G1 plugin skeleton. It packages
metadata, lifecycle, a plugin-local adapter skeleton, a manifest, and mock robot
state feedback without communicating with Unitree SDK2 or commanding motion.
Milestone 4 does not implement dynamic shared-library loading, manifest
parsing, or physical robot communication plugins.

## Module Ownership

- `common`: dependency-free lifecycle, status, and version primitives.
- `plugins`: plugin interfaces, metadata, version compatibility, lifecycle
  states, thread-safe registration registry, thread-safe plugin factory, and
  concrete plugin packages.
- `plugins/unitree/sdk`: Unitree SDK2 abstraction boundary and conversion layer.
- `utilities`: small implementation-agnostic helpers.
- `logging`: logger and sink interfaces plus sink routing infrastructure.
- `configuration`: read-only configuration interfaces and provider ownership.
- `robot`: robot and robot adapter abstractions.
- `motion`: motion controller abstractions.
- `gesture`: named gesture controller abstractions.
- `safety`: safety state and safety controller abstractions.
- `diagnostics`: diagnostic records and diagnostic controller abstractions.
- `network`: transport metadata and network manager abstraction.
- `core`: package metadata, application-facing interface context, generic robot
  state model, and generic command value model.
- `include/humanoid/adapters`: public robot adapter and factory contracts.
- `src/adapters`: adapter plugin implementations that depend on public adapter
  contracts and keep vendor SDK headers out of application-facing interfaces.
- `src/factory`: robot factory registry.
- `src/sdk`: legacy adapter-facing SDK facades hidden behind adapter implementations.
- `src/services`: vendor-independent runtime services such as telemetry
  publication.

## Adapter Rule

Vendor SDK headers must never be included by application code, manager headers,
or manager source files. Vendor SDK dependencies belong in adapter packages that
implement `IRobotAdapter` or other module interfaces.
