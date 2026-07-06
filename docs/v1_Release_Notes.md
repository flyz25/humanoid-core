# humanoid-core v1.0.0 Release Notes

humanoid-core `v1.0.0` is the first stable production release of the
vendor-independent humanoid robotics framework.

## Highlights

- Clean Architecture core framework.
- Robot state and telemetry services.
- Plugin architecture and factory registry.
- Unitree SDK2 isolation through adapter and SDK wrapper boundaries.
- Command framework with dispatch, queueing, safety validation, and execution
  lifecycle.
- Mission framework with loading, validation, flow control, events, and
  execution.
- Execution runtime with context, blackboard, resource locking, cancellation,
  and scheduler.
- Behavior tree framework with composite, decorator, leaf, loader, and runtime
  integration.
- Planner framework with goal model, planner interface, rule-based planner, LLM
  provider abstraction, and planning pipeline.
- Perception framework with sensor abstraction, manager, pipeline, inference,
  detection, and fusion interfaces.
- Optional ROS2 ecosystem integration.
- Optional cloud and fleet platform contracts.
- Production hardening, repository governance, CI/CD, documentation, and
  release validation.

## Upgrade Notes

Projects using `0.12.0-alpha` should update version constraints to `1.0.0`.
No public API rename is required for the v1 release.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix install
```

Optional integrations can be controlled with:

```bash
-DENABLE_UNITREE=ON|OFF
-DENABLE_ROS2=ON|OFF
-DENABLE_CLOUD=ON|OFF
```

## Release Artifacts

- Source repository tag: `v1.0.0`
- CPack TGZ package.
- Docker deployment metadata image.
- Doxygen HTML documentation.
- SPDX JSON SBOM.
