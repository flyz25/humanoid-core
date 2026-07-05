# Planner Goal Model

Milestone 9.1 defines the vendor-independent value types used to describe user
intent before any future planning implementation exists. It does not add a
planner engine, LLM integration, robot adapter, SDK integration, mission
execution policy, or behavior tree execution policy.

## Public Headers

```cpp
#include <humanoid/planner/Goal.h>
#include <humanoid/planner/GoalPriority.h>
#include <humanoid/planner/GoalStatus.h>
#include <humanoid/planner/GoalType.h>
#include <humanoid/planner/IPlanner.h>
#include <humanoid/planner/PlannerFactory.h>
#include <humanoid/planner/PlannerRegistry.h>
#include <humanoid/planner/PlanningRequest.h>
#include <humanoid/planner/PlanningResult.h>
#include <humanoid/planner/RuleBasedPlanner.h>
```

All types are in `humanoid::planner` and are also available through the
`<humanoid/core.hpp>` umbrella header.

## Goal

`Goal` is a framework-owned value type containing:

- A producer-assigned 64-bit goal ID. Zero means unassigned.
- A vendor-independent `GoalType`.
- A human-readable intent description.
- A `GoalPriority` planning urgency.
- A `GoalStatus` model lifecycle state.
- Named goal constraints.
- Named situational context values.
- String metadata for correlation and tracing.
- A monotonic `std::chrono::steady_clock` timestamp.

Goal producers are responsible for assigning unique nonzero IDs within their
planning domain and recording timestamps. The model intentionally does not use a
global ID generator, wall-clock time, robot SDK handles, or LLM client state.

`Goal::isValid()` checks only the minimum model requirement: assigned ID and
non-empty description. Deeper semantic validation belongs to a future planner
or application policy layer.

## Constraints, Context, and Metadata

`GoalConstraints` and `GoalContext` are ordered maps from string keys to one of:

- `bool`
- `std::int64_t`
- `double`
- `std::string`

The ordered representation gives deterministic traversal for diagnostics,
serialization boundaries, and tests. These maps must contain framework-owned
data only. SDK objects, vendor enums, transport handles, pointers, credentials,
and opaque planner client objects are forbidden.

`GoalMetadata` is an ordered string map for correlation IDs, source labels, and
tracing annotations. Runtime behavior must not silently depend on metadata.

## Planning Request

`PlanningRequest` combines:

- `Goal`
- `RuntimeContext`, defined as a copyable `ExecutionContextSnapshot`
- `humanoid::core::RobotCapabilities`

The runtime context is captured as a snapshot so the request remains copyable and
does not give model code ownership over a live runtime object. Robot capability
data is declarative and vendor independent.

## Planning Result

`PlanningResult` can carry:

- Optional `humanoid::mission::Mission`
- Optional owned `humanoid::bt::BehaviorTree`
- Ordered `PlanningDiagnostics`
- A final `GoalStatus`

`BehaviorTree` output is owned by `std::unique_ptr` because behavior trees own
node graphs and are intentionally non-copyable. `PlanningResult` is therefore
move-only.

`PlanningResult::isSuccess()` is true only when status is `GoalStatus::Planned`
and at least one executable artifact is present.

## Dependency Boundary

```text
Application or future goal producer
  -> humanoid::planner goal model
    -> ExecutionContextSnapshot
    -> RobotCapabilities
    -> Mission / BehaviorTree result models
```

The planner goal model contains no Unitree SDK headers, robot SDK headers, LLM
SDK headers, OpenAI API integration, Claude API integration, parser
implementation, planning engine, mission execution, behavior tree execution, or
robot communication.

## Planner Interface

Milestone 9.2 adds the abstract planner boundary and discovery infrastructure:

```text
Application or future planner host
  -> IPlanner
  -> PlannerFactory
    -> injected PlannerRegistry
      -> PlannerCapabilities
```

`IPlanner` is a pure abstract interface with:

- `Plan()`
- `ValidatePlan()`
- `CancelPlan()`
- `GetCapabilities()`

The interface contains no concrete rule planner, LLM planner, symbolic planner,
robot adapter, robot SDK, LLM SDK, OpenAI API, Claude API, parser, or execution
engine. Implementations are future outer-layer components that must translate
their internal failures into `PlanningResult` diagnostics or `Status` values.

