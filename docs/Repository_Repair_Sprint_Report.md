# Repository Repair Sprint Report

Date: 2026-07-06

## Executive Summary

The repair sprint converted the repository from architecture-complete into a
hardware bring-up ready state for Unitree G1 communication attempts. The sprint
did not add new framework layers. Repairs focused on verified implementation
gaps: duplicate robot adapter abstractions, incomplete Unitree G1 plugin
routing, incomplete command coverage, incomplete SDK state synchronization, and
CMake dependency inconsistencies.

The framework can now attempt real Unitree G1 communication through the
existing SDK abstraction when `ENABLE_UNITREE=ON`, Unitree SDK2 is available,
and the operator runs a physical-communication example with `--execute`.
No physical robot validation was executed or claimed during this sprint.

## Issues Found

- Two public robot adapter abstractions existed: `humanoid::core::RobotAdapter`
  and `humanoid::adapters::IRobotAdapter`.
- `CommandDispatcher` still depended on legacy adapter command methods instead
  of one unified command execution boundary.
- Unitree G1 plugin adapter was still skeleton-oriented and did not delegate to
  the SDK abstraction.
- Unitree plugin documentation and tests still described a skeleton plugin.
- Declared command types were broader than dispatcher and adapter routing.
- SDK state synchronization covered only limited connection and motion data.
- `humanoid::core` still linked back to `adapter_interfaces`, creating an
  unnecessary dependency direction after adapter unification.
- Unitree SDK2 headers failed under GCC 11 C++20 mode, blocking
  `ENABLE_UNITREE=ON` builds.

## Issues Fixed

- Unified the public robot adapter boundary on
  `humanoid::core::RobotAdapter`.
- Converted `humanoid::adapters::IRobotAdapter` into a compatibility alias to
  avoid a second public abstraction.
- Added adapter command capability and `ExecuteCommand()` contracts to the
  unified adapter interface.
- Updated `CommandDispatcher`, safety validation, mission parsing/validation,
  ROS2 command conversion, examples, and tests to use the unified command path.
- Completed Unitree G1 plugin adapter lifecycle, capability reporting, state
  snapshots, command forwarding, SDK error translation, and optional
  `RobotStateManager` synchronization.
- Reused `plugins/unitree/sdk/SdkWrapper`, `LocoAdapter`, `HandAdapter`, and
  `AudioAdapter` instead of duplicating SDK communication.
- Added explicit command routing or explicit rejection for stand, sit, walk,
  stop, emergency stop, velocity, hand, gesture, audio, volume, and custom
  commands.
- Expanded SDK state normalization to include robot/motion mode, IMU, joints,
  contacts, diagnostics, heartbeat/reconnect counters, latency, fault, and
  connection data.
- Fixed CMake dependency direction so `adapter_interfaces` depends on core
  types while `humanoid::core` no longer depends on legacy adapter headers.
- Confined the Unitree SDK C++17 compatibility dialect to the SDK abstraction
  target while preserving C++20 for the public framework.

## Architecture Changes

The architecture remains the same:

```text
Applications
  -> Managers / Factories / Plugins
    -> humanoid::core::RobotAdapter
      -> UnitreeG1Adapter
        -> SDK Wrapper
          -> Unitree SDK2
```

The only architectural correction was removing the duplicate public adapter
abstraction. `IRobotAdapter` remains as a source-compatible alias, not as an
independent interface. Core framework code now routes commands through
`RobotAdapter::ExecuteCommand()` and generic command capabilities.

## Compatibility Notes

- Existing includes of `<humanoid/adapters/IRobotAdapter.h>` continue to compile
  because the header aliases `humanoid::core::RobotAdapter`.
- Public adapter implementations must implement the unified lifecycle, state,
  information, capability, update, command capability, and command execution
  methods.
- The SDK abstraction target uses C++17 internally to compile the official
  Unitree SDK2 headers with GCC 11. This does not change the public C++20 API.
- Finger-level Unitree hand open/close commands remain explicitly unsupported
  because the SDK boundary exposes gesture-level arm actions rather than
  generic finger control.

## Validation

- Debug, `ENABLE_UNITREE=OFF`: configure, build, and CTest passed.
- Debug, `ENABLE_UNITREE=ON`: configure, build, and CTest passed.
- Release, `ENABLE_UNITREE=OFF`: configure, build, and CTest passed.
- Release, `ENABLE_UNITREE=ON`: configure, build, and CTest passed.
- Install validation passed for Release OFF and Release ON into `/tmp`.
- CPack TGZ package generation passed.
- Plugin loading example passed.
- Capability query and framework integration examples exited gracefully in the
  restricted environment with SDK initialization unavailable.
- Robot connection example did not open communication without `--execute`.
- SDK boundary example passed without physical robot communication.

## Remaining Known Limitations

- No physical Unitree G1 was connected during this sprint.
- Hardware validation must be executed by a human operator using the Hardware
  Validation Program.
- WSL2, containers, or restricted sandboxes may not provide route netlink socket
  access or a valid robot network interface; SDK initialization will fail
  gracefully in those environments.
- Real low-state field availability depends on the connected robot firmware and
  Unitree SDK2 runtime behavior.
- Dynamic shared-library plugin loading remains outside the current plugin
  architecture; plugins are built and registered statically.

## Hardware Bring-Up Readiness

Can the framework now attempt real communication with Unitree G1?

Yes, with assumptions:

- `third_party/unitree_sdk2` is initialized and matches the pinned SDK release.
- The operator runs with `ENABLE_UNITREE=ON`.
- The host has a valid network interface configured in `config/robot.yaml`.
- The operator explicitly runs a connection example with `--execute`.
- A Unitree G1 is reachable on the configured network.

Evidence:

- `humanoid_core_unitree_sdk_abstraction` builds and links against Unitree SDK2.
- `humanoid_core_unitree_g1_plugin` builds in both Unitree-enabled and
  Unitree-disabled configurations.
- `UnitreeG1Adapter` delegates to `SdkWrapper` and SDK-boundary adapters.
- `RobotStateManager` can receive normalized state snapshots from the SDK
  callback path.
- All automated tests passed in Debug and Release for Unitree ON and OFF.

No claim is made that a physical Unitree G1 connection succeeded.
