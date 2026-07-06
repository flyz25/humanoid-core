# Hardware Validation Specification

## Purpose

The Hardware Validation Program (HVP) defines repeatable procedures for
validating humanoid-core on physical robot hardware. It covers connection,
telemetry, motion, hands, audio, mission execution, runtime, behavior trees,
planner flow, perception, ROS2, cloud, stress, and fault injection.

The program provides tooling and documentation only. Physical hardware tests
must be executed and signed off by a human operator. This repository does not
claim that hardware validation has passed until completed reports are recorded.

## Scope

The HVP validates the completed v1 framework without changing architecture,
public APIs, or framework runtime behavior.

In scope:

- Operator-executed hardware procedures.
- Evidence capture and result recording.
- Repeatable validation run directories.
- Validation checklists and reports.
- Regression and acceptance test templates.

Out of scope:

- New framework features.
- New robot control APIs.
- SDK redesign.
- Automated certification of physical robot safety.

## Status Model

Every validation case shall be recorded with one of these statuses:

- `PASS`: expected result observed with sufficient evidence.
- `FAIL`: unexpected behavior, unsafe behavior, missing required evidence, or
  acceptance criteria not met.
- `SKIPPED`: intentionally not executed with documented reason.
- `NOT EXECUTED`: not yet attempted.
- `UNKNOWN`: execution attempted but result cannot be determined.

## Evidence Requirements

Each executed hardware case should include:

- validation run ID,
- operator name,
- robot model and serial number,
- software version and Git commit,
- SDK version when applicable,
- configuration snapshot,
- relevant logs,
- RobotState or command result snapshot,
- video reference for motion or physical actuation,
- issue references for failures.

## Safety Requirements

- Human operator supervision is mandatory for physical robot tests.
- Emergency stop must be reachable before connection or motion tests.
- Motion tests require a clear test area, approved surface, and spotters.
- Planner and behavior-tree output must be reviewed before hardware execution.
- Fault injection must not be combined with high-risk motion.
- Remote cloud control paths must remain disabled unless explicitly approved.

## Validation Layers

```text
Hardware
  -> SDK Wrapper / Adapter
    -> Plugin Registry
      -> Robot State / Telemetry
        -> Command / Safety
          -> Mission / Runtime / Behavior Tree / Planner
            -> Optional ROS2 / Cloud / Perception integrations
```

Validation follows the existing architecture and does not add a framework layer.

## Acceptance Criteria

A release candidate may be accepted for hardware deployment only when:

- all required connection cases are `PASS`,
- required telemetry cases are `PASS` or documented `SKIPPED`,
- motion cases required by the product profile are `PASS`,
- emergency-stop validation is `PASS`,
- no critical or high unresolved safety issue remains,
- stress duration required by the product profile is completed,
- regression suite has no unexplained regressions,
- reports are signed by operator, safety lead, and release owner.
