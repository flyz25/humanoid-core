# humanoid-core

humanoid-core is a C++20 Clean Architecture foundation for humanoid robot
applications. The framework remains vendor independent: applications depend on
interfaces and managers, while robot vendors are integrated as adapter plugins
behind factories and SDK wrappers.

The current SDK integration supports Unitree G1 through Unitree SDK2. Unitree
SDK2 is included as a pinned Git submodule at `third_party/unitree_sdk2`; it is
not installed into `/usr/local` and is not required as a system dependency.

Current release: `0.8.0-alpha`

## Architecture

```text
Applications
  -> Managers
    -> Interfaces
      -> Robot Factory
        -> Robot Adapter
          -> SDK Wrapper
            -> Vendor SDK
```

Applications receive `std::unique_ptr<humanoid::adapters::IRobotAdapter>` from
`RobotFactoryRegistry`. Applications never instantiate concrete adapters and
never include Unitree SDK headers.

Unitree G1 dependency flow:

```text
Application
  -> RobotFactoryRegistry
    -> IRobotFactory
      -> IRobotAdapter
        -> UnitreeRobotFactory
          -> UnitreeG1Adapter
            -> LocoClientWrapper
              -> SdkWrapper
                -> LocoAdapter / HandAdapter / AudioAdapter
                -> Unitree SDK2
```

Only implementation files under `plugins/unitree/sdk/` include Unitree SDK2
headers. `src/sdk/LocoClientWrapper.cpp` is a compatibility facade over the SDK
abstraction layer and does not include vendor SDK headers.

Milestone 4.6 adds read-only SDK2 communication monitoring inside the SDK
abstraction. Heartbeats, connection timeout handling, automatic reconnect, and
state synchronization use read-only SDK queries. The monitoring path does not
issue walking, standing, hand, audio, or other actuator commands. A
`RobotStateManager` can be injected through `UnitreeRobotFactory` so synchronized
state is published as vendor-independent `humanoid::core::RobotState`.
On Linux, SDK initialization also validates network-interface existence and
route netlink socket access before constructing the Unitree SDK client, so
restricted environments fail gracefully through `Result`.

Milestone 4.7 adds vendor-specific SDK adapters inside the same boundary:
`LocoAdapter`, `HandAdapter`, and `AudioAdapter`. They translate framework-owned
motion, hand/gesture, and audio commands into SDK2 calls without adding mission
logic, planning, AI, or behavior execution. Unitree G1 SDK2 exposes
hand-related behavior through the arm action service; unsupported finger-level
open, close, and grip commands are rejected explicitly instead of being faked.

Milestone 3 adds a vendor-independent runtime state path:

```text
Robot adapter or state producer
  -> RobotState
    -> RobotStateManager
      -> TelemetryService
        -> Subscriber callbacks
```

The state and telemetry path remains SDK-free. `RobotStateManager` is injected
through `CoreContext`, and `TelemetryService` receives that manager explicitly.

Milestone 5 adds the generic command path:

```text
Command producer
  -> CommandExecutionPipeline
    -> injected executor
      -> CommandDispatcher
        -> SafetyValidator
        -> IRobotAdapter
  -> CommandQueue
  <- CommandResult / CommandStatus
```

Commands carry producer-assigned IDs, monotonic timestamps, `std::chrono`
timeouts, typed framework payload values, and non-operational metadata. The
model, execution pipeline, queue, and dispatcher are vendor independent.
`CommandExecutionPipeline` provides execution IDs, lifecycle callbacks, optional
logging, metrics, and bounded history around injected executors. `CommandQueue`
provides bounded priority/FIFO scheduling, configurable consumers, timeout,
cancellation, statistics, and controlled shutdown. `SafetyValidator` rejects
disconnected, faulted, emergency-stop, unsupported-capability, low-battery, and
unsafe-state commands before adapter execution. The dispatcher composes the
validator and queue for forwarding while keeping SDK and concrete adapter
dependencies out of core.

Milestone 4 adds a separate plugin infrastructure target:

```text
Application or plugin host
  -> humanoid::plugins
    -> humanoid::common
```

The aggregate core target `humanoid::humanoid_core` does not link against
`humanoid::plugins`. Plugin hosts can use `PluginRegistry` for metadata and
lifecycle visibility, and `PluginFactory` for dependency-injected creator
registration, creation, destruction, and enumeration.

