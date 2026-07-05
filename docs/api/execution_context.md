# Execution Runtime Context

Milestone 7.1 introduces a vendor-independent, thread-safe state container for
execution engines. It does not integrate an engine or alter mission execution.

## Public Headers

```cpp
#include <humanoid/runtime/ExecutionContext.h>
#include <humanoid/runtime/ExecutionContextId.h>
#include <humanoid/runtime/ExecutionMetadata.h>
#include <humanoid/runtime/ExecutionScope.h>
#include <humanoid/runtime/ExecutionState.h>
```

All types are in `humanoid::runtime` and are available through
`<humanoid/core.hpp>`.

## Context Model

`ExecutionContext` contains:

- An immutable execution ID and execution scope.
- An optional mission association represented by a framework-owned integer ID.
- Runtime state from `Created` through terminal completion states.
- An optional monotonic start timestamp.
- An optional engine-owned current step ID.
- A C++20 cooperative cancellation source and token.
- Non-operational string metadata.

Zero identifies an unassigned execution or mission ID. Empty optional values
mean that execution has not started or no step is active.

## Execution State

The shared states are `Created`, `Starting`, `Running`, `Paused`, `Completed`,
`Cancelled`, `Failed`, and `Aborted`. `isTerminal()` identifies the final four
states. The context stores state but intentionally does not enforce transition
policy; each future execution engine owns its valid transition rules.

## Execution Scope

`ExecutionScope` identifies the owning subsystem without introducing a
dependency on it. Defined scopes are `Unknown`, `Mission`, `BehaviorTree`,
`AIPlanner`, `ROS2`, and `Custom`. These values are descriptive and contain no
middleware or engine implementation.

## Cancellation

`CancellationToken()` returns a copyable `std::stop_token`.
`RequestCancellation()` is thread-safe, one-shot, and monotonic. An existing
context cannot clear cancellation because doing so would invalidate the
cooperative cancellation contract observed by token holders.

## Thread Safety

All public operations may be called concurrently. Mutable runtime data is
protected by an internal mutex. `Snapshot()` captures mission ID, state,
timestamp, step, and metadata under one lock. Cancellation is sampled from the
independently synchronized standard C++ stop state. Execution ID and scope are
immutable after construction.

Callers receive copies of metadata and optional values. No reference to
internally synchronized state escapes the context.

## Dependency Boundary

```text
Future execution engine
  -> humanoid::runtime::ExecutionContext
    -> C++ standard library
```

The runtime context contains no SDK headers and does not depend on robot
adapters, `MissionExecutor`, behavior-tree libraries, AI planners, or ROS2.
