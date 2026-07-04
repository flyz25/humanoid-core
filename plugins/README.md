# Plugins Module

The plugins module defines the vendor-independent plugin infrastructure for
humanoid-core. It contains only host contracts, metadata types, version
compatibility rules, a thread-safe registration registry, and a thread-safe
plugin factory.

Milestone 4.4 adds the SDK-free Unitree G1 plugin skeleton. It intentionally
does not implement dynamic shared-library loading, SDK communication, or robot
motion.

## Provided Contracts

- `humanoid::plugins::IPlugin`: base lifecycle interface implemented by future
  plugins.
- `humanoid::plugins::IPluginLoader`: host-side abstraction for future
  platform-specific loaders.
- `humanoid::plugins::IPluginRegistrar`: registration boundary exposed by the
  host to plugin instances.
- `humanoid::plugins::PluginMetadata`: plugin identity, vendor, version, and
  compatibility metadata.
- `humanoid::plugins::PluginVersionCompatibility`: inclusive framework API
  version range accepted by a plugin.
- `humanoid::plugins::PluginRegistry`: thread-safe metadata and lifecycle
  registry.
- `humanoid::plugins::PluginFactory`: thread-safe creator registry and plugin
  instance factory.
- `humanoid::plugins::unitree::g1::UnitreeG1Plugin`: first concrete plugin
  skeleton package.
- `humanoid::plugins::unitree::g1::UnitreeG1Adapter`: SDK-free adapter skeleton
  returning conservative mock robot state.

## Dependency Rule

The plugin infrastructure target depends on `humanoid::common` only. Concrete
plugin package targets are separate and may depend on plugin infrastructure plus
public framework interfaces. The core framework target
`humanoid::humanoid_core` does not link against `humanoid::plugins`, and no core
module depends on plugin implementations.

Applications or host executables that need plugin infrastructure should link it
explicitly:

```cmake
target_link_libraries(my_host PRIVATE humanoid::plugins)
```

Hosts that use the static Unitree G1 skeleton package should link it explicitly:

```cmake
target_link_libraries(my_host PRIVATE humanoid::unitree_g1_plugin)
```

## Lifecycle

The plugin lifecycle is:

```text
Discovered -> Registered -> Loaded -> Initialized -> Started -> Stopped -> Shutdown
```

Failure is represented by `PluginLifecycleState::kFailed`.

## Loading Architecture

Future loaders will discover plugin metadata, validate version compatibility,
load a plugin package or shared library, register a creator with
`PluginFactory`, create a plugin instance, call `Initialize()`, and let the
plugin update host-visible lifecycle state through `IPluginRegistrar`.

Milestone 4.4 provides the interfaces, registry, factory, and Unitree G1 plugin
skeleton only. No platform-specific `dlopen`, `LoadLibrary`, manifest parser,
SDK communication, or physical robot control is implemented.

Milestone 4.8 adds buildable examples that compose `PluginRegistry`,
`PluginFactory`, and `UnitreeG1Plugin` from an application-owned composition
root. The examples use static registration through `RegisterUnitreeG1Plugin()`;
they do not add dynamic shared-library loading.

## Documentation

See `docs/architecture/plugin_architecture.md` for the full plugin architecture
document.