Milestone 4.4 adds the first concrete plugin package:

```text
Application or plugin host
  -> PluginFactory
    -> UnitreeG1Plugin
      -> UnitreeG1Adapter
        -> mock RobotState
```

The Unitree G1 plugin skeleton is SDK-free. It validates plugin packaging,
metadata, lifecycle, adapter construction, and conservative mock state feedback.
It does not communicate with Unitree SDK2 and does not command robot movement.

Milestone 4.8 adds buildable integration examples that exercise the composition
paths for plugin registry/factory wiring, plugin lifecycle, robot factory
connection, telemetry, adapter capability queries, and Unitree SDK-boundary
command adapters. These examples do not change dependency direction: core still
does not depend on plugins, and application-facing examples never include
Unitree SDK2 headers.

Milestone 6.1 adds a vendor-independent mission model:

```text
Mission
  -> MissionStep
    -> Command
```

Milestone 6.2 adds `MissionExecutor`, which executes mission steps only through
the existing `CommandDispatcher`:

```text
MissionExecutor
  -> Mission
    -> MissionStep
      -> CommandDispatcher
        -> Command Framework
```

The mission layer does not parse YAML, implement behavior trees, add planners,
perform navigation, include SDK headers, or call robot adapters directly.

Milestone 6.3 adds `MissionLoader`, `MissionParser`, and `MissionValidator`:

```text
Mission file (.json/.yaml/.yml)
  -> MissionLoader
    -> MissionParser
    -> MissionValidator
      -> Mission
```

The loader converts mission JSON or YAML into the mission model. `MissionExecutor`
does not know about file formats and still receives only `Mission` objects.

Milestone 6.4 adds mission flow control:

```text
MissionStep
  -> WaitStep / DelayStep
  -> RetryPolicy
  -> LoopPolicy
  -> TimeoutPolicy
  -> CommandDispatcher for command steps only
```

Flow-control steps can wait, delay, retry, loop, timeout, skip, or abort mission
execution without adding robot-specific logic or SDK dependencies. Command
steps still execute only through the existing command framework.

Milestone 6.5 adds mission events and conditions:

```text
MissionStep
  -> MissionCondition
  -> ConditionEvaluator
    -> RobotStateManager
```

Conditions let mission execution react to battery level, connection state,
generic robot motion state, command capabilities, fault codes, and emergency
stop state. Runtime state is read only through `RobotStateManager`; no adapter,
plugin, SDK wrapper, or vendor SDK is called from the condition layer.

Milestone 6.6 provides runnable, hardware-free mission examples in
`examples/mission_execution/`. Build the project, then run:

```bash
./build/examples/humanoid_core_mission_execution_example \
  examples/mission_execution/simple.yaml execute
./build/examples/humanoid_core_mission_execution_example \
  examples/mission_execution/demo.yaml pause-resume
./build/examples/humanoid_core_mission_execution_example \
  examples/mission_execution/flag_ceremony.yaml cancel
```

Each command loads and validates YAML before composing `MissionExecutor` with
`CommandDispatcher`. The process-local example adapter performs no SDK or robot
communication. Replace it at the application composition boundary to execute a
mission against a supported robot adapter.

Milestone 6 completes the mission framework by integrating the mission model,
strict loader and validator, thread-safe executor, flow-control policies,
RobotStateManager-backed conditions, and runnable lifecycle examples. Mission
commands always enter the existing command and safety framework; mission code
does not call adapters or SDKs directly. The release validation record is in
`docs/Milestone_6_Report.md`.

Milestone 7.1 adds a vendor-independent execution runtime context:

```text
Future execution engine
  -> ExecutionContext
    -> execution identity, scope, state, timestamp, current step
    -> cooperative cancellation and runtime metadata
```

`ExecutionContext` is a synchronized state container, not an execution engine.
It has no dependency on `MissionExecutor`, behavior trees, AI planners, ROS2,
robot adapters, or vendor SDKs. See `docs/api/execution_context.md`.

Milestone 7.2 adds `humanoid::runtime::Blackboard`, a thread-safe namespaced
store for exact typed values. Values are exposed through immutable shared
ownership, so retrieved handles remain valid after replacement, removal, or
clear operations. The blackboard contains no mission, behavior-tree, robot, or
vendor logic. See `docs/api/blackboard.md`.

