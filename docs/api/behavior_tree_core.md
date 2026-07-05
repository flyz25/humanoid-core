# Behavior Tree Core

Milestone 8.1 introduces a vendor-independent behavior tree core that executes
on top of the execution runtime primitives added in Milestone 7.

## Public Headers

```cpp
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/bt/BehaviorTreeFactory.h>
```

All behavior tree types are in `humanoid::bt`. The headers are also available
through `<humanoid/core.hpp>`.

## Dependency Boundary

```text
BehaviorTree
  -> BTNode
  -> BTContext
    -> runtime::ExecutionContext
    -> runtime::Blackboard
  -> C++ standard library
```

The behavior tree core contains no mission implementation, robot adapter,
plugin implementation, SDK wrapper, Unitree SDK2 include, ROS2 integration,
XML parser, planner, navigation stack, or AI subsystem.

## Status Model

`BTStatus` defines:

- `Idle`: initialized but not currently executing.
- `Running`: the node or tree requires another tick.
- `Success`: execution completed successfully.
- `Failure`: execution completed with a handled failure.
- `Aborted`: execution was terminated by cancellation or policy.

`isTerminal()` returns true for `Success`, `Failure`, and `Aborted`.

## Node Contract

`BTNode` is a pure abstract interface. Implementations provide:

- `Name()`
- `Initialize(BTContext&)`
- `Tick(BTContext&)`
- `Reset(BTContext&)`
- `Shutdown(BTContext&)`

Nodes should return quickly from `Tick()` and report `BTStatus::Running` when
work must continue on a future tick. Long-running work must observe
cooperative cancellation through `BTContext`.

## Runtime Context

`BTContext` owns shared references to:

- `runtime::ExecutionContext`
- `runtime::Blackboard`

Null dependencies are replaced with default runtime-owned instances. Context
dependency replacement is thread-safe, and getters return copied
`std::shared_ptr` handles.

## Behavior Tree Lifecycle

`BehaviorTree` owns one root node. Lifecycle calls are serialized:

- `Initialize()` initializes the root and marks the runtime context as
  starting.
- `Tick()` initializes on first use, checks runtime cancellation, ticks the
  root, and maps the result to `ExecutionState`.
- `Reset()` resets the root and returns the tree to `Idle`.
- `Shutdown()` shuts down the root and is idempotent.

After a tree reaches a terminal status, additional ticks return the existing
terminal status until `Reset()` is called.

## Factory

`BehaviorTreeFactory` is a thread-safe registry for node creator callbacks. It
supports registering, unregistering, enumerating, creating nodes, and creating
a tree from a registered root node type. Creator exceptions are contained and
reported as an empty result.

The factory does not parse XML or any other behavior tree document format.
