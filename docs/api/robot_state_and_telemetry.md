# Robot State and Telemetry API

Milestone 3 introduces the vendor-independent state and telemetry surface used
by adapters, managers, diagnostics, and application composition roots.

## RobotState

Header:

```cpp
#include <humanoid/core/RobotState.hpp>
```

`humanoid::core::RobotState` is a standard-layout, trivially copyable value
type. It contains no vendor SDK type and performs no dynamic allocation.

State groups:

- `connection`: communication status.
- `power`: battery percentage and charging state.
- `motion`: standing, walking, and sitting posture flags.
- `velocity`: linear X, linear Y, and angular Z velocity.
- `pose`: X, Y, and Z position.
- `orientation`: roll, pitch, and yaw angles.
- `health`: emergency stop and normalized fault code.
- `timestamp`: monotonic `std::chrono::steady_clock` timestamp.

The default constructor initializes all booleans to `false`, all numeric fields
to zero, and the timestamp to the default time point.

## RobotStateManager

Header:

```cpp
#include <humanoid/core/RobotStateManager.hpp>
```

`humanoid::core::RobotStateManager` owns the latest `RobotState` snapshot. It is
the synchronization boundary between state producers and state consumers.

Public API:

- `UpdateState(const RobotState&)`: replace the current snapshot.
- `GetState() const`: return a copied snapshot.
- `Reset()`: restore the default snapshot.
- `IsConnected() const`: read the connection flag.
- `BatteryLevel() const`: read battery percentage.
- `EmergencyStop() const`: read emergency-stop state.
- `FaultCode() const`: read normalized fault code.

Threading:

- Writers use exclusive locking.
- Readers use shared locking.
- The manager does not perform I/O, SDK calls, or dynamic allocation.

## TelemetryService

Header:

```cpp
#include <humanoid/services/TelemetryService.h>
```

`humanoid::services::TelemetryService` periodically reads a shared
`RobotStateManager` and publishes copied `RobotState` snapshots to subscribers.

Public API:

- `Start()`: start the worker thread.
- `Stop()`: request cooperative stop and release the worker.
- `Subscribe(Listener)`: register a callback.
- `Unsubscribe(SubscriptionId)`: remove a callback.

Threading:

- The worker uses `std::jthread`.
- The service sleeps on a condition variable and does not busy wait.
- Listener storage is protected by `std::shared_mutex`.
- Callbacks are invoked outside service locks.

## Composition

Applications inject the state manager through their composition root:

```cpp
auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
humanoid::core::CoreContext context;
context.setRobotStateManager(state_manager);

humanoid::services::TelemetryService telemetry{context.robotStateManager()};
```

This preserves the existing no-singleton and no-global-state architecture.

## Test Coverage

Milestone 3 state and telemetry behavior is covered by the always-built
`humanoid_core_robot_state_unit_test` CTest target. The test verifies:

- `RobotState` default values and allocation-free type traits.
- Full-state update, scalar accessors, and reset behavior.
- Concurrent `RobotStateManager` readers and writers.
- Subscriber callback delivery to multiple telemetry listeners.
- Unsubscribe behavior.
- Invalid telemetry start behavior when no state manager is provided.
- A bounded state update/read performance sanity check.