Milestone 7.3 adds `humanoid::runtime::ResourceManager` for process-local
coordination of logical robot resources. It returns move-only RAII
`ResourceLock` leases, supports shared and exclusive ownership, non-blocking
acquisition, timed acquisition, and handle-based release for integration
boundaries. The manager is vendor independent and contains no SDK, adapter,
mission, or behavior-tree logic. See `docs/api/resource_manager.md`.

Milestone 7.4 adds `humanoid::runtime::CancellationSource`,
`CancellationToken`, and `CancellationRegistration` for framework-wide
cooperative cancellation. The same primitive can be shared by mission, command,
runtime, and future behavior-tree execution without adding dependencies between
those layers. See `docs/api/cancellation.md`.

Milestone 7.5 adds `humanoid::runtime::RuntimeScheduler`, a generic execution
scheduler for runtime jobs. It provides bounded priority/FIFO queueing,
parallel and sequential dispatch, lifecycle snapshots, cooperative pause/resume,
and cooperative stop through runtime cancellation. See
`docs/api/runtime_scheduler.md`.

Milestone 7.6 adds runnable runtime integration examples for
`ExecutionContext`, `Blackboard`, `ResourceManager`, `CancellationSource`, and
`RuntimeScheduler`. The examples are hardware-free and link only against the
vendor-independent core runtime. See `docs/api/runtime_examples.md`.

Milestone 7 completes the execution runtime foundation. The release integrates
the runtime context, blackboard, resource manager, cancellation framework,
scheduler, tests, examples, install/export metadata, and documentation without
adding mission, behavior-tree, ROS2, AI, planner, robot-adapter, or SDK
dependencies to the runtime layer. The release validation record is in
`docs/Milestone_7_Report.md`.

Milestone 8.1 adds the vendor-independent behavior tree core:

```text
BehaviorTree
  -> BTNode
  -> BTContext
    -> ExecutionContext
    -> Blackboard
```

The behavior tree layer provides node contracts, lifecycle management, runtime
context wiring, status handling, and node factory registration. It does not
implement mission execution, robot adapters, SDK communication, XML parsing,
planners, navigation, ROS2, or AI. See `docs/api/behavior_tree_core.md`.

Milestone 8.2 adds standard behavior tree composites: sequence, selector,
memory sequence, memory selector, and parallel nodes. Composite nodes own child
nodes, use the runtime-backed `BTContext`, support cooperative cancellation,
and remain vendor independent.

Milestone 8.3 adds standard behavior tree decorators: inverter, repeat, retry,
succeeder, failer, limiter, and timeout nodes. Decorators own one child node,
transform child status or local execution policy, observe cooperative
cancellation, and remain independent of missions, adapters, plugins, SDKs,
ROS2, planners, navigation, and AI.

Milestone 8.4 adds reusable behavior tree leaf nodes: action, condition, wait,
delay, command, and mission nodes. `CommandNode` executes only through
`CommandDispatcher`, `MissionNode` executes only through `MissionExecutor`, and
condition-oriented nodes inspect runtime state through `BTContext`.

Milestone 8.5 adds `TreeLoader`, `TreeParser`, and `TreeValidator` for loading
behavior trees from JSON or YAML documents. Parsing remains outside
`BehaviorTree`; documents are validated against registered node types and then
constructed through `BehaviorTreeFactory`.

Milestone 8.6 adds `BehaviorTreeRuntime`, which submits factory-created or
loader-created trees to the existing `RuntimeScheduler`. Each tree receives the
scheduler-owned `ExecutionContext`, the injected shared `Blackboard`,
cooperative cancellation, and an optional RAII resource lease from the shared
`ResourceManager`. The integration owns no scheduler, worker queue, blackboard,
or resource registry implementation of its own.

Milestone 8.7 adds runnable Greeting, Flag Ceremony, Inspection, and Patrol
behavior tree examples. Together they demonstrate sequence, selector, retry,
parallel, mission, and command nodes through the existing runtime, command, and
mission framework boundaries without robot hardware.

