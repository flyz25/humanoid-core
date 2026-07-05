# Architecture

humanoid-core follows Clean Architecture with strict dependency direction:

```text
Applications
  -> Managers
    -> Interfaces
      -> Robot Factory
        -> Robot Adapter
          -> SDK Wrapper
            -> Vendor SDK
```

Applications depend on interfaces and managers. Managers depend only on abstract
interfaces. Robot adapters implement framework interfaces and isolate vendor
SDKs, simulators, middleware, or embedded transport details.

## Unitree G1 Communication Layer

Milestone 2 adds an optional Unitree G1 EDU communication adapter without
changing the core architecture:

```text
Application
  -> RobotFactoryRegistry
    -> IRobotFactory
      -> IRobotAdapter
        -> UnitreeG1Adapter
          -> LocoClientWrapper
            -> SdkWrapper
              -> LocoAdapter / HandAdapter / AudioAdapter
                -> Unitree SDK2
```

`IRobotAdapter` is the application-facing dependency. `UnitreeRobotFactory`
creates `UnitreeG1Adapter` through the generic factory interface.
`UnitreeG1Adapter` translates generic commands such as `Move`, `Stop`, `StandUp`,
`BalanceStand`, and `EmergencyStop`. `LocoClientWrapper` is a compatibility
facade over `plugins/unitree/sdk/SdkWrapper`. Only implementation files under
`plugins/unitree/sdk/` include Unitree SDK2 headers and own SDK client objects.

Milestone 4.6 adds read-only SDK2 communication monitoring inside `SdkWrapper`.
Heartbeat, timeout detection, reconnect attempts, and state synchronization use
the SDK locomotion service query path and do not issue movement, posture, hand,
audio, or actuator commands. When `UnitreeRobotFactory` receives an injected
`RobotStateManager`, synchronized state is converted into
`humanoid::core::RobotState` before it leaves the SDK boundary.

The Unitree adapter is optional at build time. When `UnitreeSDK2` is not found,
the SDK-free core and adapter interface still build.

Milestone 4.7 adds SDK-boundary command adapters for locomotion, hand/gesture,
and audio commands. These adapters perform command translation only; they do not
contain business logic, mission execution, planning, AI, or behavior sequencing.

Milestone 4.8 adds integration examples that wire the existing targets from an
application composition root:

```text
Example application
  -> PluginFactory / PluginRegistry
    -> UnitreeG1Plugin
      -> core::RobotAdapter

Example application
  -> RobotFactoryRegistry
    -> UnitreeRobotFactory
      -> IRobotAdapter
        -> LocoClientWrapper
          -> SdkWrapper
```

The examples validate integration without changing dependency direction. Core
libraries do not link concrete plugins, plugin infrastructure does not link
vendor SDKs, and SDK-boundary examples are built only when the Unitree SDK
abstraction target exists.

See `docs/api/plugin_integration.md` for the public plugin integration API
summary and example target list.

The core foundation modules do not implement ROS2, DDS participants, AI, OpenCV,
GUI workflows, behavior trees, planners, navigation, or event controllers.
Unitree SDK2 integration is isolated in the optional adapter and SDK wrapper
targets.

## Robot State and Telemetry Layer

Milestone 3 adds a vendor-independent state path without changing the adapter or
factory architecture:

```text
Robot adapter or state producer
  -> RobotState
    -> RobotStateManager
      -> TelemetryService
        -> Subscriber callbacks
```

`RobotState` is the canonical value-type snapshot for connection, power, motion,
velocity, pose, orientation, health, and timestamp data. `RobotStateManager`
stores the latest snapshot behind `std::shared_mutex`, allowing shared readers
and exclusive writers. `TelemetryService` receives a
`std::shared_ptr<const RobotStateManager>` through dependency injection and
publishes copied snapshots to subscribed callbacks.

This layer remains SDK-free and vendor independent. It does not perform robot
communication, command execution, planning, navigation, AI, behavior trees, or
mission orchestration.

## Mission Model

Milestone 6.1 adds a declarative vendor-independent mission model:

```text
Mission
  -> MissionStep
    -> Command
```

Milestone 6.2 adds `MissionExecutor`:

```text
MissionExecutor
  -> Mission
    -> MissionStep
      -> CommandDispatcher
        -> Command Framework
```

`Mission` and `MissionStep` are value types for describing ordered command
collections. `MissionExecutor` runs those steps only through the injected
`CommandDispatcher`; it does not bypass safety validation or command
translation. The mission layer does not parse YAML, manage robot state,
instantiate adapters, include vendor SDK headers, or implement behavior trees,
planners, navigation, AI, or application missions. See
`docs/api/mission_model.md` for the public contract.

Milestone 6.3 adds mission file loading:

