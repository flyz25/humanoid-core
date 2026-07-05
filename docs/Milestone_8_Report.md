# Milestone 8 Report

## Summary

Milestone 8 completes the vendor-independent Behavior Tree Framework for
humanoid-core `0.8.0-alpha`.

Behavior trees execute on the shared Milestone 7 runtime and reach robot or
mission operations only through existing framework boundaries. The behavior
tree layer contains no vendor SDK headers, concrete robot adapter dependency,
ROS2 integration, planner, navigation stack, or AI subsystem.

## Delivered Milestones

- Milestone 8.1: behavior tree node contract, status model, runtime-backed
  context, tree lifecycle, and node factory.
- Milestone 8.2: sequence, memory sequence, selector, memory selector, and
  parallel composite nodes.
- Milestone 8.3: inverter, repeat, retry, succeeder, failer, limiter, and
  timeout decorator nodes.
- Milestone 8.4: action, condition, wait, delay, command, and mission leaf
  nodes.
- Milestone 8.5: JSON and YAML tree parser, validator, loader, registered-node
  validation, and recursive tree construction.
- Milestone 8.6: execution through the shared runtime scheduler, execution
  context, blackboard, cancellation framework, and resource manager.
- Milestone 8.7: runnable Greeting, Flag Ceremony, Inspection, and Patrol
  behavior tree examples.
- Milestone 8.8: integrated documentation, release metadata, build matrix,
  test, example, install, and package validation.

## Architecture Validation

```text
Application
  -> BehaviorTreeRuntime
    -> BehaviorTreeFactory / loaded BehaviorTree
    -> RuntimeScheduler
      -> ExecutionContext
      -> CancellationToken
    -> Blackboard
    -> ResourceManager

BehaviorTree
  -> Composite / Decorator / Leaf BTNode
  -> BTContext

CommandNode -> CommandDispatcher -> IRobotAdapter
MissionNode -> MissionExecutor -> CommandDispatcher
```

- Runtime primitives do not depend on behavior tree implementation types.
- Behavior tree core does not depend on concrete adapters, plugins, SDK
  wrappers, or vendor SDKs.
- `BehaviorTreeRuntime` receives existing runtime services through dependency
  injection and creates no duplicate scheduler, blackboard, cancellation, or
  resource infrastructure.
- `CommandNode` and `MissionNode` preserve command and mission framework
  boundaries.
- Parser and loader concerns remain outside `BehaviorTree` and `BTNode`.
- Tree ownership uses `std::unique_ptr`; shared runtime service lifetimes use
  explicit `std::shared_ptr` ownership.

## Functional Validation

The automated suite verifies:

- Tree initialization, tick, reset, shutdown, cancellation, status mapping,
  and serialized concurrent access.
- Nested sequence, selector, memory traversal, and parallel aggregation.
- Retry, timeout, repeat, limit, inverter, succeeder, and failer behavior.
- Action, condition, wait, delay, command, and mission leaf execution.
- JSON and YAML parsing, missing-node rejection, unknown-node rejection, file
  loading, and factory-backed construction.
- Shared execution context and blackboard binding.
- Concurrent trees, multiple runtime integration instances, resource
  exclusion, cancellation, and runtime result propagation.

## Build Matrix

All configurations use `HUMANOID_CORE_WARNINGS_AS_ERRORS=ON`.

| Configuration | Configure | Build | CTest |
| --- | --- | --- | --- |
| Debug, `ENABLE_UNITREE=ON` | Passed | Passed | 28/28 passed |
| Debug, `ENABLE_UNITREE=OFF` | Passed | Passed | 26/26 passed |
| Release, `ENABLE_UNITREE=ON` | Passed | Passed | 28/28 passed |
| Release, `ENABLE_UNITREE=OFF` | Passed | Passed | 26/26 passed |

The ON configurations detected the pinned Unitree SDK2 submodule at
`third_party/unitree_sdk2`. No physical robot was required by the behavior tree
tests or examples.

## Example Validation

| Example | Debug ON | Debug OFF | Release ON | Release OFF |
| --- | --- | --- | --- | --- |
| Greeting | Passed | Passed | Passed | Passed |
| Flag Ceremony | Passed | Passed | Passed | Passed |
| Inspection | Passed | Passed | Passed | Passed |
| Patrol | Passed | Passed | Passed | Passed |

The examples collectively validate sequence, selector, retry, parallel,
`MissionNode`, and `CommandNode`. They use a process-local adapter behind
`IRobotAdapter`; command and mission leaves still execute through the production
framework boundaries.

## Install and Package Validation

| Configuration | Install | CMake package discovery |
| --- | --- | --- |
| Release, `ENABLE_UNITREE=ON` | Passed | Passed |
| Release, `ENABLE_UNITREE=OFF` | Passed | Passed |

The installed package exports public behavior tree headers, runtime-linked
targets, semantic version `0.8.0-alpha`, package configuration, package version
configuration, and `FindUnitreeSDK2.cmake`. The ON package was discovered with
`UNITREE_SDK2_ROOT`; the OFF package was discovered without an SDK dependency.
Temporary downstream consumers compiled, linked, and ran against both installed
packages through `find_package(humanoid_core CONFIG REQUIRED)` and
`humanoid::humanoid_core`.

## Documentation

Release documentation includes:

- `README.md`
- `CHANGELOG.md`
- `SECURITY.md`
- `docs/api/behavior_tree_core.md`
- `docs/architecture/README.md`
- `docs/dependency_graph.md`
- `docs/thread_safety.md`
- `docs/versioning.md`
- `examples/README.md`
- `docs/Milestone_8_Report.md`

Documentation and source formatting were validated with `markdownlint`,
`clang-format`, and repository whitespace checks.

## Known Limitations

- Behavior tree cancellation and scheduler pause are cooperative. A blocking
  node callback cannot be preempted by the framework.
- The YAML parser supports the documented deterministic subset. XML loading is
  recognized but remains disabled.
- `TimeoutNode` observes elapsed time between ticks and cannot interrupt a
  blocking child tick.
- `ParallelNode` requires child implementations and captured external state to
  provide their own synchronization.
- The supplied behavior tree examples use a process-local adapter and do not
  validate physical robot behavior.

## Release Result

Milestone 8 is complete. The repository is ready for the `v0.8.0-alpha`
release tag.
