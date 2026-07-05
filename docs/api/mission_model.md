# Mission Model

Milestone 6.1 defines the vendor-independent mission value model. Milestone 6.2
adds a thread-safe executor that runs mission steps through the existing command
framework. Milestone 6.3 adds strict JSON/YAML loading. Milestone 6.4 adds
flow-control policies for wait, delay, retry, loop, timeout, skip, and abort.
Milestone 6.5 adds robot-state conditions and mission condition events. This
API does not add a robot adapter, planner, behavior tree, or vendor integration.

## Public Headers

```cpp
#include <humanoid/mission/ConditionEvaluator.h>
#include <humanoid/mission/DelayStep.h>
#include <humanoid/mission/LoopPolicy.h>
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionCondition.h>
#include <humanoid/mission/MissionEvent.h>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionLoader.h>
#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/MissionParser.h>
#include <humanoid/mission/MissionResult.h>
#include <humanoid/mission/MissionStatus.h>
#include <humanoid/mission/MissionStep.h>
#include <humanoid/mission/MissionValidator.h>
#include <humanoid/mission/RetryPolicy.h>
#include <humanoid/mission/TimeoutPolicy.h>
#include <humanoid/mission/WaitStep.h>
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
- A `RetryPolicy`, `LoopPolicy`, and `TimeoutPolicy`.
- Optional `WaitStep` or `DelayStep` flow-control behavior.
- Optional `MissionCondition` values evaluated before step execution.
- Skip and abort flags.
- An enabled flag.
- Step metadata.

`MissionStep::isValid()` requires a nonzero step ID, valid flow-control
policies, and nonnegative timeout values. Command steps require a valid embedded
command. Wait, delay, skip, and abort steps do not require a command because
they do not dispatch robot commands. A zero timeout disables timeout policy.

The legacy `retry` field remains supported. New mission authors should prefer
`RetryPolicy::maxAttempts`, which includes the initial attempt.

## Flow Control

Milestone 6.4 adds vendor-independent flow-control value types:

- `WaitStep`: deterministic duration-based wait before continuing.
- `DelayStep`: deterministic duration-based delay between steps.
- `RetryPolicy`: total attempts, retry delay, and retry conditions.
- `LoopPolicy`: total number of times to execute a step.
- `TimeoutPolicy`: step timeout and abort-on-timeout behavior.

The mission executor applies flow control without bypassing the command
framework. Command steps still execute through `CommandDispatcher`; wait, delay,
skip, and abort steps do not call robot adapters and contain no SDK or robot
logic.

## Events and Conditions

Milestone 6.5 adds condition-aware mission execution:

```text
MissionStep
  -> MissionCondition
  -> ConditionEvaluator
    -> RobotStateManager
```

`MissionCondition` supports:

- Battery level.
- Connection state.
- Standing, walking, and sitting robot state flags.
- Generic command capability checks.
- Fault code checks.
- Emergency-stop checks.

`ConditionEvaluator` reads runtime robot state only through the injected
`humanoid::core::RobotStateManager`. Capability conditions use an injected
generic `CommandCapabilitySet`; the evaluator never calls robot adapters,
plugins, SDK wrappers, or vendor SDKs.

Each condition failure can request `Skip` or `Abort`. The executor evaluates
step conditions before command dispatch or flow-control execution. Skip failures
complete the step without dispatching a command. Abort failures fail the step
and stop mission execution. `MissionEvent` records condition satisfied, failed,
and unavailable outcomes for diagnostics and tests.

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

Loop policy wraps retry policy: each loop iteration executes the step with its
own retry attempts. Timeout policy is applied to command timeout selection and
to wait/delay flow-control steps.

When constructed with a `ConditionEvaluator`, the executor evaluates each
step's conditions before loop/retry execution starts. The original dispatcher
constructor remains available for missions that do not use conditions.

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
- `command.id` and `command.type` for command steps

Optional step and command fields:

- `timeout_ms`
- `retry`
- `retry_policy`
- `loop_policy`
- `timeout_policy`
- `wait`
- `delay`
- `skip`
- `abort`
- `conditions`
- `enabled`
- `priority`
- `payload`
- `metadata`

Unknown command names, invalid syntax, missing required fields, invalid scalar
types, negative timeouts, and empty enabled-step sets are rejected before a
mission reaches `MissionExecutor`.
