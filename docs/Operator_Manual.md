# Hardware Validation Operator Manual

## Role

The operator executes physical tests, records evidence, and decides the status
for each validation case. The automation prepares runs and records results but
does not certify hardware behavior.

## Basic Workflow

1. Read `docs/Hardware_Validation_Specification.md`.
2. Prepare the environment using `docs/Validation_Environment.md`.
3. Complete `docs/Operator_Checklist.md`.
4. Create a run with `validation/scripts/hvp.py new-run`.
5. Execute procedures under `validation/`.
6. Record results with `validation/scripts/hvp.py record`.
7. File issues using `validation/reports/Issue_Report_Template.md`.
8. Complete acceptance or regression reports.

## Result Guidance

- Use `PASS` only with evidence.
- Use `FAIL` for unexpected behavior, unsafe behavior, missing required
  evidence, crash, deadlock, stale state, or command safety bypass.
- Use `SKIPPED` only with reason and capability evidence.
- Use `UNKNOWN` for ambiguous outcomes requiring follow-up.
- Leave `NOT EXECUTED` for cases not attempted.

## Evidence Guidance

Store large logs and videos outside the repository when required by site
policy. Record references in the HVP run directory.
