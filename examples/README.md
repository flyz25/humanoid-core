# Examples

This directory contains buildable examples for humanoid-core. Examples are
application-layer composition code and do not change framework dependency
direction.

## Runtime Examples

Milestone 7.6 adds runtime examples for:

- `runtime_execution_context`: execution identity, lifecycle state, metadata,
  timestamps, current step, and cancellation.
- `runtime_blackboard`: typed namespaced storage, exact-type retrieval, and
  retained shared ownership.
- `runtime_resource_manager`: shared and exclusive resource leases.
- `runtime_cancellation`: cancellation sources, tokens, callbacks, and linked
  cancellation.
- `runtime_scheduler`: runtime job submission, sequential execution,
  pause/resume, and scheduler statistics.

Build the project, then run the examples from the build directory:

```bash
./examples/humanoid_core_runtime_execution_context_example
./examples/humanoid_core_runtime_blackboard_example
./examples/humanoid_core_runtime_resource_manager_example
./examples/humanoid_core_runtime_cancellation_example
./examples/humanoid_core_runtime_scheduler_example
```

These examples require no robot hardware, Unitree SDK calls, mission execution,
behavior trees, ROS2, planners, navigation, or AI systems.
