# Dependency Graph

The dependency graph must remain acyclic and point toward stable framework
interfaces.

```text
Applications
  -> humanoid::humanoid_core
  -> humanoid::robot_factory
  -> humanoid::adapter_interfaces

humanoid::humanoid_core
  -> humanoid::core
  -> humanoid::common
  -> humanoid::utilities
  -> humanoid::logging
  -> humanoid::configuration
  -> humanoid::robot
  -> humanoid::motion
  -> humanoid::gesture
  -> humanoid::safety
  -> humanoid::diagnostics
  -> humanoid::network
  -> humanoid::adapter_interfaces
  -> humanoid::robot_factory
  -> humanoid::telemetry_service

humanoid::plugins
  -> humanoid::common

humanoid::unitree_g1_plugin
  -> humanoid::plugins
  -> humanoid::humanoid_core

humanoid::robot_factory
  -> humanoid::adapter_interfaces

humanoid::telemetry_service
  -> humanoid::core

humanoid::core
  -> humanoid::common
  -> humanoid::configuration
  -> humanoid::logging
  -> humanoid::adapter_interfaces

humanoid::core::CoreContext
  -> injected RobotStateManager

Future execution engines
  -> humanoid::runtime::ExecutionContext
  -> humanoid::runtime::Blackboard
  -> humanoid::runtime::ResourceManager
  -> humanoid::runtime::CancellationSource / CancellationToken
  -> humanoid::runtime::RuntimeScheduler
  -> C++ standard library

Behavior tree core
  -> humanoid::bt::BehaviorTree
  -> humanoid::bt::BTNode
  -> humanoid::bt::BTContext
  -> humanoid::runtime::ExecutionContext
  -> humanoid::runtime::Blackboard
  -> C++ standard library

Robot state flow
  -> humanoid::core::RobotState
  -> humanoid::core::RobotStateManager
  -> humanoid::telemetry_service
  -> subscriber callbacks

Mission model flow
  -> humanoid::mission::MissionLoader
  -> humanoid::mission::MissionParser
  -> humanoid::mission::MissionValidator
  -> humanoid::mission::MissionExecutor
  -> humanoid::mission::Mission
  -> humanoid::mission::MissionStep
  -> humanoid::mission::WaitStep / DelayStep
  -> humanoid::mission::RetryPolicy / LoopPolicy / TimeoutPolicy
  -> humanoid::mission::MissionCondition / MissionEvent
  -> humanoid::mission::ConditionEvaluator
  -> humanoid::core::RobotStateManager
  -> humanoid::core::CommandDispatcher
  -> humanoid::core::Command

Command execution flow
  -> humanoid::core::CommandExecutionPipeline
  -> injected executor callback
  -> optional humanoid::core::CommandDispatcher
  -> humanoid::core::Command
  -> humanoid::core::SafetyValidator
  -> humanoid::core::CommandQueue
  -> injected humanoid::adapters::IRobotAdapter
  -> concrete adapter
  -> SDK wrapper

humanoid::unitree_adapter
  -> humanoid::adapter_interfaces
  -> humanoid::logging
  -> humanoid::unitree_loco_client
  -> injected humanoid::core::RobotStateManager

humanoid::unitree_loco_client
  -> humanoid::adapter_interfaces
  -> humanoid::core
  -> humanoid::unitree_sdk_abstraction

humanoid::unitree_sdk_abstraction
  -> humanoid::core::RobotState
  -> internal LocoAdapter
  -> internal HandAdapter
  -> internal AudioAdapter
  -> unitree_sdk2

unitree_sdk2
  -> ddsc
  -> ddscxx
  -> Threads
```

## Allowed Directions

- Applications may depend on managers, interfaces, and factory registry.
- Factory registry may depend on factory interfaces.
- Factories may create concrete adapters.
- Plugin hosts may depend on `humanoid::plugins`.
- Plugin registry and factory infrastructure may depend on `common`.
- Concrete plugin packages may depend on `humanoid::plugins` and public
  framework interfaces.
- Adapters may depend on SDK wrapper facades.
- SDK wrapper facades may depend on vendor SDK abstraction targets.
- Vendor SDK abstraction targets may depend on vendor SDKs.
- Managers may depend on module interfaces and `common`.
- Runtime services may depend on core state models and managers.
- Runtime services receive core services through dependency injection.
- Runtime resource coordination may depend only on runtime resource primitives
  and the C++ standard library.
- Runtime cancellation primitives may be consumed by mission, command, runtime,
  and future behavior-tree layers, but must not depend on those layers.
- Runtime scheduler may depend on runtime context and cancellation primitives,
  and may execute injected callbacks only.
- Behavior tree core may depend on execution runtime context and blackboard,
  but must not depend on mission execution, command dispatch, robot adapters,
  plugins, SDK wrappers, vendor SDKs, XML parsers, ROS2, planners, navigation,
  or AI.
- Mission models may depend on generic command value types and
  vendor-independent flow-control policy value types.
- Mission condition evaluation may depend on `RobotStateManager` and generic
  command capability values, but must not call adapters, plugins, SDK wrappers,
  or vendor SDKs.
- Mission execution may depend on the command dispatcher component and must not
  call robot adapters directly.
- Mission executors must not depend on YAML, JSON, files, or parser code.
- Command execution lifecycle infrastructure may depend on generic commands,
  command results, logging interfaces, and injected executor callbacks.
- Command dispatch may depend on the abstract robot adapter interface.
- Command safety validation may depend only on generic command, capability, and
  robot state models.
- `common` depends only on the C++ standard library.

## Forbidden Dependencies

- Applications including vendor SDK headers.
- Applications directly constructing concrete robot adapters.
- Managers depending on concrete adapters or vendor SDKs.
- Core modules depending on Unitree SDK2, ROS2, OpenCV, AI runtimes, GUI
  frameworks, planners, navigation, behavior trees, or mission execution paths
  that bypass the command framework.
- Runtime resource primitives depending on robot adapters, plugins, SDK
  wrappers, vendor SDKs, mission execution, or behavior-tree implementations.
- Runtime cancellation primitives depending on robot adapters, plugins, SDK
  wrappers, vendor SDKs, command dispatch, mission execution, or behavior-tree
  implementations.
- Runtime scheduler depending on mission execution, behavior-tree
  implementation, command dispatch, robot adapters, plugins, SDK wrappers, or
  vendor SDKs.
- Behavior tree core depending on mission execution, robot adapters, plugins,
  SDK wrappers, vendor SDKs, XML parsers, ROS2, planners, navigation, or AI.
- Core framework modules depending on concrete plugins.
- `humanoid::humanoid_core` linking concrete plugins or plugin implementations.
- SDK-free plugin skeletons including vendor SDK headers.
- SDK wrapper facades including vendor SDK headers directly.
- Unitree SDK headers outside `plugins/unitree/sdk/*.cpp`.
- Vendor SDK types in public interfaces.
- Global singleton access as a framework dependency pattern.