`PlannerCapabilities` describes planner identity, implementation family, goal
types, output types, validation support, and cancellation support. The
implementation family can describe rule, LLM, symbolic, or custom planners
without exposing concrete classes.

`PlannerRegistry` stores capability metadata behind `std::shared_mutex`.
Readers use shared locks and writes use exclusive locks. The registry owns no
planner instances and performs no dynamic loading.

`PlannerFactory` stores creator callables behind `std::shared_mutex`, delegates
capability discovery to an injected `PlannerRegistry`, tracks active instances,
and refuses unregistration while created planners are still active. It is
dependency-injection friendly, owns no global state, and is not a singleton.

## Rule-Based Planner

Milestone 9.3 adds `RuleBasedPlanner`, the first deterministic planner
implementation. It is static and contains no AI, LLM, OpenAI API, Claude API,
robot SDK, adapter call, mission execution, or behavior tree runtime execution.

The planner evaluates an ordered `RuleBasedPlannerRule` set:

- Match by `GoalType`.
- Require `GoalPriority` to be at least the rule's minimum priority.
- Select the matching rule with the highest minimum priority.
- Preserve insertion order for ties.
- Use the first fallback rule when no normal rule matches.

Each selected rule produces:

- A `Mission` containing deterministic `MissionStep` command values.
- A `BehaviorTree` with deterministic success leaf nodes representing the
  generated plan structure.
- Planner diagnostics identifying whether a normal rule or fallback rule was
  selected.

Generated command priorities are derived directly from goal priority. Command
and mission identifiers are derived from the goal identifier and contain no
global generator.

`RuleBasedPlanner::ValidatePlan()` requires `GoalStatus::Planned`, a valid
mission, and a behavior tree. It intentionally does not execute the mission or
tick the tree.

## Planning Pipeline

Milestone 9.5 adds `PlanningPipeline`, the vendor-independent orchestration
boundary from goal planning to optional runtime handoff:

```cpp
#include <humanoid/planner/PlanningPipeline.h>
```

```text
PlanningRequest
  -> PlanningPipeline
    -> primary IPlanner
    -> optional fallback IPlanner
    -> PlanningResult diagnostics
    -> optional BehaviorTreeRuntime submission
```

`PlanningPipelineOptions` controls plan validation, fallback use, and optional
behavior tree runtime submission. Runtime submission transfers ownership of the
generated `BehaviorTree` to the injected `BehaviorTreeRuntime`; the returned
`PlanningPipelineResult` retains mission output, diagnostics, status, fallback
metadata, and the optional runtime job handle.

`PlanningPipelineMetrics` is a thread-safe snapshot containing request,
rejection, planner-attempt, fallback, validation-failure, failure, runtime
submission, and runtime-submission-failure counters.

The pipeline is dependency-injection friendly and owns no global state. It does
not implement a provider, HTTP transport, LLM client, robot adapter, robot SDK,
mission executor, runtime scheduler, behavior tree node factory, or concrete
planning policy. Those dependencies remain behind their existing interfaces and
are supplied by the application composition root.

## Planning Examples

Milestone 9.6 adds the `humanoid_core_planning_examples` application under
`examples/planning_examples`. It demonstrates the pipeline as application-layer
composition:

```text
Goal
  -> PlanningPipeline
    -> RuleBasedPlanner
      -> Mission
      -> BehaviorTree
    -> BehaviorTreeRuntime
      -> RuntimeScheduler
```

The example scenarios are Greeting, Flag Ceremony, Inspection, and Stage Demo.
They are intentionally hardware-free and use deterministic rules only. The
examples do not add provider SDKs, HTTP clients, robot SDKs, robot hardware
communication, motion execution, mission-engine bypasses, or application
business logic to the planner module.

## Planner Framework Release

Milestone 9.7 releases the planner framework API in humanoid-core
`0.9.0-alpha`. The public planner surface includes the goal model, planner
interface, planner registry, planner factory, deterministic rule planner, LLM
provider abstraction, planning pipeline, and runnable planning examples. The
release remains provider-neutral and vendor-independent; concrete LLM providers
and robot execution stay outside the planner module.
