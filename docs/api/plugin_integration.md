# Plugin Integration API

Milestone 4 exposes plugin integration through explicit host-owned objects. The
core framework does not create global registries, load plugins implicitly, or
link concrete plugin packages into `humanoid::humanoid_core`.

## Host Composition Flow

```text
Application or plugin host
  -> PluginRegistry
  -> PluginFactory
    -> RegisterUnitreeG1Plugin()
      -> UnitreeG1Plugin
        -> core::RobotAdapter
```

Applications own the composition root. They construct a `PluginRegistry`,
inject it into `PluginFactory`, register plugin creators, create plugin
instances, and call lifecycle methods through `IPlugin`.

## Core Types

- `humanoid::plugins::PluginRegistry`: thread-safe metadata and lifecycle
  registry.
- `humanoid::plugins::PluginFactory`: thread-safe plugin creator registry and
  instance factory.
- `humanoid::plugins::IPlugin`: lifecycle interface for plugin instances.
- `humanoid::plugins::PluginMetadata`: plugin identity, version, compatibility,
  and manifest metadata.
- `humanoid::plugins::PluginLifecycleState`: host-visible plugin lifecycle
  state.
- `humanoid::core::RobotAdapter`: vendor-independent plugin adapter interface
  for lifecycle, connection, state, robot information, capabilities, and update.

## Unitree G1 Plugin Package

`humanoid::plugins::unitree::g1::RegisterUnitreeG1Plugin()` statically
registers the Unitree G1 plugin package with a host-owned `PluginFactory`.

The Unitree G1 plugin public headers intentionally do not include Unitree SDK2
headers or vendor types. The plugin adapter delegates lifecycle, connection,
state synchronization, and command execution to the SDK abstraction when that
target is available; otherwise it reports `Unavailable` statuses cleanly.

## SDK Boundary

Physical Unitree SDK2 integration remains outside the plugin infrastructure API:

```text
core::RobotAdapter
  -> UnitreeG1Adapter
    -> LocoClientWrapper
      -> SdkWrapper
        -> LocoAdapter / HandAdapter / AudioAdapter
          -> Unitree SDK2
```

Only implementation files under `plugins/unitree/sdk/` include Unitree SDK2
headers. The SDK boundary exposes framework-owned `SdkResult`, `SdkRobotState`,
and command payload types.

## Examples

- `humanoid_core_plugin_loading_example`
- `humanoid_core_framework_integration_example`
- `humanoid_core_capability_query_example`
- `humanoid_core_robot_connection_example`
- `humanoid_core_telemetry_example`
- `humanoid_core_unitree_sdk_boundary_example`

The SDK-boundary example is built only when `ENABLE_UNITREE=ON` and the Unitree
SDK2 abstraction target is available.
