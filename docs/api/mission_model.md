# Mission Model

Milestone 6.1 defines the vendor-independent mission value model. Milestone 6.2
adds a thread-safe executor that runs mission steps through the existing command
framework. This API does not add a scheduler, parser, robot adapter, planner,
behavior tree, or vendor integration.

## Public Headers

```cpp
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionLoader.h>
#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/MissionParser.h>
#include <humanoid/mission/MissionResult.h>
#include <humanoid/mission/MissionStatus.h>
#include <humanoid/mission/MissionStep.h>
#include <humanoid/mission/MissionValidator.h>
```

All types are in `humanoid::mission` and are also available through the
`<humanoid/core.hpp>` umbrella header.

## Mission

`Mission` is a framework-owned value type containing:

- A producer-assigned 64-bit mission ID. Zero means unassigned.
- Human-readable name, description, version, and author fields.
- A monotonic `std::chrono::steady_clock` timestamp.
- An ordered list of `MissionStep` values.
- Non-operational metadata for labels, correlation, and authoring context.

The model intentionally does not generate IDs or use wall-clock time. Mission
authoring tools or application code own identity and timestamp assignment.

`Mission::isValid()` verifies that the mission ID is nonzero and at least one
enabled step is valid. Disabled steps may remain in the model for authoring
workflows and do not block mission validity.

## Mission Step

`MissionStep` contains:

- A mission-local 64-bit step ID. Zero means unassigned.
- A human-readable step name.
- A vendor-independent `humanoid::core::Command`.
- A `std::chrono::milliseconds` timeout.
- A retry count.
- An enabled flag.
- Step metadata.

`MissionStep::isValid()` requires a nonzero step ID, a valid embedded command,
and a nonnegative timeout. A zero timeout disables step timeout policy.

Retry policy is declarative only in Milestone 6.1. No retry execution behavior
is implemented by the mission model.

## Status and Result

`MissionStatus` represents the lifecycle from `Pending`, `Running`, and
`Paused` through terminal outcomes. `Completed` is the only successful terminal
state. `Failed` and `Cancelled` are unsuccessful terminal states.

`MissionResult` contains a status and optional diagnostic message. Its
`isSuccess()` and `isTerminal()` queries are `constexpr` and do not throw.

## Metadata

`MissionMetadata` is an ordered string map intended for labels, correlation,
tracing, and authoring context. Metadata must not contain SDK objects, robot
handles, credentials, or values that silently alter command behavior.

Operational parameters belong in the embedded `Command` payload.

## Dependency Boundary

```text
Application or future mission service
  -> humanoid::mission model
    -> humanoid::core::Command
      -> C++ standard library only
```

The mission value model contains no Unitree SDK headers, robot communication,
execution engine, behavior tree, planner, navigation stack, YAML parser, or
state machine. Mission execution components may consume this model, but vendor
types must remain behind adapter and SDK boundaries.

## Mission Executor

Milestone 6.2 adds `MissionExecutor`, a thread-safe component for executing a
mission's enabled steps through an injected `humanoid::core::CommandDispatcher`.

```text
MissionExecutor
  -> Mission
    -> MissionStep
      -> CommandDispatcher
        -> Command Framework
```

The executor provides:

- `Start()` for asynchronous mission execution.
- `Pause()` to pause before the next step or retry starts.
- `Resume()` to continue a paused mission.
- `Cancel()` to request cancellation without waiting for the worker to join.
- `Stop()` to request cancellation and wait for executor-owned work to finish.
- `ExecuteStep()` for synchronous single-step execution through the dispatcher.
- Status, current step index, current step ID, and last result snapshots.

Pause, cancel, and stop do not interrupt a command already executing inside the
dispatcher because the command and adapter contracts do not expose cooperative
interruption. Queued dispatcher work is cancelled when the dispatcher can still
cancel it; active adapter work is allowed to finish.

The executor owns no robot adapter, SDK client, parser, planner, behavior tree,
or mission authoring state. It is dependency-injection friendly and has no
singleton or global state.

## Mission Loading

Milestone 6.3 adds file-format conversion and schema validation:

```text
Mission file (.json/.yaml/.yml)
  -> MissionLoader
    -> MissionParser
    -> MissionValidator
      -> Mission
```

`MissionLoader` reads mission files, dispatches by extension, and validates the
parsed mission before returning it. `MissionParser` supports the strict
humanoid-core mission JSON schema and a deterministic YAML subset for the same
schema. `MissionValidator` validates the in-memory mission model independently
from file parsing.

`MissionExecutor` does not include or depend on YAML, JSON, file I/O, or parser
types. Applications load a mission first, then pass the returned `Mission` to
the executor.

Required mission document fields:

- `id`
- `name`
- `description`
- `version`
- `author`
- `steps`

Required enabled step fields:

- `id`
- `name`
- `command.id`
- `command.type`

Optional step and command fields:

- `timeout_ms`
- `retry`
- `enabled`
- `priority`
- `payload`
- `metadata`

Unknown command names, invalid syntax, missing required fields, invalid scalar
types, negative timeouts, and empty enabled-step sets are rejected before a
mission reaches `MissionExecutor`.
