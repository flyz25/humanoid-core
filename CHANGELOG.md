# Changelog

All notable changes to humanoid-core are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.9.0-alpha] - 2026-07-06

### Added

- Milestone 9.1 Goal Model: added vendor-independent planner goal, goal type,
  priority, status, planning request, planning result, and diagnostics value
  types, plus API documentation and always-built validation coverage.
- Milestone 9.2 Planner Interface: added the pure abstract planner interface,
  planner capability metadata, thread-safe planner registry, dependency-injected
  planner factory, and validation coverage without adding concrete planner
  implementations.
- Milestone 9.3 Rule-Based Planner: added a deterministic static planner with
  rule matching, priority-specific rule selection, fallback planning,
  mission/behavior-tree output generation, cancellation handling, factory
  integration, documentation, and validation coverage without AI or LLM
  dependencies.
- Milestone 9.4 LLM Provider Interface: added provider-neutral LLM request,
  response, capabilities, and pure abstract provider contracts for future
  OpenAI, Anthropic, Gemini, Ollama, local, and custom adapters without provider
  SDKs, HTTP implementation, or concrete providers.
- Milestone 9.5 Planning Pipeline: added a vendor-independent planning
  orchestration layer with request validation, primary planner execution,
  optional fallback planning, plan validation, diagnostics, optional logging,
  metrics, optional behavior tree runtime handoff, and hardware-free validation
  coverage without provider implementations or SDK dependencies.
- Milestone 9.6 Planning Examples: added runnable hardware-free examples for
  Greeting, Flag Ceremony, Inspection, and Stage Demo scenarios that create
  goals, produce missions and behavior trees through the planning pipeline, and
  execute generated trees through the existing runtime.
- Milestone 9.7 Planner Framework Release: completed planner framework
  documentation, release metadata, validation report, build matrix, install
  validation, package validation, and release readiness for `v0.9.0-alpha`.

## [0.8.0-alpha] - 2026-07-05

### Added

- Milestone 8.1 Behavior Tree Core: added vendor-independent behavior tree
  status, node, runtime context, tree lifecycle, node factory registration,
  thread-safe lifecycle validation, and API documentation on top of the
  execution runtime.
- Milestone 8.2 Composite Nodes: added vendor-independent sequence, selector,
  memory sequence, memory selector, and parallel behavior tree composites with
  nested composite validation, concurrent parallel tick validation, runtime
  cancellation handling, and updated documentation.
- Milestone 8.3 Decorator Nodes: added vendor-independent inverter, repeat,
  retry, succeeder, failer, limiter, and timeout behavior tree decorators with
  retry, timeout, loop, limit, cancellation, and documentation coverage.
- Milestone 8.4 Action & Condition Nodes: added reusable behavior tree leaf
  nodes for injected actions, runtime-context conditions, wait predicates,
  timed delays, command dispatch through `CommandDispatcher`, and mission
  execution through `MissionExecutor`, with hardware-free validation coverage.
- Milestone 8.5 Behavior Tree Loader: added JSON and YAML behavior tree
  parsing, validation against registered node types, recursive tree
  construction through `BehaviorTreeFactory`, optional XML rejection, file
  loading, and invalid/missing/unknown-node validation coverage.
- Milestone 8.6 Runtime Integration: added dependency-injected behavior tree
  execution through the shared runtime scheduler, execution context,
  blackboard, cancellation framework, and resource manager, with concurrent
  tree, multiple-runtime, resource exclusion, and cancellation validation.
- Milestone 8.7 Behavior Tree Examples: added runnable hardware-free Greeting,
  Flag Ceremony, Inspection, and Patrol applications demonstrating sequence,
  selector, retry, parallel, mission, and command nodes through existing
  framework boundaries.
- Milestone 8.8 Behavior Tree Release: completed Behavior Tree Framework
  integration, release documentation, version metadata, build matrix, examples,
  install, and package validation.

## [0.7.0-alpha] - 2026-07-05

### Added

- Milestone 7.1 Execution Runtime Context: added vendor-independent execution
  IDs, scope, lifecycle state, cooperative cancellation, metadata, coherent
  stored-state snapshots, thread-safe mutable runtime state, tests, and API
  documentation.
- Milestone 7.2 Runtime Blackboard: added namespaced exact-type storage,
  immutable shared ownership, concurrent reads and writes, replacement,
  removal, clear operations, stress coverage, and API documentation.
- Milestone 7.3 Runtime Resource Manager: added vendor-independent shared and
  exclusive logical resource leases, move-only RAII locks, opaque handles,
  non-blocking acquisition, timed acquisition, release by handle, concurrent
  ownership validation, timeout coverage, and API documentation.
