# Thread-Safety Guarantees

This document defines the thread-safety expectations for the current
humanoid-core skeleton.

## Summary

| Component | Guarantee |
| --- | --- |
| `RobotFactoryRegistry` | Thread-safe registration, lookup, factory count, and vendor listing. |
| `PluginRegistry` | Thread-safe plugin registration, unregistration, lifecycle updates, metadata lookup, and registry snapshots. |
| `PluginFactory` | Thread-safe plugin creator registration, unregistration, creation, destruction, enumeration, and active-instance accounting. |
| `LoggerManager` | Thread-safe sink registration, sink clearing, severity updates, and logging calls. |
| `RobotStateManager` | Thread-safe state updates, resets, snapshots, and scalar field reads. |
| `TelemetryService` | Thread-safe start, stop, subscribe, and unsubscribe operations. Listener callbacks are invoked outside service locks. |
| `LocoClientWrapper` | Thread-safe public methods through internal serialization of SDK access. |
| `UnitreeG1Adapter` | Thread-safe public methods through adapter-level serialization. |
| `UnitreeRobotFactory` | Stateless; safe to share between threads. |
| Managers in `robot`, `motion`, `gesture`, `safety`, `diagnostics`, and `configuration` | Not internally synchronized; require external synchronization if mutated or queried concurrently. |
| Value types and enums | Safe to copy and read concurrently after construction. |

## Managers

The Milestone 1 managers are lightweight dependency holders. They are intended
for composition-time wiring and single-threaded ownership unless a downstream
application provides external synchronization.

`CoreContext` follows the same composition-time rule for replacing injected
dependencies. The injected `RobotStateManager` remains thread-safe after it is
published through the context.

Examples that require external synchronization:

- Concurrent `setController()` and `startGesture()` on `GestureManager`.
- Concurrent `setConfiguration()` and `value()` on `ConfigManager`.
- Concurrent `setRobot()` and `activate()` on `RobotManager`.

`RobotStateManager` is an exception to this composition-time-manager rule. It
is a runtime latest-state cache and protects its state with `std::shared_mutex`.
Writers use `std::unique_lock`; readers use `std::shared_lock`.

## Services

`TelemetryService` owns a `std::jthread` while running and sleeps on a condition
variable between state samples. `Stop()` requests cooperative cancellation and
wakes the worker, so shutdown is not tied to the full polling interval.

Subscriber storage is protected separately from lifecycle state. The service
copies the current listener callbacks under a shared lock and invokes callbacks
after releasing internal locks. A listener may receive one final copied state
after `Unsubscribe()` if publication was already in progress.

## Registry

`RobotFactoryRegistry` protects its factory collection with a mutex. Returned
factory instances are held by `std::shared_ptr`; factory implementations must
preserve their own thread-safety guarantees.

`PluginRegistry` protects metadata and lifecycle state with `std::shared_mutex`.
Registration, unregistration, and lifecycle updates use exclusive access.
Metadata lookup, lifecycle lookup, registry snapshots, and count reads use
shared access. The registry does not own plugin implementation objects.

`PluginFactory` protects creator storage and active-instance accounting with
`std::shared_mutex`. Creator registration, unregistration, and instance-count
updates use exclusive access. Enumeration is delegated to the injected
`PluginRegistry`. Plugin creator callbacks and plugin `Shutdown()` calls are not
invoked while the factory lock is held. Instance accounting includes plugin
creation that is already in progress, so unregistration is rejected until
in-flight creation or destruction has completed.

## Logging

`LoggerManager` protects sink collection and minimum severity state. Logging
copies the current sink list before calling sinks so sink callbacks do not run
under the manager lock. Individual `LogSink` implementations are responsible
for their own thread-safety.

## SDK Wrapper and Adapter

`LocoClientWrapper` serializes SDK calls. `UnitreeG1Adapter` serializes adapter
state changes and wrapper access. This prevents concurrent command interleaving
inside one adapter instance.

The Unitree SDK may own process-level transport state internally. Applications
should avoid creating multiple active Unitree SDK wrapper instances for the same
robot interface unless the SDK vendor documents that use case as safe.
