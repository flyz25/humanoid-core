# Changelog

All notable changes to humanoid-core are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Milestone 4.1 Plugin Infrastructure: added plugin interfaces, lifecycle
  states, version compatibility metadata, a thread-safe plugin registry,
  plugin tests, and the plugin architecture document.

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

[Unreleased]: https://github.com/humanoid-core/humanoid-core/compare/v0.3.0-alpha...HEAD
[0.3.0-alpha]: https://github.com/humanoid-core/humanoid-core/compare/v0.1.0-alpha...v0.3.0-alpha
[0.1.0-alpha]: https://github.com/humanoid-core/humanoid-core/releases/tag/v0.1.0-alpha