Milestone 8.8 releases the integrated Behavior Tree Framework as
`0.8.0-alpha`. Core lifecycle, composites, decorators, leaves, loaders, runtime
integration, and hardware-free examples are validated across Debug/Release and
Unitree-enabled/disabled configurations. See `docs/Milestone_8_Report.md`.

Milestone 9.1 adds a vendor-independent goal model for user intent:

```text
Goal
  -> PlanningRequest
    -> ExecutionContextSnapshot
    -> RobotCapabilities
  -> PlanningResult
    -> Mission / BehaviorTree / Diagnostics
```

The planner model contains only framework-owned data. It does not implement a
planner engine, LLM integration, OpenAI or Claude APIs, robot SDK calls, mission
execution, behavior tree execution, or adapter communication. See
`docs/api/planner_goal_model.md`.

Milestone 9.2 adds the abstract planner interface and planner discovery
infrastructure:

```text
Application or future planner host
  -> IPlanner
  -> PlannerFactory
    -> injected PlannerRegistry
```

`IPlanner` exposes only `Plan()`, `ValidatePlan()`, `CancelPlan()`, and
`GetCapabilities()`. `PlannerFactory` and `PlannerRegistry` support multiple
future planner families such as rule planners, LLM planners, and symbolic
planners without adding any concrete planner implementation or SDK dependency.

## Directory Structure

```text
humanoid-core/
  apps/                      Application targets owned by downstream products
  cmake/                     CMake package and dependency discovery modules
  common/                    Shared status, lifecycle, and version primitives
  config/                    Robot configuration examples
  configuration/             Configuration interfaces and manager
  core/                      Core context, metadata, state, and command models
  diagnostics/               Diagnostic interfaces and manager
  docs/                      Architecture and engineering documentation
  examples/                  Buildable examples
  gesture/                   Gesture interfaces and manager
  include/humanoid/adapters/ Public robot adapter and factory contracts
  include/humanoid/bt/       Public behavior tree core contracts
  include/humanoid/mission/ Public mission model contracts
  include/humanoid/planner/ Public goal and planning request/result contracts
  include/humanoid/runtime/ Public execution runtime contracts
  logging/                   Logging interfaces and routing manager
  motion/                    Motion interfaces and manager
  network/                   Network interfaces and endpoint metadata
  plugins/                   Plugin interfaces, registry, factory, and plugin packages
  plugins/unitree/g1/        SDK-free Unitree G1 plugin skeleton
  plugins/unitree/sdk/       Unitree SDK2 abstraction boundary
  robot/                     Robot interfaces plus manager
  safety/                    Safety interfaces and manager
  scripts/                   Build and formatting scripts
  src/adapters/unitree/      Optional SDK-backed Unitree adapter target
  src/factory/               Robot factory registry
  src/sdk/                   Legacy adapter-facing SDK facades
  src/services/              Vendor-independent runtime services
  tests/                     Smoke test and optional GoogleTest tests
  third_party/unitree_sdk2/  Pinned Unitree SDK2 submodule
  utilities/                 Reusable framework utilities
```

## Unitree SDK2

Pinned SDK:

```text
Repository: https://github.com/unitreerobotics/unitree_sdk2.git
Tag: 2.0.2
Commit: 811bc77
```

Initialize the submodule:

```bash
git submodule update --init --recursive
```

Build with Unitree enabled, which is the default:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If the SDK is unavailable, CMake disables only the Unitree adapter targets and
continues building the vendor-independent framework.

The SDK-free Unitree G1 plugin skeleton under `plugins/unitree/g1` is still
built when `ENABLE_UNITREE=OFF`.

Manual SDK override:

```bash
cmake -S . -B build -DUNITREE_SDK2_ROOT=/path/to/unitree_sdk2
```

## Examples

Basic framework initialization:

```bash
./build/examples/humanoid_core_basic_initialization
```

Plugin registry and factory loading example:

```bash
./build/examples/humanoid_core_plugin_loading_example
```

Full composition example using plugin factory, Unitree plugin skeleton,
`RobotStateManager`, and `TelemetryService`:

```bash
./build/examples/humanoid_core_framework_integration_example
```

Factory-based robot connection example:

```bash
./build/examples/humanoid_core_basic_robot_connection config/robot.yaml
./build/examples/humanoid_core_robot_connection_example config/robot.yaml
```

