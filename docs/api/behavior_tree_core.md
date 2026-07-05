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
#include <humanoid/bt/CompositeNode.h>
#include <humanoid/bt/SequenceNode.h>
#include <humanoid/bt/SelectorNode.h>
#include <humanoid/bt/ParallelNode.h>
#include <humanoid/bt/DecoratorNode.h>
#include <humanoid/bt/InverterNode.h>
#include <humanoid/bt/RepeatNode.h>
#include <humanoid/bt/RetryNode.h>
#include <humanoid/bt/SucceederNode.h>
#include <humanoid/bt/FailerNode.h>
#include <humanoid/bt/LimiterNode.h>
#include <humanoid/bt/TimeoutNode.h>
```

All behavior tree types are in `humanoid::bt`. The headers are also available
through `<humanoid/core.hpp>`.

## Dependency Boundary

```text
BehaviorTree
  -> BTNode
  -> CompositeNode
  -> DecoratorNode
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

## Composite Nodes

Milestone 8.2 adds standard composite nodes:

- `SequenceNode`
- `SelectorNode`
- `ParallelNode`

`CompositeNode` owns child nodes with `std::unique_ptr<BTNode>` and propagates
initialization, reset, and shutdown to children. It rejects null child
ownership and provides thread-safe child count and child registration APIs.

`SequenceNode` ticks children in order and succeeds only when all children
succeed. It fails on the first child failure and reports running on the first
running child.

`SelectorNode` ticks children in order and succeeds on the first child success.
It fails only when all children fail and reports running on the first running
child.

Both sequence and selector nodes support stateless and memory traversal:

- Stateless traversal restarts from the first child on every tick.
- Memory traversal resumes from the child that previously returned `Running`.

`ParallelNode` ticks children concurrently and aggregates their statuses. By
default it succeeds when all children succeed and fails when any child fails.
Configurable success and failure thresholds allow future execution policies
without introducing robot or mission dependencies.

All composite nodes check cooperative cancellation through `BTContext` before
executing child ticks.

## Decorator Nodes

Milestone 8.3 adds standard decorator nodes:

- `InverterNode`
- `RepeatNode`
- `RetryNode`
- `SucceederNode`
- `FailerNode`
- `LimiterNode`
- `TimeoutNode`

`DecoratorNode` owns one optional child node with `std::unique_ptr<BTNode>`.
It serializes child replacement, lifecycle propagation, reset, shutdown, and
decorator-specific state. Null child ownership is rejected by `SetChild()`.

`InverterNode` maps child success to failure and child failure to success.
Running, idle, and aborted statuses pass through as running or aborted policy
requires.

`RepeatNode` repeats a successful child for a configured number of completions.
A repeat count of zero means repeat indefinitely. It performs one child tick per
decorator tick and never busy-loops.

`RetryNode` retries a failing child up to the configured attempt count. It
resets the child between failed attempts and returns failure only after the
retry budget is exhausted.

`SucceederNode` forces non-running, non-aborted child results to success.
`FailerNode` forces non-running, non-aborted child results to failure.

`LimiterNode` permits only a configured number of child ticks before reset.
Once the limit is exhausted, it returns failure without ticking the child.

`TimeoutNode` fails a running child when a steady-clock timeout expires. It
checks the deadline before and after each child tick. It cannot preempt a
blocking child call; cancellation remains cooperative through `BTContext`.

All decorator nodes are vendor-independent and contain no mission, command,
adapter, plugin, SDK, XML parser, ROS2, planner, navigation, or AI logic.
