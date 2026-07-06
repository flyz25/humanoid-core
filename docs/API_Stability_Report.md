# API Stability Report

## Release

humanoid-core `1.0.0` is the first stable public release.

## Stable Public API Surface

The following public interfaces are considered stable within major version 1:

- Core robot state, telemetry, command, safety, runtime, and adapter contracts.
- Plugin metadata, registry, factory, and lifecycle contracts.
- Mission model, parser, validator, executor, flow-control, events, and
  conditions.
- Behavior tree node, tree, composite, decorator, leaf, loader, and runtime
  integration contracts.
- Planner goal model, planner interface, rule-based planner, LLM provider
  abstraction, and planning pipeline contracts.
- Perception sensor, manager, pipeline, inference, detection, and fusion
  contracts.
- Optional ROS2 bridge DTOs, catalogs, launch assets, and interface assets.
- Optional cloud platform contracts, fleet manager, auth manager, OTA manager,
  observability registry, and deployment metadata.

## Compatibility Rules

- Public headers under `include/`, `common/include/`, `plugins/include/`,
  `ros2/include/`, and `cloud/include/` must not introduce breaking changes in
  the 1.x line without a deprecation window.
- Vendor SDK types must never appear in public framework APIs.
- Optional integration layers must remain removable through CMake options.
- New concrete robot vendors must be added through plugins or adapters without
  application-layer API changes.

## Allowed Non-Breaking Changes

- Adding new enum values when callers are expected to handle unknown/default
  cases.
- Adding new optional fields to value types when defaults preserve behavior.
- Adding overloads, helpers, examples, tests, and documentation.
- Adding optional targets guarded by CMake options.

## Breaking Change Process

Breaking changes require an architecture issue, CODEOWNERS review, migration
guide updates, changelog entry, and major-version release.
