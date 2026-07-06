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
