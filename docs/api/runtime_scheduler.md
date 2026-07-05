# Runtime Scheduler

Milestone 7.5 introduces a vendor-independent scheduler for generic runtime
execution jobs.

## Public Header

```cpp
#include <humanoid/runtime/RuntimeScheduler.h>
```

`RuntimeScheduler`, `RuntimeJob`, `RuntimeJobContext`, and related scheduler
types are in `humanoid::runtime`. The header is also available through
`<humanoid/core.hpp>`.

## Responsibilities

The scheduler owns:

- Bounded runtime job queueing.
- Priority scheduling with FIFO ordering inside equal priority.
- Parallel execution through worker threads.
- Sequential execution for jobs that require exclusive scheduler access.
- Lifecycle snapshots.
- Cooperative pause and resume.
- Cooperative stop through runtime cancellation.
- Shutdown and worker joining.

Runtime jobs are injected callbacks. The scheduler does not know about missions,
behavior trees, robot adapters, plugins, ROS2, SDK wrappers, vendor SDKs,
planners, navigation, or AI systems.

## Job Model

`RuntimeJob` contains a producer-assigned nonzero id, priority, execution mode,
scope, metadata, and callback.

`RuntimeExecutionMode::Parallel` allows concurrent execution with other parallel
jobs up to the configured worker count. `RuntimeExecutionMode::Sequential`
requires exclusive scheduler execution and starts only when no other job is
running.

`RuntimeJobPriority` provides `Low`, `Normal`, `High`, and `Critical`. Higher
priority jobs start first; equal priority jobs preserve FIFO order by scheduler
sequence number.

## Callback Context

Workers invoke callbacks with `RuntimeJobContext`. The context exposes:

- Job id.
- `ExecutionContext`.
- `CancellationToken`.
- `IsCancellationRequested()`.
- `IsPaused()`.
- `WaitIfPaused()`.

Pause, resume, and stop are cooperative. A callback that performs long-running
work should periodically call `WaitIfPaused()` and check cancellation.

## Lifecycle

Queued jobs have `ExecutionState::Created`. Running jobs use
`ExecutionState::Running`. Paused jobs use `ExecutionState::Paused`. Terminal
jobs use `Completed`, `Cancelled`, `Failed`, or `Aborted`.

`Submit()` returns `RuntimeJobHandle`, which contains the job id and a
`std::shared_future<RuntimeJobResult>`. Invalid, duplicate, full-queue, and
post-shutdown submissions return a ready failed result.

## Dependency Boundary

```text
Future execution engines
  -> RuntimeScheduler
    -> RuntimeJob callback
    -> RuntimeJobContext
      -> ExecutionContext
      -> CancellationToken
      -> C++ standard library
```

The scheduler is a runtime primitive. It must remain independent of mission
execution, behavior-tree execution, command dispatch, adapters, plugins, and
vendor SDKs.
