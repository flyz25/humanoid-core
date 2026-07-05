# Mission Model

Milestone 6.1 defines the vendor-independent mission value model. It does not
add a mission engine, scheduler, parser, robot adapter, or vendor integration.

## Public Headers

```cpp
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/MissionResult.h>
#include <humanoid/mission/MissionStatus.h>
#include <humanoid/mission/MissionStep.h>
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

The mission model contains no Unitree SDK headers, robot communication,
execution engine, behavior tree, planner, navigation stack, YAML parser, or
state machine. Future mission execution components may consume this model, but
vendor types must remain behind adapter and SDK boundaries.
