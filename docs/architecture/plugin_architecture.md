# Plugin Architecture

Milestone 4 defines the plugin infrastructure for humanoid-core. The design
adds plugin contracts, a host registry, a creator-based plugin factory, and the
first SDK-free plugin skeleton without dynamic library loading or dependencies
from the core framework target to plugins.

## Goals

- Allow future robot vendors, simulators, diagnostics exporters, and other
  extensions to be delivered as plugins.
- Keep applications dependent on framework interfaces, not vendor SDKs.
- Keep `humanoid::humanoid_core` independent of concrete plugins and plugin
  implementations.
- Provide explicit lifecycle, metadata, compatibility, registration, and factory
  contracts before adding SDK-backed vendor plugins.

## Non-Goals

Milestone 4 does not implement:

- SDK-backed Unitree communication plugins.
- Simulator or mock robot plugins.
- Dynamic shared-library loading.
- Plugin manifest parsing.
- ROS2, DDS integration, AI, mission execution, planning, navigation, behavior
  trees, GUI code, or robot communication.

## Dependency Direction

The plugin infrastructure is a separate exported target:

```text
Application or plugin host
  -> humanoid::plugins
    -> humanoid::common
```

The core framework target remains independent:

```text
humanoid::humanoid_core
  -> core framework modules
  -> no plugin dependency
```

Future concrete plugins must depend on framework interfaces and plugin
contracts. Framework core modules must never depend on concrete plugins.

## Public Contracts

### IPlugin

`humanoid::plugins::IPlugin` is the base lifecycle interface implemented by
future plugins.

Lifecycle functions:

- `Metadata()`
- `Initialize(IPluginRegistrar&)`
- `Start()`
- `Stop()`
- `Shutdown()`

Plugin lifecycle functions return `humanoid::common::Status`. Plugin
implementations should translate internal exceptions or SDK failures into
status values before crossing the plugin boundary.

### IPluginRegistrar

`humanoid::plugins::IPluginRegistrar` is the host registration boundary passed
to plugin instances during initialization.

Registration functions:

- `RegisterPlugin(PluginMetadata)`
- `UnregisterPlugin(std::string_view)`
- `SetLifecycleState(std::string_view, PluginLifecycleState)`

This keeps plugin registration explicit and testable. It also avoids singleton
or global registries.

### IPluginLoader

`humanoid::plugins::IPluginLoader` defines the host-side loading abstraction for
future platform-specific loaders. Milestone 4.4 intentionally provides only the
interface.

Future loader responsibilities:

- Locate plugin package or shared-library paths.
- Read and validate plugin metadata before loading.
- Verify framework API compatibility.
- Load the binary through a platform implementation.
- Create the plugin instance.
- Drive `Initialize()`, `Start()`, `Stop()`, and `Shutdown()`.
- Unload only after plugin shutdown has completed.

### PluginRegistry

`humanoid::plugins::PluginRegistry` implements `IPluginRegistrar` and stores
metadata plus host-visible lifecycle state. It owns no plugin implementation
objects and performs no dynamic loading.

Registry functions:

- `RegisterPlugin(PluginMetadata)`
- `UnregisterPlugin(std::string_view)`
- `SetLifecycleState(std::string_view, PluginLifecycleState)`
- `Contains(std::string_view)`
- `Metadata(std::string_view)`
- `LifecycleState(std::string_view)`
- `EnumeratePlugins()`
- `PluginCount()`

### PluginFactory

`humanoid::plugins::PluginFactory` stores plugin creator functions and delegates
metadata and lifecycle visibility to an injected `PluginRegistry`. It is
thread-safe, owns no global state, and is intended to be constructed by an
application or future plugin host composition root.

Factory functions:

- `RegisterPlugin(PluginMetadata, PluginCreator)`
- `UnregisterPlugin(std::string_view)`
- `CreatePlugin(std::string_view)`
- `DestroyPlugin(std::unique_ptr<IPlugin>&)`
- `EnumeratePlugins()`
- `RegisteredPluginCount()`

