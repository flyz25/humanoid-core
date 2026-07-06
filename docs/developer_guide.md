# Developer Guide

## Purpose

This guide complements `CONTRIBUTING.md` with release engineering and hardware
validation practices for humanoid-core maintainers.

## Local Quality Checks

Use the standard build, test, static-analysis, and formatting commands
documented in `CONTRIBUTING.md`.

## Hardware Validation Program

The Hardware Validation Program lives under `validation/`. It is used after
software validation when a human operator is ready to execute physical robot
tests.

List validation cases:

```bash
python3 validation/scripts/hvp.py list-cases
```

Create a run:

```bash
python3 validation/scripts/hvp.py new-run --operator "<name>"
```

Record a result:

```bash
python3 validation/scripts/hvp.py record \
  --run-dir validation/reports/runs/<run-id> \
  --case-id HVP-CONN-001 \
  --status PASS \
  --operator "<name>" \
  --notes "<evidence summary>"
```

Generated validation runs are local evidence artifacts and are ignored by Git.

## Hardware Claims

Maintainers must not claim physical validation passed unless an operator-run
report contains evidence and sign-off. Tooling validation alone is not hardware
validation.

## Unitree G1 Bring-Up Preparation

Before connecting a physical Unitree G1, validate both build modes:

```bash
cmake -S . -B build-debug-off -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=OFF
cmake --build build-debug-off --parallel
ctest --test-dir build-debug-off --output-on-failure

cmake -S . -B build-debug-on -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=ON
cmake --build build-debug-on --parallel
ctest --test-dir build-debug-on --output-on-failure
```

Run integration examples without opening physical communication:

```bash
./build-debug-on/examples/humanoid_core_plugin_loading_example
./build-debug-on/examples/humanoid_core_capability_query_example
./build-debug-on/examples/humanoid_core_framework_integration_example
./build-debug-on/examples/humanoid_core_robot_connection_example config/robot.yaml
```

Only an operator should attempt physical communication, and only with the
explicit execution flag:

```bash
./build-debug-on/examples/humanoid_core_robot_connection_example config/robot.yaml --execute
```

If SDK initialization reports route netlink socket or network-interface access
errors in WSL2, containers, or restricted sandboxes, treat that as an
environment limitation. It is not evidence that the robot failed hardware
validation.