By default, the connection example validates configuration, registers available
factories, creates the adapter through the registry, and exits without
opening physical robot communication. To execute the read-only physical
communication workflow:

```bash
./build/examples/humanoid_core_basic_robot_connection config/robot.yaml --execute
./build/examples/humanoid_core_robot_connection_example config/robot.yaml --execute
```

The read-only hardware workflow is:

```text
Initialize -> Connect -> Disconnect -> Shutdown
```

Telemetry example:

```bash
./build/examples/humanoid_core_telemetry_example
```

Capability query example:

```bash
./build/examples/humanoid_core_capability_query_example
```

Command framework examples:

```bash
./build/examples/humanoid_core_command_execution_example
./build/examples/humanoid_core_command_queue_example
./build/examples/humanoid_core_command_cancellation_example
./build/examples/humanoid_core_capability_validation_example
```

When Unitree SDK2 is available and `ENABLE_UNITREE=ON`, the SDK-boundary command
adapter validation example is also built:

```bash
./build/examples/humanoid_core_unitree_sdk_boundary_example
```

## Configuration

`config/robot.yaml` uses the current robot schema:

```yaml
robot:
  vendor: Unitree
  model: G1
  ip: 192.168.123.161
  network_interface: eth0
  timeout_ms: 500
  domain_id: 0
  serial_number:
  firmware:
```

The core configuration module still exposes interfaces only. There is no general
YAML parser in the framework.

## Tests

The smoke test is always built when `BUILD_TESTING` is enabled and does not
require a physical robot:

```bash
ctest --test-dir build --output-on-failure
```

GoogleTest tests are built when GoogleTest is available. To require GoogleTest:

```bash
cmake -S . -B build -DHUMANOID_CORE_REQUIRE_GTEST=ON
```

Milestone 3 state and telemetry coverage is provided by the always-built
`humanoid_core_robot_state_unit_test` CTest target. It validates state defaults,
state updates, reset behavior, concurrent manager access, telemetry subscriber
callbacks, unsubscribe behavior, invalid telemetry startup, and a bounded
state-manager performance sanity check.

The always-built `humanoid_core_command_model_unit_test` validates command
defaults, identity and timeout rules, typed payload values, metadata, lifecycle
results, and stable enum names without requiring robot hardware.

The always-built `humanoid_core_planner_goal_model_unit_test` validates goal
defaults, minimum validity rules, typed constraints and context, planning
request capability input, move-only planning results, diagnostics, and stable
enum names without requiring robot hardware.

The always-built `humanoid_core_planner_interface_unit_test` validates the
abstract planner interface, dependency-injected factory and registry,
capability lookup, active-instance tracking, invalid creator rejection, and
concurrent registry access without adding a concrete production planner.

The always-built `humanoid_core_execution_context_unit_test` validates runtime
identity, scope and state values, timestamps, current-step tracking, metadata
replacement, cancellation token propagation, snapshots, and concurrent access.

The always-built `humanoid_core_blackboard_unit_test` validates typed values,
namespace isolation, shared lifetime, replacement, remove and clear operations,
plus stress access from concurrent readers and writers.

The always-built `humanoid_core_command_dispatcher_unit_test` validates command
forwarding, payload rejection, timeout and exception translation, asynchronous
priority, cancellation, duplicate IDs, and concurrent shutdown without robot
hardware.

The always-built `humanoid_core_command_queue_unit_test` validates bounded
capacity, priority/FIFO ordering, timeout, cancellation, statistics, and stress
execution with concurrent producers and consumers.

The always-built `humanoid_core_safety_validator_unit_test` validates
vendor-independent connection, emergency-stop, fault, battery, posture, and
capability safety rejection rules.

The always-built `humanoid_core_command_execution_pipeline_unit_test` validates
execution IDs, lifecycle callbacks, cancellation, shutdown, metrics, bounded
history, timeout, exception propagation, logging, and concurrent submissions.

The always-built `humanoid_core_mission_model_unit_test` validates mission
defaults, embedded command steps, metadata, enabled-step validity rules,
mission status names, terminal status detection, and mission results.

The always-built `humanoid_core_mission_executor_unit_test` validates mission
start, pause, resume, cancel, stop, current-step tracking, retry behavior, and
step execution through `CommandDispatcher`.

