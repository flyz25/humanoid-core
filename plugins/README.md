# Plugins Module

The plugins module defines the vendor-independent plugin infrastructure for
humanoid-core. It contains only host contracts, metadata types, version
compatibility rules, and a thread-safe registration registry.

Milestone 4.1 intentionally does not implement vendor plugins or dynamic
shared-library loading.

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

## Dependency Rule

The plugin infrastructure depends on `humanoid::common` only. The core framework
target `humanoid::humanoid_core` does not link against `humanoid::plugins`, and
no core module depends on plugin implementations.

Applications or host executables that need plugin infrastructure should link it
explicitly:

```cmake
target_link_libraries(my_host PRIVATE humanoid::plugins)
```

## Lifecycle

The plugin lifecycle is:

```text
Discovered -> Registered -> Loaded -> Initialized -> Started -> Stopped -> Shutdown
```

Failure is represented by `PluginLifecycleState::kFailed`.

## Loading Architecture

Future loaders will discover plugin metadata, validate version compatibility,
load a plugin package or shared library, create a plugin instance, call
`Initialize()`, and let the plugin register itself through `IPluginRegistrar`.

Milestone 4.1 provides the interfaces and registry only. No platform-specific
`dlopen`, `LoadLibrary`, manifest parser, vendor adapter plugin, or robot
communication plugin is implemented.

## Documentation

See `docs/architecture/plugin_architecture.md` for the full plugin architecture
document.
