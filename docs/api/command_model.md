# Generic Command Model

Milestone 5.1 defines the vendor-independent value types used to describe robot
commands and their processing outcomes. It does not add command scheduling,
execution, dispatch, or adapter behavior.

## Public Headers

```cpp
#include <humanoid/core/Command.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandPriority.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/CommandStatus.h>
#include <humanoid/core/CommandType.h>
```

All types are in `humanoid::core` and are also available through the
`<humanoid/core.hpp>` umbrella header.

## Command

`Command` is a framework-owned value type containing:

- A producer-assigned 64-bit command ID. Zero means unassigned.
- A monotonic `std::chrono::steady_clock` timestamp.
- A vendor-independent `CommandType`.
- A `CommandPriority` scheduling hint.
- A millisecond timeout. Zero disables timeout enforcement.
- Named operational payload values.
- String metadata for correlation and tracing.

Command producers are responsible for assigning a unique nonzero ID within
their command-processing domain and recording the creation timestamp. The core
model intentionally does not use a global ID generator or wall-clock time.

`Command::isValid()` checks that the ID is assigned and the timeout is not
negative. It does not perform command-specific payload validation. That policy
belongs to the future component accepting a command.

## Payload and Metadata

`CommandPayload` is an ordered map from parameter names to one of the following
framework-owned scalar types:

- `bool`
- `std::int64_t`
- `double`
- `std::string`

The ordered representation gives deterministic iteration for diagnostics,
serialization boundaries, and tests. Parameter names and units must be defined
by framework-level command contracts. SDK objects, vendor enums, transport
handles, and pointers are forbidden.

`CommandMetadata` is an ordered string map intended for correlation, tracing,
and source annotations. Operational parameters must remain in the payload so
metadata cannot silently change command behavior.

## Lifecycle and Result

`CommandStatus` represents the lifecycle from `Pending`, `Queued`, and
`Running` through terminal outcomes. `Completed` is the only successful terminal
state. `Cancelled`, `Failed`, `Timeout`, and `Rejected` are unsuccessful terminal
states.

`CommandResult` contains a status and optional diagnostic message. Its
`isSuccess()` and `isTerminal()` queries are `constexpr` and do not throw.

`CommandPriority` expresses relative scheduling importance. `Critical` does not
bypass validation, authorization, capability checks, or robot safety policy.

## Dependency Boundary

```text
Application and future command services
  -> humanoid::core command model
    -> C++ standard library only
```

The model contains no Unitree SDK headers, vendor implementations, robot
communication, or execution logic. Future adapters may consume validated
commands, but vendor types must remain behind their SDK boundary.

## Command Dispatcher

`CommandDispatcher` receives a dependency-injected
`std::shared_ptr<humanoid::adapters::IRobotAdapter>`. It validates generic
commands and forwards supported operations through that interface. It never
includes an SDK header or depends on a concrete adapter.

```cpp
auto dispatcher = humanoid::core::CommandDispatcher{adapter};

humanoid::core::Command stop;
stop.id = 1;
stop.timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now());
stop.type = humanoid::core::CommandType::Stop;

const humanoid::core::CommandResult result = dispatcher.Execute(stop);
```

The dispatcher exposes:

- `Execute(const Command&)` for synchronous execution on the calling thread.
- `ExecuteAsync(Command)` for priority-aware queued execution through a
  `std::future<CommandResult>`.
- `Cancel(CommandId)` for commands that remain queued.
- `Shutdown()` to reject new work, cancel queued work, and wait for active
  adapter calls.

`Shutdown()` controls dispatcher resources only. Adapter lifecycle remains
owned by the application composition root, so dispatcher shutdown does not call
`IRobotAdapter::Shutdown()`.

### Adapter Mapping

The current `IRobotAdapter` contract supports the following mappings:

| Command type | Adapter operation | Required payload |
| --- | --- | --- |
| `Stand` | `StandUp()` | Empty |
| `Walk` | `Move(vx, vy, omega)` | `linear_x`, `linear_y`, `angular_z` |
| `Move` | `Move(vx, vy, omega)` | `linear_x`, `linear_y`, `angular_z` |
| `Rotate` | `Move(0, 0, omega)` | `angular_z` |
| `Stop` | `Stop()` | Empty |

Velocity payload values must be finite `double` or `std::int64_t` values that
fit in the adapter's `float` parameter range. Capability limits remain the
adapter's responsibility.

`Sit`, hand, audio, and custom commands are rejected because the current
adapter interface has no corresponding operation. The dispatcher does not fake
support or call vendor APIs directly.

### Execution Semantics

Synchronous and asynchronous adapter calls are serialized. The asynchronous
queue selects the highest command priority and preserves FIFO order among
commands with equal priority. Priority does not preempt an adapter call that has
already started and never bypasses validation or robot safety policy.

Command IDs must be nonzero and unique among in-flight commands. A positive
timeout requires a valid monotonic creation timestamp. Expired commands return
`CommandStatus::Timeout` before adapter execution; operations that finish after
their deadline also report timeout.

Cancellation is deterministic for queued commands. Running and synchronous
commands cannot be interrupted because `IRobotAdapter` has no cancellation
contract; cancellation attempts for them return `CommandStatus::Rejected`.
Adapter exceptions are contained and translated to `CommandStatus::Failed`.
