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

## Behavior Tree Examples

Milestone 8.7 adds hardware-free behavior tree applications:

- `bt_greeting`: a sequence that stands through `CommandNode` before running a
  greeting action.
- `bt_flag_ceremony`: a preparation `MissionNode` followed by parallel flag and
  anthem actions.
- `bt_inspection`: a selector that falls back from a condition to a retrying
  inspection action.
- `bt_patrol`: a sequence of movement, observation, and stop nodes using the
  command framework.

Run the examples from the build directory:

```bash
./examples/humanoid_core_bt_greeting_example
./examples/humanoid_core_bt_flag_ceremony_example
./examples/humanoid_core_bt_inspection_example
./examples/humanoid_core_bt_patrol_example
```

All robot commands use a process-local `IRobotAdapter` implementation and pass
through `CommandDispatcher`. Mission execution passes through
`MissionExecutor`, and every tree executes through `BehaviorTreeRuntime` and
the shared runtime services. No vendor SDK or robot hardware is required.

## Planning Examples

Milestone 9.6 adds hardware-free planning pipeline examples:

- `greeting`: plans the goal "Wave to audience" into a mission and behavior tree.
- `flag-ceremony`: plans a high-priority ceremony routine.
- `inspection`: plans an inspection sequence with movement and stop commands.
- `stage-demo`: plans a stage presentation routine.

Run all examples or one named scenario from the build directory:

```bash
./examples/humanoid_core_planning_examples
./examples/humanoid_core_planning_examples greeting
./examples/humanoid_core_planning_examples flag-ceremony
./examples/humanoid_core_planning_examples inspection
./examples/humanoid_core_planning_examples stage-demo
```

Each scenario creates a `Goal`, executes it through `PlanningPipeline` and
`RuleBasedPlanner`, prints the generated `Mission`, submits the generated
`BehaviorTree` through `BehaviorTreeRuntime`, and reports the runtime result.
The examples contain no provider SDK, HTTP client, robot SDK, robot hardware
dependency, mission-engine bypass, or application-specific adapter logic.
