# Milestone 4.8 Integration Report

## Scope

Milestone 4.8 completes integration coverage at the application composition
layer. It does not change framework architecture, public runtime behavior, or
plugin dependency direction.

## Changes

- Added `examples/common/ExampleRobotConfig.hpp` for example-only loading of
  the repository `config/robot.yaml` schema.
- Added `humanoid_core_plugin_loading_example` for `PluginRegistry`,
  `PluginFactory`, and `UnitreeG1Plugin` lifecycle wiring.
- Added `humanoid_core_framework_integration_example` for plugin factory,
  Unitree plugin skeleton, `core::RobotAdapter`, `RobotStateManager`, and
  `TelemetryService` composition.
- Added `humanoid_core_robot_connection_example` for factory-based robot
  adapter creation and optional read-only physical connection execution.
- Added `humanoid_core_telemetry_example` for state-manager and telemetry
  callback integration.
- Added `humanoid_core_capability_query_example` for vendor-independent adapter
  capability inspection.
- Added `humanoid_core_unitree_sdk_boundary_example`, built only when the
  Unitree SDK abstraction target exists, for hardware-free validation of
  `SdkWrapper`, `LocoAdapter`, `HandAdapter`, and `AudioAdapter` command
  adapter boundaries.

## Dependency Validation

- Core targets do not depend on concrete plugins.
- Plugin infrastructure still depends only on `humanoid::common`.
- Unitree plugin skeleton remains SDK-free.
- Unitree SDK2 headers remain isolated to implementation files under
  `plugins/unitree/sdk/`.
- SDK-boundary examples are gated on `humanoid::unitree_sdk_abstraction`.
- Unitree-disabled builds continue to compile examples that do not require SDK2.

## Runtime Example Validation

Executed successfully in `build-m48-debug-unitree`:

- `humanoid_core_plugin_loading_example`
- `humanoid_core_framework_integration_example`
- `humanoid_core_capability_query_example`
- `humanoid_core_telemetry_example`
- `humanoid_core_robot_connection_example config/robot.yaml`
- `humanoid_core_robot_connection_example config/robot.yaml --execute`
- `humanoid_core_unitree_sdk_boundary_example`

The physical connection execution path exited gracefully in the local
environment because route netlink socket access is unavailable. No motion,
hand, or audio command was sent by the connection example.

Executed successfully in `build-m48-debug-no-unitree`:

- `humanoid_core_robot_connection_example config/robot.yaml`
- `humanoid_core_plugin_loading_example`

## Build Validation

All builds used `HUMANOID_CORE_WARNINGS_AS_ERRORS=ON`.

| Configuration | Result |
| --- | --- |
| Debug, `ENABLE_UNITREE=ON` | Passed |
| Release, `ENABLE_UNITREE=ON` | Passed |
| Debug, `ENABLE_UNITREE=OFF` | Passed |
| Release, `ENABLE_UNITREE=OFF` | Passed |

## Test Validation

| Configuration | CTest Result |
| --- | --- |
| Debug, `ENABLE_UNITREE=ON` | 8/8 passed |
| Release, `ENABLE_UNITREE=ON` | 8/8 passed |
| Debug, `ENABLE_UNITREE=OFF` | 6/6 passed |
| Release, `ENABLE_UNITREE=OFF` | 6/6 passed |

## Known Limitations

- Dynamic shared-library loading is still intentionally not implemented.
- The Unitree G1 plugin package remains an SDK-free skeleton.
- Physical robot online validation was not performed in this environment.
- Motion, hand, and audio examples validate adapter boundaries and invalid
  command handling only; they do not issue physical robot commands.

## Production Readiness

Milestone 4.8 is complete for composition-layer integration and examples. The
framework remains vendor independent, optional Unitree integration remains
build-gated, and all required validation passed locally.
