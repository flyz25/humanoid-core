# Core Module

The core module exposes package-level runtime metadata, the core application
context, and vendor-independent data models such as `humanoid::core::RobotState`.
It stores only framework interfaces and injectable shared services. It does not
depend on robot SDKs, middleware, perception, mission execution, GUI code, or
application logic.

Applications should depend on core interfaces and managers. Robot-specific
implementation details must stay behind adapter boundaries.

## Robot State Model

`humanoid::core::RobotState` is the canonical generic state snapshot for future
robot implementations. It contains connection, power, high-level motion,
velocity, pose, orientation, health, and timestamp fields. The model contains no
vendor SDK types, no dynamic allocation, and no robot-specific implementation
details.

The timestamp uses a monotonic `std::chrono` clock so state consumers are not
affected by wall-clock adjustments.

## Robot State Manager

`humanoid::core::RobotStateManager` maintains the latest
`humanoid::core::RobotState` snapshot behind an internal `std::shared_mutex`.
State updates and resets use `std::unique_lock`; snapshot and field reads use
`std::shared_lock`.

The manager is vendor independent, performs no heap allocation or I/O, and is
intended for unit tests, adapters, managers, and diagnostics that need a
thread-safe latest-state cache.

## Context Integration

`humanoid::core::CoreContext` can hold an injected `RobotStateManager`.
Applications and services should receive this dependency explicitly from their
composition root. `humanoid::services::TelemetryService` is constructed from the
same injected state manager, which keeps telemetry publication wired to the
framework state cache without introducing singleton or global state.

## API Documentation

The public Milestone 3 API is documented in
`docs/api/robot_state_and_telemetry.md`. The always-built
`humanoid_core_robot_state_unit_test` target validates state defaults, full
state updates, reset behavior, concurrent reads and writes, telemetry callback
delivery, unsubscribe behavior, invalid telemetry startup, and a bounded
state-manager performance sanity check.