- Milestone 7.4 Cancellation Framework: added framework-wide cancellation
  sources, tokens, RAII callback registrations, linked nested cancellation,
  callback exception containment, ExecutionContext runtime-token bridging,
  concurrent cancellation coverage, and API documentation.
- Milestone 7.5 Runtime Scheduler: added vendor-independent runtime job
  scheduling with bounded priority/FIFO queueing, parallel and sequential
  dispatch, cooperative pause/resume, cooperative stop, lifecycle snapshots,
  scheduler statistics, concurrent submission coverage, and API documentation.
- Milestone 7.6 Runtime Integration Examples: added runnable hardware-free
  examples for `ExecutionContext`, `Blackboard`, `ResourceManager`,
  `CancellationSource`, and `RuntimeScheduler`, plus example documentation.
- Milestone 7.7 Execution Runtime Release: completed runtime framework release
  documentation, version metadata, install/package validation, and the full
  Debug/Release and Unitree-enabled/disabled validation matrix.

## [0.6.0-alpha] - 2026-07-05

### Added

- Milestone 6.1 Mission Model: added vendor-independent mission, mission step,
  mission status, mission result, and mission metadata value types with
  hardware-free validation coverage and API documentation.
- Milestone 6.2 Mission Executor: added a thread-safe mission executor that
  starts, pauses, resumes, cancels, stops, tracks current step state, executes
  mission steps through `CommandDispatcher`, and validates lifecycle behavior
  without robot hardware.
- Milestone 6.3 Mission Loader: added dependency-injected mission loading,
  strict JSON and YAML mission parsing, schema validation, unknown-command
  rejection, file-extension dispatch, and hardware-free loader tests.
- Milestone 6.4 Flow Control: added vendor-independent wait, delay, retry,
  loop, timeout, skip, and abort mission flow-control types; integrated them
  into mission validation, parsing, and executor behavior with nested retry,
  loop, timeout, skip, and abort tests.
- Milestone 6.5 Events and Conditions: added mission conditions, mission
  events, and a RobotStateManager-backed condition evaluator for battery,
  connection, robot state, capability, fault, and emergency-stop checks;
  integrated condition skip/abort behavior into mission execution and mission
  document parsing with hardware-free tests.
- Milestone 6.6 Examples: added runnable YAML mission examples for normal
  execution, pause/resume, and cancellation through `MissionLoader`,
  `MissionExecutor`, `CommandDispatcher`, and a process-local example adapter.
- Milestone 6.7 Mission Framework Release: completed mission framework
  integration, release documentation, package metadata, and the full
  Debug/Release and Unitree-enabled/disabled validation matrix.

## [0.5.0-alpha] - 2026-07-04

### Added

- Milestone 5.1 Generic Command Model: added vendor-independent command type,
  priority, lifecycle status, result, payload, metadata, monotonic timestamp,
  timeout, and command identity value types with API documentation and
  hardware-free validation coverage.
- Milestone 5.2 Command Dispatcher: added dependency-injected synchronous and
  asynchronous command forwarding, command and payload validation,
  priority-aware queueing, queued-command cancellation, timeout and adapter
  failure translation, serialized adapter access, controlled shutdown, and
  hardware-free dispatcher tests.
- Milestone 5.3 Command Queue: added a bounded asynchronous command queue with
  priority/FIFO scheduling, configurable `std::jthread` consumers,
  condition-variable waiting, timeout enforcement, cancellation, duplicate-ID
  rejection, queue statistics, dispatcher integration, and concurrent
  producer/consumer stress coverage.
- Milestone 5.4 Safety Layer: added vendor-independent command safety
  validation for connection state, emergency stop, command capabilities,
  battery thresholds, robot faults, posture state, dispatcher integration, and
  hardware-free safety tests.
- Milestone 5.5 Execution Pipeline: added vendor-independent command execution
  lifecycle infrastructure with execution IDs, queued/running/terminal
  callbacks, optional logging, metrics, bounded execution history, concurrent
  worker execution, cancellation, timeout handling, and error propagation tests.
- Milestone 5.6 Integration & Validation: completed command framework
  integration, added runnable command execution, queue, cancellation, and
  capability-validation examples, refreshed architecture and API documentation,
  and prepared the Milestone 5 validation report.

## [0.4.0-alpha] - 2026-07-04

### Added

- Milestone 4.1 Plugin Infrastructure: added plugin interfaces, lifecycle
  states, version compatibility metadata, a thread-safe plugin registry,
  plugin tests, and the plugin architecture document.
