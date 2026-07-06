# Test Contract Repair Report

## Executive Summary

The Unitree G1 plugin contract test has been aligned with the production SDK
integration contract. Default CTest execution is deterministic offline and does
not require a Unitree G1, Ethernet connectivity, DDS discovery, or physical
hardware.

The production SDK integration remains active. The repair changes only error
classification and CTest metadata:

- Non-zero Unitree SDK return codes now use the failure category supplied by the
  adapter operation.
- `humanoid_core_unitree_g1_plugin_skeleton_test` is explicitly labeled as an
  offline SDK integration test.
- The test remains enabled in default CTest and retains its meaningful contract
  assertions.

## Problem Description

`humanoid_core_unitree_g1_plugin_skeleton_test` could fail with:

```text
Unitree G1 adapter production contract: connect failed with an unexpected status
```

The diagnostic report identified that the test accepts `kOk`, `kUnavailable`,
and `kFailedPrecondition` for offline connection attempts, but the production SDK
path could return `kInternalError` when a Unitree SDK command returned a non-zero
integer status.

## Root Cause

The SDK support helper mapped every non-zero Unitree SDK integer return value to
`SdkErrorCode::kUnknown`. `UnitreeG1Adapter` then translated
`SdkErrorCode::kUnknown` to `StatusCode::kInternalError`.

For connection probing, that was too broad. A failed `GetFsmId()` return code is
a communication failure, not an internal programming failure. It should preserve
the operation's requested error category, which is `kConnectionFailed` for
connection checks and maps to framework `kUnavailable`.

## Files Modified

- `plugins/unitree/sdk/SdkClientSupport.h`
- `tests/CMakeLists.txt`
- `CHANGELOG.md`
- `docs/Test_Contract_Repair_Report.md`

The existing diagnostic artifact remains available as
`Unitree_Plugin_Test_Diagnostic_Report.md`.

## Reasoning

`internal::InvokeSdkCommand()` already receives an operation-specific
`SdkErrorCode`. Before this repair, that category was used only for exceptions,
while non-zero SDK integer returns were collapsed into `kUnknown`.

The repaired behavior treats non-zero SDK return codes and SDK exceptions
consistently:

- Connection operations continue to report `kConnectionFailed`.
- Robot command operations continue to report `kRobotFault`.
- Internal failures remain available through callers that explicitly use
  `kUnknown`.

This preserves production signal while making offline default tests
deterministic.

## Test Classification Decision

`humanoid_core_unitree_g1_plugin_skeleton_test` is classified as:

```text
SDK Integration Test, offline-safe
```

It is not a pure unit test because it constructs the production
`UnitreeG1Adapter` and links `humanoid::unitree_g1_plugin`. With Unitree enabled,
it exercises the real SDK abstraction boundary.

It is not a hardware validation test because it does not require a robot and
accepts offline statuses for unavailable SDK transport or absent robot
communication.

CTest labels added:

```text
integration;sdk;unitree;offline
```

## SDK Status Mapping Review

Status usage after repair:

- `StatusCode::kOk`: operation completed successfully.
- `StatusCode::kUnavailable`: SDK transport, DDS communication, network
  interface, robot presence, or connection verification is unavailable.
- `StatusCode::kFailedPrecondition`: adapter lifecycle precondition failed, or
  robot state/fault prevents a command.
- `StatusCode::kInternalError`: reserved for genuine internal failures or
  callers that explicitly classify a failure as unknown.

`SdkErrorCode::kUnknown` is no longer the default mapping for all non-zero SDK
integer return codes. This avoids treating normal offline connection failures as
internal programming defects.

## Regression Results

Configured and built:

| Build directory | Type | Unitree | ROS2 | Cloud | Result |
| --- | --- | --- | --- | --- | --- |
| `build-test-contract-debug-off` | Debug | OFF | OFF | OFF | Passed |
| `build-test-contract-debug-on` | Debug | ON | OFF | OFF | Passed |
| `build-test-contract-release-off` | Release | OFF | OFF | OFF | Passed |
| `build-test-contract-release-on` | Release | ON | OFF | OFF | Passed |
| `build-test-contract-debug-default` | Debug | ON | default | default | Passed |
| `build-test-contract-release-default` | Release | ON | default | default | Passed |

## CTest Results

| Build directory | Tests | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: | ---: |
| `build-test-contract-debug-off` | 35 | 35 | 0 | 0 |
| `build-test-contract-debug-on` | 37 | 37 | 0 | 0 |
| `build-test-contract-release-off` | 35 | 35 | 0 | 0 |
| `build-test-contract-release-on` | 37 | 37 | 0 | 0 |
| `build-test-contract-debug-default` | 38 | 38 | 0 | 0 |
| `build-test-contract-release-default` | 38 | 38 | 0 | 0 |

The default `38/38` count includes the optional cloud platform unit test. When
`ENABLE_CLOUD=OFF`, the suite contains fewer tests by design.

## Remaining Known Limitations

- The repaired test validates the offline-safe SDK integration contract. It does
  not prove that a physical Unitree G1 is reachable.
- Real hardware connection, DDS discovery, robot heartbeat, and command
  execution remain hardware validation responsibilities.
- `GoogleTest` was not found during configure, so optional GoogleTest-based
  tests were not built in this environment.

## Framework Bring-up Readiness

The repository is framework bring-up ready for Unitree G1 from an automated test
contract perspective:

- Default CTest passes offline.
- The Unitree SDK abstraction remains linked when available.
- Connection failures caused by absent robot/DDS/network are reported as
  availability failures instead of internal programming errors.
- Hardware validation is still required before claiming physical robot
  operation.