```text
MissionLoader
  -> MissionParser
  -> MissionValidator
  -> Mission
```

The loader converts JSON or YAML documents into the mission model before
execution. `MissionExecutor` remains independent of YAML, JSON, files, and
parser code.

Milestone 6.4 through 6.7 complete the mission execution path:

```text
MissionLoader -> MissionParser -> MissionValidator -> Mission
                                                  -> MissionExecutor
MissionStep -> WaitStep / DelayStep
            -> RetryPolicy / LoopPolicy / TimeoutPolicy
            -> MissionCondition -> ConditionEvaluator -> RobotStateManager
            -> Command -> CommandDispatcher -> SafetyValidator -> IRobotAdapter
```

The executor applies wait, delay, retry, loop, timeout, skip, abort, and
condition decisions before forwarding command steps to `CommandDispatcher`.
Condition evaluation consumes only copied generic state from
`RobotStateManager` and generic capability metadata. The mission layer does not
depend on concrete adapters, plugins, SDK wrappers, or vendor SDKs. Runnable
examples remain application-layer composition and do not reverse this
dependency direction.

## Execution Runtime Context

Milestone 7.1 adds a shared state boundary for future execution engines:

```text
Mission framework / future execution engine
  -> ExecutionContext
    -> ExecutionContextId
    -> ExecutionScope / ExecutionState
    -> cooperative cancellation
    -> ExecutionMetadata
```

The context is a synchronized value container. It does not invoke or depend on
`MissionExecutor`, behavior trees, planners, ROS2, robot adapters, plugins, or
SDK wrappers. Future integrations may depend on the runtime context; the
runtime context must not depend on those integrations.

Milestone 7.2 adds a sibling runtime data boundary:

```text
Future execution engines
  -> Blackboard
    -> namespaced typed immutable values
```

The blackboard is not part of mission or behavior-tree policy. It provides
thread-safe storage and shared value lifetime only, and depends exclusively on
the C++ standard library.

Milestone 7.3 adds a process-local resource ownership boundary:

```text
Future execution engines
  -> ResourceManager
    -> ResourceLock / ResourceHandle
      -> shared or exclusive logical resource lease
```

The resource manager prevents incompatible owners from using the same logical
robot resource at the same time. It supports RAII release, timed acquisition,
and non-blocking acquisition. It does not know about missions, behavior trees,
robot adapters, plugins, SDK wrappers, or vendor SDKs.

Milestone 7.4 adds a framework-wide cooperative cancellation boundary:

```text
Mission / command / runtime / future behavior-tree execution
  -> CancellationToken
  -> CancellationSource
  -> CancellationRegistration
```

Cancellation sources own cancellation authority, tokens provide copyable
observation and callback registration, and linked sources support nested
execution. The cancellation framework contains no mission, command-dispatch,
behavior-tree, robot adapter, plugin, SDK wrapper, or vendor SDK logic.

Milestone 7.5 adds a generic scheduler boundary:

```text
Future execution engine
  -> RuntimeScheduler
    -> RuntimeJob callback
    -> RuntimeJobContext
      -> ExecutionContext
      -> CancellationToken
```

The scheduler owns queueing, priority/FIFO selection, parallel and sequential
worker dispatch, lifecycle snapshots, cooperative pause/resume, and cooperative
stop. It executes injected runtime callbacks only and does not contain mission,
behavior-tree, command-dispatch, robot adapter, plugin, SDK wrapper, or vendor
SDK logic.

Milestone 7.6 adds application-layer runtime integration examples:

```text
Examples
  -> ExecutionContext
  -> Blackboard
  -> ResourceManager
  -> CancellationSource / CancellationToken
  -> RuntimeScheduler
```

The examples validate runtime composition from the application boundary. They
link only against the vendor-independent core runtime and contain no Unitree,
SDK, adapter, mission, behavior-tree, ROS2, planner, navigation, or AI
dependencies.

Milestone 7.7 releases the execution runtime foundation as `0.7.0-alpha`. The
runtime layer remains a set of reusable primitives for future execution engines,
not an execution policy engine itself.

## Behavior Tree Core

Milestone 8.1 adds a behavior tree execution boundary on top of the runtime
foundation:

```text
BehaviorTree
  -> BTNode
  -> BTContext
    -> ExecutionContext
    -> Blackboard
```

The behavior tree core owns root-node lifecycle, serialized ticks, status
mapping, and node factory registration. It does not implement mission
execution, robot adapter logic, SDK communication, XML parsing, ROS2, planners,
navigation, or AI. Behavior tree nodes receive runtime dependencies through
`BTContext` and must keep robot/vendor integration behind existing adapter and
command boundaries.

Milestone 8.2 adds composite node policy under the same boundary:

