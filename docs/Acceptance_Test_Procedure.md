# Acceptance Test Procedure

## Objective

The acceptance test determines whether a humanoid-core release is suitable for
deployment on a specific robot profile and site.

## Minimum Acceptance Set

- Connection validation.
- Reconnect and heartbeat validation.
- Robot state and telemetry validation.
- Emergency-stop validation.
- Required motion profile validation.
- Mission and command lifecycle validation.
- Stress duration defined by the product profile.
- Regression cases for changed areas.

## Decision States

- Accepted.
- Accepted with limitations.
- Rejected.

## Procedure

1. Confirm all required test cases are `PASS` or explicitly `SKIPPED`.
2. Review all `FAIL` and `UNKNOWN` records.
3. Confirm critical and high severity issues are closed or accepted by risk
   owner.
4. Review evidence package completeness.
5. Complete `docs/Acceptance_Test_Report.md`.
6. Obtain operator, safety lead, and release owner sign-off.
