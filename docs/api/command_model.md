# Generic Command Model

Milestone 5.1 defines the vendor-independent value types used to describe robot
commands and their processing outcomes. It does not add command scheduling,
execution, dispatch, or adapter behavior.

## Public Headers

```cpp
#include <humanoid/core/Command.h>
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
