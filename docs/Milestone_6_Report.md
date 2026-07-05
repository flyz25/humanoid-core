# Milestone 6 Report

## Summary

Milestone 6 completes the vendor-independent mission framework for
humanoid-core `0.6.0-alpha`.

The release preserves the existing dependency direction:

```text
Application
  -> MissionLoader -> MissionParser -> MissionValidator -> Mission
  -> MissionExecutor
    -> ConditionEvaluator -> RobotStateManager
    -> CommandDispatcher -> SafetyValidator -> IRobotAdapter
```

Mission code contains no vendor SDK headers and never bypasses the command or
safety framework.

## Delivered Milestones

- Milestone 6.1: mission, step, status, result, and metadata value models.
- Milestone 6.2: thread-safe asynchronous mission executor and lifecycle.
- Milestone 6.3: strict JSON/YAML loading, parsing, and schema validation.
- Milestone 6.4: wait, delay, retry, loop, timeout, skip, and abort policies.
- Milestone 6.5: generic robot-state conditions and condition events.
- Milestone 6.6: runnable execute, pause/resume, and cancel examples.
- Milestone 6.7: integrated validation, documentation, package metadata, and
  release preparation.

## Architecture Validation

- Mission models depend only on framework-owned command and standard-library
  value types.
- `MissionExecutor` forwards command steps only through injected
  `CommandDispatcher`.
- `ConditionEvaluator` reads runtime state only through injected
  `RobotStateManager` and accepts generic capability metadata.
- `MissionExecutor` does not depend on file formats; loading and parsing remain
  separate responsibilities.
- Mission components contain no concrete adapter, plugin, SDK wrapper, or
  vendor SDK dependency.
- Worker and synchronization resources are lifecycle-managed without
  singleton or global state.

## Functional Validation

The automated suite verifies:

- Mission model validity and status semantics.
- JSON/YAML loading, schema failures, and unknown command rejection.
- Start, execution, pause, resume, cancel, stop, and current-step tracking.
- Retry recovery, nested retry inside loops, and loop iteration counts.
- Wait/delay execution, timeout failure, skip, and abort behavior.
- Battery, connection, posture, capability, fault, and emergency-stop
  condition evaluation.
- Condition-controlled execution, skip, and abort behavior.

## Build Matrix

All configurations use `HUMANOID_CORE_WARNINGS_AS_ERRORS=ON`.

| Configuration | Configure | Build | CTest |
| --- | --- | --- | --- |
| Debug, `ENABLE_UNITREE=ON` | Passed | Passed | 17/17 passed |
| Release, `ENABLE_UNITREE=ON` | Passed | Passed | 17/17 passed |
| Debug, `ENABLE_UNITREE=OFF` | Passed | Passed | 15/15 passed |
| Release, `ENABLE_UNITREE=OFF` | Passed | Passed | 15/15 passed |

## Mission Examples

| Mission | Lifecycle | Debug ON | Release ON | Debug OFF | Release OFF |
| --- | --- | --- | --- | --- | --- |
| `simple.yaml` | Execute | Passed | Passed | Passed | Passed |
| `demo.yaml` | Pause and resume | Passed | Passed | Passed | Passed |
| `flag_ceremony.yaml` | Cancel | Passed | Passed | Passed | Passed |

The examples use a process-local adapter and do not communicate with physical
hardware.

## Install and Package Validation

| Configuration | Install | Package discovery |
| --- | --- | --- |
| Release, `ENABLE_UNITREE=ON` | Passed | Passed |
| Release, `ENABLE_UNITREE=OFF` | Passed | Passed |

Package discovery validates the installed CMake config and exported targets.
The installed example executable also loaded and completed the installed
`simple.yaml` mission in both package variants.

## Documentation

Release documentation includes:

- `README.md`
- `CHANGELOG.md`
- `docs/api/mission_model.md`
- `docs/architecture/README.md`
- `docs/dependency_graph.md`
- `docs/thread_safety.md`
- `docs/versioning.md`
- `docs/Milestone_6_Report.md`

`scripts/run_markdownlint.sh` was executed. It reported that `markdownlint` is
not installed in the local environment and skipped lint execution.

## Known Limitations

- Pause, cancel, and stop apply between steps or retries. A command already
  executing in an adapter cannot be interrupted by the current adapter API.
- The built-in parser intentionally supports the documented mission schema and
  deterministic YAML subset, not the complete YAML language.
- Mission persistence, distributed orchestration, planning, navigation,
  behavior trees, and AI are outside Milestone 6.
- Hardware execution depends on the selected adapter and is not required for
  the hardware-free mission framework validation.

## Release Result

Milestone 6 is complete. The repository is ready for the `v0.6.0-alpha`
release tag.
