# Telemetry Service

`humanoid::services::TelemetryService` periodically reads
`humanoid::core::RobotStateManager` and publishes the latest
`humanoid::core::RobotState` snapshot to subscribed callbacks.

The service is vendor independent. It does not include robot adapter headers,
vendor SDK headers, networking middleware, mission logic, AI, planning, or
visualization dependencies.

## Responsibilities

- Read the latest state from `RobotStateManager`.
- Publish snapshots at a configured `std::chrono` polling interval.
- Support multiple callback listeners.
- Allow listeners to unsubscribe by subscription identifier.
- Stop the worker thread before destruction.

## Threading

The service uses `std::jthread` for the worker and sleeps on a condition
variable between samples. It does not busy wait.

Lifecycle operations are serialized by an internal mutex. Listener storage is
protected by `std::shared_mutex`. Publication copies the current listener list
under a shared lock, then invokes callbacks outside service locks.

Callbacks must return promptly. Long-running listener work should be delegated
by the listener to its own execution context.

## Integration

The service is intended to receive the same `RobotStateManager` instance that is
stored in `humanoid::core::CoreContext`. This keeps state publication explicit
and testable:

```cpp
auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
humanoid::core::CoreContext context;
context.setRobotStateManager(state_manager);

humanoid::services::TelemetryService telemetry{context.robotStateManager()};
```

Applications own service lifetime and decide when to call `Start()` and
`Stop()`. The framework does not create hidden background threads.

## Validation

Telemetry behavior is covered by `humanoid_core_robot_state_unit_test`. The test
starts the service with a short polling interval, verifies delivery to multiple
listeners, verifies unsubscribe behavior, and checks that `Start()` fails
gracefully when no state manager is injected.
