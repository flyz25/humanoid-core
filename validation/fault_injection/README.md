# Fault Injection Validation

Fault injection validates recovery behavior under controlled failures. Inject
only one fault at a time and return the system to baseline before continuing.

## Faults

- Disconnect Ethernet.
- Disable Wi-Fi.
- Restart SDK process.
- Restart robot.
- Low battery.
- Network latency.
- Packet loss.
- Kill process.
- Kill runtime.
- Disk full.
- Configuration corruption.

## Procedure

1. Confirm robot is stationary or in a site-approved safe state.
2. Record baseline connection and telemetry state.
3. Inject one approved fault.
4. Observe framework state, command rejection, logging, and recovery behavior.
5. Restore baseline and verify state freshness.
6. Record failures for crashes, stale connected state, unsafe command execution,
   data corruption, or unrecoverable deadlock.
