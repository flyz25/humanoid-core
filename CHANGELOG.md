# Changelog

All notable changes to humanoid-core are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/humanoid-core/humanoid-core/compare/v0.5.0-alpha...HEAD
[0.5.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.4.0-alpha...v0.5.0-alpha
[0.4.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.3.0-alpha...v0.4.0-alpha
[0.3.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.1.0-alpha...v0.3.0-alpha
[0.1.0-alpha]: https://github.com/humanoid-core/humanoid-core/releases/tag/v0.1.0-alpha
