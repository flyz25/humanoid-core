# Runtime Integration Examples

Milestone 7.6 adds runnable examples for the vendor-independent runtime
primitives introduced in Milestone 7.

## Example Targets

| Target | Source | Coverage |
| --- | --- | --- |
| `humanoid_core_runtime_execution_context_example` | `examples/runtime_execution_context/main.cpp` | Execution identity, lifecycle state, metadata, timestamp, current step, and cancellation bridge. |
| `humanoid_core_runtime_blackboard_example` | `examples/runtime_blackboard/main.cpp` | Typed blackboard storage, exact-type lookup, removal, and retained shared ownership. |
| `humanoid_core_runtime_resource_manager_example` | `examples/runtime_resource_manager/main.cpp` | Shared resource leases, exclusive lease rejection while shared owners exist, release, and exclusive acquisition. |
| `humanoid_core_runtime_cancellation_example` | `examples/runtime_cancellation/main.cpp` | Cancellation source, token, callback registration, and linked child cancellation. |
| `humanoid_core_runtime_scheduler_example` | `examples/runtime_scheduler/main.cpp` | Runtime scheduler job submission, sequential execution, cooperative pause/resume, lifecycle result, and statistics. |

## Running

After configuring and building the project, run the examples from the configured
build directory's `examples/` folder:

```bash
./build/<configuration>/examples/humanoid_core_runtime_execution_context_example
./build/<configuration>/examples/humanoid_core_runtime_blackboard_example
./build/<configuration>/examples/humanoid_core_runtime_resource_manager_example
./build/<configuration>/examples/humanoid_core_runtime_cancellation_example
./build/<configuration>/examples/humanoid_core_runtime_scheduler_example
```

## Dependency Boundary

The examples link only against `humanoid::core`. They do not depend on Unitree
SDK2, robot adapters, plugins, mission execution, behavior trees, ROS2,
planners, navigation, AI, or robot hardware.

Each example validates its result and exits with `EXIT_FAILURE` when an
invariant is not met. This makes the examples suitable for lightweight manual
smoke validation in both Unitree-enabled and Unitree-disabled builds.
