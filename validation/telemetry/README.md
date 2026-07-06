# Robot State and Telemetry Validation

Telemetry validation verifies that physical robot state is mapped into
vendor-independent framework state. The operator must compare telemetry against
approved robot-side references when available.

## Required Observations

- Battery level and charging state.
- IMU orientation.
- Joint and motor state where supported by the adapter.
- Motor temperature where supported by the adapter.
- Fault and emergency-stop state.
- Telemetry latency and packet loss.

## Procedure

1. Start with the robot stationary and connected.
2. Enable telemetry logging in the selected application or bridge.
3. Capture at least 10 minutes of baseline telemetry.
4. Record min, max, mean, and p95 latency when timestamps are available.
5. Record any packet loss, stale timestamps, state regressions, or missing
   fields as issues.

Telemetry fields not supported by a robot model must be recorded as `SKIPPED`
with the capability evidence attached.