`CreatePlugin()` returns a `PluginCreationResult`, which contains a
`humanoid::common::Status` and an optional `std::unique_ptr<IPlugin>`. Plugin
creation failures are reported through `Status`; creator exceptions are caught
and translated to an internal error status.

## Metadata

`humanoid::plugins::PluginMetadata` contains:

- `plugin_id`: stable plugin identifier.
- `name`: human-readable plugin name.
- `vendor`: organization responsible for the plugin.
- `description`: plugin purpose.
- `version`: plugin semantic version.
- `compatibility`: accepted framework API version range.
- `manifest_path`: optional path for future manifest-based loaders.

Required fields are validated before registry insertion. Current required
fields are `plugin_id`, `name`, `vendor`, and a valid compatibility range.

## Version Compatibility

`humanoid::plugins::PluginVersionCompatibility` declares an inclusive framework
API version range:

```text
minimumFrameworkVersion <= humanoid-core API version <= maximumFrameworkVersion
```

The registry rejects incompatible plugins before registration. This makes
version failures deterministic and visible before plugin runtime startup.

Milestone 4.x compatibility targets humanoid-core `0.4.0-alpha` through the
numeric API version `0.4.0`.

## Lifecycle

Plugin lifecycle states are represented by
`humanoid::plugins::PluginLifecycleState`:

```text
Discovered
Registered
Loaded
Initialized
Started
Stopped
Shutdown
Failed
```

Expected host flow:

```text
Discover metadata
  -> Validate compatibility
    -> Register metadata
      -> Load package or shared library
        -> Initialize plugin
          -> Start plugin
            -> Stop plugin
              -> Shutdown plugin
                -> Unload plugin
```

Milestone 4.4 implements metadata registration, lifecycle state tracking, a
creator-based factory, and an SDK-free Unitree G1 plugin skeleton. It does not
implement package discovery, binary loading, SDK communication, physical robot
control, or unloading.

## Registration Mechanism

`humanoid::plugins::PluginRegistry` implements `IPluginRegistrar`.

Registry guarantees:

- Thread-safe registration and unregistration.
- Thread-safe metadata lookup.
- Thread-safe lifecycle state updates.
- Snapshot access to registered plugin records.
- No ownership of plugin implementation objects.
- No dynamic loader behavior.

The registry stores metadata and lifecycle state only. `PluginFactory` stores
creator callables and returns plugin instances to the host as
`std::unique_ptr<IPlugin>`. Plugin instances remain owned by the host or future
loader and must be returned to `DestroyPlugin()` for lifecycle-aware shutdown.

## Adding Future Plugins

Future plugin implementation steps:

1. Implement `humanoid::plugins::IPlugin`.
2. Provide complete `PluginMetadata`.
3. Declare a compatible framework API version range.
4. Keep vendor SDK headers inside the plugin implementation or SDK wrapper.
5. Register a creator with `PluginFactory` from the plugin host composition
   root.
6. Register or update lifecycle state through `IPluginRegistrar` during
   `Initialize()`.
7. Return `humanoid::common::Status` for lifecycle failures.
8. Ensure `Stop()` and `Shutdown()` are safe to call during host cleanup.

## Unitree G1 Plugin Skeleton

Milestone 4.4 adds the first concrete plugin package under
`plugins/unitree/g1`.

```text
Plugin host
  -> PluginFactory
    -> UnitreeG1Plugin
      -> UnitreeG1Adapter
        -> mock RobotState
```

The Unitree G1 plugin skeleton provides:

- `humanoid::plugins::unitree::g1::UnitreeG1Plugin`
- `humanoid::plugins::unitree::g1::UnitreeG1Adapter`
- `RegisterUnitreeG1Plugin(PluginFactory&)`
- `plugins/unitree/g1/plugin_manifest.json`

The skeleton is intentionally SDK-free. It returns conservative mock
`RobotState` snapshots, reports `Connect()` as unavailable, and never commands
robot movement. It builds whether `ENABLE_UNITREE` is `ON` or `OFF`.

Milestone 4.8 adds examples that demonstrate static plugin package integration:

- `humanoid_core_plugin_loading_example` registers `UnitreeG1Plugin` through
  `PluginFactory`, creates it through the `IPlugin` boundary, runs lifecycle
  methods, enumerates registry records, and destroys the instance through the
  factory.
- `humanoid_core_framework_integration_example` composes plugin factory,
  Unitree plugin skeleton, `core::RobotAdapter`, `RobotStateManager`, and
  `TelemetryService` in one application-owned composition root.
- `humanoid_core_capability_query_example` creates a plugin-owned adapter and
  queries vendor-independent `RobotCapabilities`.

These examples use the existing static registration helper and do not introduce
dynamic shared-library loading or a new plugin host architecture.

## Unitree SDK Abstraction Layer

Milestone 4.5 adds the internal Unitree SDK abstraction under
`plugins/unitree/sdk`.

```text
UnitreeG1Adapter
  -> LocoClientWrapper
    -> SdkWrapper
      -> LocoAdapter / HandAdapter / AudioAdapter
      -> Unitree SDK2
```

Only implementation files under `plugins/unitree/sdk/` are allowed to include
Unitree SDK2 headers. `SdkWrapper` wraps SDK initialization, shutdown,
discovery, connection, disconnection, read-only communication monitoring, state
synchronization, and command delegation. `LocoAdapter`, `HandAdapter`, and
`AudioAdapter` translate SDK-boundary command types into SDK2 locomotion, arm
action, and audio calls. `SdkTypes.h` defines framework-owned normalized SDK
boundary types, and `SdkConverter` converts those types into framework
`Result`, adapter connection state, and `humanoid::core::RobotState`.

`LocoClientWrapper` remains as the existing adapter-facing facade so Milestone 2
adapter code does not expose SDK details or change public behavior.

Milestone 4.8 also adds `humanoid_core_unitree_sdk_boundary_example` when the
SDK abstraction target is available. The example validates command-adapter error
paths for `LocoAdapter`, `HandAdapter`, `AudioAdapter`, and `SdkWrapper`
without opening robot communication or issuing physical commands.

## Forbidden Dependencies

- Core framework modules depending on concrete plugins.
- `humanoid::humanoid_core` linking to concrete plugins.
- Vendor SDK types in plugin infrastructure headers.
- Global plugin registries or singletons.
- Applications directly depending on vendor SDK headers.
- Plugin lifecycle methods throwing exceptions across the host boundary.

## Validation

Milestone 4.3 keeps the always-built
`humanoid_core_plugin_registry_unit_test` CTest target. It validates:

- Plugin metadata validation.
- Version compatibility range handling.
- Duplicate registration rejection.
- Lifecycle state updates.
- Registration through `IPluginRegistrar`.
- Concurrent registry registration.
- Lifecycle state string conversion.

Milestone 4.3 also adds the always-built
`humanoid_core_plugin_factory_unit_test` CTest target. It validates:

- Creator registration and duplicate rejection.
- Plugin creation and lifecycle transition to `Loaded`.
- Lifecycle-aware destruction and transition to `Shutdown`.
- Unregistration rejection while plugin instances are active.
- Null and mismatched creator rejection.
- Concurrent create/destroy operations.

Milestone 4.4 adds the always-built
`humanoid_core_unitree_g1_plugin_skeleton_test` CTest target. It validates:

- Unitree G1 plugin metadata and manifest path.
- Factory registration through `RegisterUnitreeG1Plugin`.
- Plugin lifecycle transition to `Initialized`.
- Adapter creation through the plugin instance.
- SDK-free adapter capabilities.
- Conservative mock robot state.

Milestone 4.5 adds `humanoid_core_unitree_sdk_converter_test` when Unitree SDK2
is enabled. It validates normalized SDK result, connection state, string, and
robot state conversions without connecting to physical hardware.

Milestone 4.6 extends the same SDK abstraction with read-only communication
contracts. The test now also validates conservative communication type defaults
without requiring physical hardware. Physical online and reconnect validation
must be performed on a Unitree G1 EDU network because the worker intentionally
uses the official SDK transport.