The always-built `humanoid_core_mission_loader_unit_test` validates JSON and
YAML loading, invalid YAML rejection, missing required fields, unknown command
rejection, and file-extension dispatch.

API-level documentation:

- `docs/api/command_model.md`
- `docs/api/behavior_tree_core.md`
- `docs/api/blackboard.md`
- `docs/api/cancellation.md`
- `docs/api/execution_context.md`
- `docs/api/mission_model.md`
- `docs/api/planner_goal_model.md`
- `docs/api/plugin_integration.md`
- `docs/api/resource_manager.md`
- `docs/api/robot_state_and_telemetry.md`
- `docs/api/runtime_examples.md`
- `docs/api/runtime_scheduler.md`
- `docs/Milestone_7_Report.md`
- `docs/Milestone_6_Report.md`
- `docs/services/telemetry_service.md`

## Install and Export

```bash
cmake --install build --prefix install
```

Installed consumers can link the exported package:

```cmake
find_package(humanoid_core CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE humanoid::humanoid_core)
```

When the Unitree adapter was built, consumers must make the pinned SDK available
through `UNITREE_SDK2_ROOT` or the same repository submodule layout.

Installed CMake package metadata exposes:

```cmake
HUMANOID_CORE_VERSION
HUMANOID_CORE_VERSION_PRERELEASE
HUMANOID_CORE_SEMANTIC_VERSION
```

## Updating SDK Version

To switch SDK versions:

```bash
git -C third_party/unitree_sdk2 fetch --tags
git -C third_party/unitree_sdk2 checkout <tag-or-commit>
git add third_party/unitree_sdk2 third_party/unitree_sdk2.version
```

Update `third_party/unitree_sdk2.version` with the exact tag and commit. Do not
track a floating branch head for production builds.

## Adding New Robot Vendors

Add new vendors without modifying application code:

1. Implement `humanoid::adapters::IRobotAdapter`.
2. Implement `humanoid::adapters::IRobotFactory`.
3. Hide vendor SDK headers inside a vendor SDK abstraction boundary.
4. Register the factory with `RobotFactoryRegistry`.
5. Keep application code dependent only on `IRobotFactory`, `IRobotAdapter`, and
   manager interfaces.

## Coding Style

- C++20 for framework code. Vendor SDK wrapper translation units may use a
  vendor-compatible dialect when required to compile official SDK headers.
- LLVM formatting.
- Doxygen comments on public headers, classes, and functions.
- RAII for ownership and cleanup.
- Smart pointers for injected interfaces.
- `enum class` for scoped enumerations.
- `std::chrono` for time.
- `std::filesystem` for paths.
- No singletons.
- No global variables.
- No `using namespace std`.
- No raw owning pointers.
- No vendor SDK includes outside SDK wrappers.

## Development Workflow

See `CONTRIBUTING.md` for formatting, static analysis, pre-commit hooks, local
CI reproduction, and contribution rules.

Repository governance:

- Use GitHub issue templates for bugs, feature requests, and architecture
  changes.
- Use the pull request template checklist before requesting review.
- Follow CODEOWNERS review routing for public APIs, build rules, vendor
  adapters, and documentation.
- Report security issues through `SECURITY.md`, not public issues.

## Versioning Policy

humanoid-core uses Semantic Versioning. Current version: `0.8.0-alpha`.

Release tags use:

```text
vMAJOR.MINOR.PATCH[-PRERELEASE]
```

See `docs/versioning.md` and `docs/release_checklist.md` for release process
details.

## Issue and Security Reporting

- Bugs: use `.github/ISSUE_TEMPLATE/bug_report.md`.
- Feature requests: use `.github/ISSUE_TEMPLATE/feature_request.md`.
- Architecture changes: use `.github/ISSUE_TEMPLATE/architecture_change.md`.
- Vulnerabilities or unsafe robot-control issues: follow `SECURITY.md`.

Production hardening documentation:

- `docs/Production_Hardening_Report.md`
- `docs/Repository_Governance_Report.md`
- `docs/Milestone_5_Report.md`
- `docs/Milestone_4_Report.md`
- `docs/integration/Milestone_4_8_Integration_Report.md`
- `docs/architecture/plugin_architecture.md`
- `docs/thread_safety.md`
- `docs/dependency_graph.md`
- `docs/security_review.md`
- `docs/adr/`