- Milestone 4.2 RobotAdapter Interface: added the vendor-independent core robot
  adapter contract, robot information and capability metadata, and interface
  validation coverage.
- Milestone 4.3 Plugin Registry & Factory: added the thread-safe plugin
  factory, explicit plugin enumeration API, dependency-injected creator
  registration, lifecycle-aware instance destruction, and factory validation
  coverage.
- Milestone 4.4 Unitree G1 Plugin Skeleton: added the SDK-free Unitree G1
  plugin package, plugin-local adapter skeleton, manifest, factory registration
  helper, mock state validation, and build coverage for Unitree-enabled and
  Unitree-disabled configurations.
- Milestone 4.5 SDK Abstraction Layer: added the Unitree SDK abstraction
  boundary under `plugins/unitree/sdk`, normalized SDK result/state conversion,
  SDK converter tests, and refactored the legacy locomotion wrapper so Unitree
  SDK2 headers are included only by the SDK abstraction implementation.
- Milestone 4.6 SDK2 Communication: added read-only SDK2 heartbeat monitoring,
  timeout handling, automatic reconnect attempts, cached state synchronization,
  Linux runtime preflight before SDK client construction, optional
  `RobotStateManager` injection for Unitree adapters, and validation of
  communication type defaults without requiring physical robot hardware.
- Milestone 4.7 Motion, Hand & Audio Adapters: added thread-safe Unitree SDK2
  `LocoAdapter`, `HandAdapter`, and `AudioAdapter` command translation layers,
  refactored `SdkWrapper` to delegate locomotion commands, and added
  hardware-free adapter validation tests.
- Milestone 4.8 Integration & Examples: added buildable integration examples
  for plugin loading, full framework composition, robot connection, telemetry,
  capability queries, and Unitree SDK-boundary command-adapter validation, plus
  the Milestone 4.8 integration report.
- Milestone 4.9 Validation & Release: completed full Milestone 4 validation,
  refreshed release documentation, and added the Milestone 4 completion report.

## [0.3.0-alpha] - 2026-07-04

### Added

- Milestone 3.1 Robot State Model: vendor-independent, allocation-free core
  state snapshot with connection, power, motion, velocity, pose, orientation,
  health, and timestamp fields.
- Milestone 3.2 Robot State Manager: thread-safe latest-state cache using
  `std::shared_mutex` with shared readers and exclusive writers.
- Milestone 3.3 Telemetry Service: vendor-independent periodic robot state
  publisher with callback subscription, multiple listeners, and `std::jthread`
  lifecycle management.
- Milestone 3.4 Integration: wired `RobotStateManager` through `CoreContext`
  and validated `TelemetryService` construction from injected framework state.
- Milestone 3.5 Testing & Documentation: added always-built state and telemetry
  unit coverage, API documentation, architecture updates, and the final
  Milestone 3 report.
- Repository governance issue templates.
- Pull request template.
- CODEOWNERS ownership map.
- Security policy.
- Release checklist.
- License header template documentation.

## [0.1.0-alpha] - 2026-07-04

### Added

- Milestone 1: Clean Architecture framework skeleton, module layout, managers,
  interfaces, logging/configuration abstractions, examples, tests, CMake, and
  documentation.
- Milestone 2: Unitree SDK2 submodule integration, Unitree G1 communication
  adapter, SDK wrapper, robot factory, factory registry, robot configuration,
  example workflow, and smoke tests.
- Architecture Audit: SDK isolation review, exception-safety hardening,
  thread-safe logger manager, dependency cleanup, and validation report.
- Production Hardening: static-analysis configuration, pre-commit hooks,
  GitHub Actions CI matrix, ADRs, thread-safety documentation, dependency graph,
  security review, install-layout hardening, and production hardening report.
- Repository Governance: contribution governance, security disclosure policy,
  release process, issue templates, pull request template, CODEOWNERS, and
  semantic-versioning policy.

[Unreleased]: https://github.com/humanoid-core/humanoid-core/compare/v0.9.0-alpha...HEAD
[0.9.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.8.0-alpha...v0.9.0-alpha
[0.8.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.7.0-alpha...v0.8.0-alpha
[0.7.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.6.0-alpha...v0.7.0-alpha
[0.6.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.5.0-alpha...v0.6.0-alpha
[0.5.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.4.0-alpha...v0.5.0-alpha
[0.4.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.3.0-alpha...v0.4.0-alpha
[0.3.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.1.0-alpha...v0.3.0-alpha
[0.1.0-alpha]: https://github.com/humanoid-core/humanoid-core/releases/tag/v0.1.0-alpha