```text
CompositeNode
  -> SequenceNode / SelectorNode / ParallelNode
  -> owned BTNode children
  -> BTContext
```

Composite nodes provide child traversal and aggregation only. They do not call
commands, missions, adapters, plugins, SDK wrappers, or vendor SDKs.

Milestone 8.3 adds decorator node policy under the same boundary:

```text
DecoratorNode
  -> InverterNode / RepeatNode / RetryNode
  -> SucceederNode / FailerNode / LimiterNode / TimeoutNode
  -> owned BTNode child
  -> BTContext
```

Decorator nodes transform child status or local execution policy only. They do
not issue commands, evaluate missions, instantiate adapters, load plugins,
include SDK wrappers, or depend on vendor SDKs.

## Generic Command Model

Milestone 5 adds a vendor-independent command path:

```text
Application or future command producer
  -> CommandExecutionPipeline
    -> injected executor
      -> CommandDispatcher
        -> SafetyValidator
        -> IRobotAdapter
  -> CommandQueue
  <- CommandResult / CommandStatus
```

Command IDs are assigned by the producer, timestamps use a monotonic clock, and
timeouts use `std::chrono`. Payload values and metadata contain framework-owned
standard-library types only. `CommandExecutionPipeline` owns execution IDs,
lifecycle callbacks, optional logging, metrics, and bounded history around an
injected executor. `CommandQueue` owns bounded asynchronous priority scheduling,
worker threads, timeout, cancellation, and statistics. `SafetyValidator` gates
execution using generic state, capability, emergency stop, fault, battery, and
posture data. `CommandDispatcher` composes the validator and queue, validates
commands, serializes adapter access, and forwards through `IRobotAdapter`.
These components have no SDK headers, concrete adapter dependencies, mission
logic, or global state. See `docs/api/command_model.md` for the complete public
contract.

Milestone 5.6 adds buildable command examples for execution, queueing,
cancellation, and capability validation. The examples validate composition from
application code into the command framework without introducing SDK, vendor, or
mission-engine dependencies.

## Plugin Infrastructure

Milestone 4 adds plugin infrastructure as a separate exported module:

```text
Application or plugin host
  -> humanoid::plugins
    -> humanoid::common
```

The aggregate core target `humanoid::humanoid_core` does not link against
`humanoid::plugins`, and no core module depends on concrete plugins. Future
plugin hosts may use `humanoid::plugins::IPlugin`,
`humanoid::plugins::IPluginRegistrar`, `humanoid::plugins::IPluginLoader`, and
`humanoid::plugins::PluginRegistry` to manage metadata, version compatibility,
registration, and lifecycle state. Plugin hosts may use
`humanoid::plugins::PluginFactory` to register creator callables, create plugin
instances, destroy plugin instances, and enumerate registered plugin records.

Milestone 4.4 adds the SDK-free Unitree G1 plugin skeleton. It packages
metadata, lifecycle, a plugin-local adapter skeleton, a manifest, and mock robot
state feedback without communicating with Unitree SDK2 or commanding motion.
Milestone 4 does not implement dynamic shared-library loading, manifest
parsing, or physical robot communication plugins.

## Module Ownership

- `common`: dependency-free lifecycle, status, and version primitives.
- `plugins`: plugin interfaces, metadata, version compatibility, lifecycle
  states, thread-safe registration registry, thread-safe plugin factory, and
  concrete plugin packages.
- `plugins/unitree/sdk`: Unitree SDK2 abstraction boundary and conversion layer.
- `utilities`: small implementation-agnostic helpers.
- `logging`: logger and sink interfaces plus sink routing infrastructure.
- `configuration`: read-only configuration interfaces and provider ownership.
- `robot`: robot and robot adapter abstractions.
- `motion`: motion controller abstractions.
- `gesture`: named gesture controller abstractions.
- `safety`: safety state and safety controller abstractions.
- `diagnostics`: diagnostic records and diagnostic controller abstractions.
- `network`: transport metadata and network manager abstraction.
- `core`: package metadata, application-facing interface context, generic robot
  state model, and generic command value model.
- `include/humanoid/adapters`: public robot adapter and factory contracts.
- `src/adapters`: adapter plugin implementations that depend on public adapter
  contracts and keep vendor SDK headers out of application-facing interfaces.
- `src/factory`: robot factory registry.
- `src/sdk`: legacy adapter-facing SDK facades hidden behind adapter implementations.
- `src/services`: vendor-independent runtime services such as telemetry
  publication.

## Adapter Rule

Vendor SDK headers must never be included by application code, manager headers,
or manager source files. Vendor SDK dependencies belong in adapter packages that
implement `IRobotAdapter` or other module interfaces.
