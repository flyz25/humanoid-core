# Milestone 3 Report

Milestone 3 adds the vendor-independent robot state and telemetry foundation.
It does not introduce robot communication, AI, mission execution, planning,
navigation, GUI code, ROS2, or behavior trees.

## Scope

Completed milestones:

- Milestone 3.1: `RobotState` value model.
- Milestone 3.2: thread-safe `RobotStateManager`.
- Milestone 3.3: `TelemetryService` callback publisher.
- Milestone 3.4: framework integration through `CoreContext`.
- Milestone 3.5: testing, API documentation, architecture documentation, and
  final validation.

## Architecture

The Milestone 3 dependency path is:

```text
Robot adapter or state producer
  -> RobotState
    -> RobotStateManager
      -> TelemetryService
        -> Subscriber callbacks
```

The path remains vendor independent. No Unitree SDK2 headers, SDK types, or
adapter implementation details appear in the state model, state manager, or
telemetry service.

## API Surface

Public state and telemetry API:

- `humanoid::core::RobotState`
- `humanoid::core::RobotStateManager`
- `humanoid::services::TelemetryService`

Documentation:

- `docs/api/robot_state_and_telemetry.md`
- `docs/services/telemetry_service.md`
- `core/docs/core.md`

## Testing

Milestone 3 introduces the always-built
`humanoid_core_robot_state_unit_test` CTest target. It verifies:

- `RobotState` default values.
- `RobotState` standard-layout and trivially-copyable type properties.
- State update, snapshot, scalar accessor, and reset behavior.
- Concurrent `RobotStateManager` readers and writers.
- Telemetry callback delivery to multiple subscribers.
- Telemetry unsubscribe behavior.
- Graceful telemetry start failure when no state manager is provided.
- A bounded state-manager update/read performance sanity check.

The existing `humanoid_core_smoke_test` continues to verify factory, registry,
adapter mock, and integrated telemetry construction behavior.

## Performance

The milestone does not introduce a benchmark suite. The CTest unit executable
includes a bounded performance sanity check for repeated `RobotStateManager`
update/read cycles. This protects against accidental pathological overhead while
avoiding fragile microbenchmark assertions in CI.

## Thread Safety

`RobotStateManager` protects state with `std::shared_mutex`. Writes use
exclusive locking; snapshots and scalar reads use shared locking.

`TelemetryService` serializes lifecycle operations, protects listener storage
with `std::shared_mutex`, sleeps on a condition variable, and invokes callbacks
outside service locks.

## Validation

Validation performed for Milestone 3.5:

| Configuration | Result |
| --- | --- |
| Debug, `ENABLE_UNITREE=OFF` | Passed |
| Release, `ENABLE_UNITREE=OFF` | Passed |
| Debug, `ENABLE_UNITREE=ON` | Passed |
| Release, `ENABLE_UNITREE=ON` | Passed |
| CTest | Passed |
| Install/export package discovery | Passed |

GoogleTest is optional. In this environment, GoogleTest was not installed, so
the optional GoogleTest target was not built. The always-built CTest targets
provide the Milestone 3 validation coverage.

## Known Limitations

- Telemetry callbacks execute in the telemetry worker thread. Long-running
  listener work must be delegated by the listener.
- The performance check is a CI sanity check, not a hardware or real-time
  performance benchmark.
- `RobotState` is intentionally generic. Vendor-specific state expansion must
  remain outside the core state model or be normalized into vendor-independent
  fields.

## Completion Status

Milestone 3 is complete. The repository now contains the generic state model,
thread-safe state manager, telemetry publication service, framework integration,
tests, documentation, and validation artifacts required before Milestone 4.
