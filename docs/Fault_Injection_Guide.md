# Fault Injection Guide

## Purpose

Fault injection validates controlled recovery behavior. Only one fault shall be
introduced at a time.

## Fault Catalog

- Ethernet disconnect.
- Wi-Fi disable.
- SDK restart.
- Robot restart.
- Low battery.
- Network latency.
- Packet loss.
- Application process kill.
- Runtime process kill.
- Disk full.
- Configuration corruption.

## Procedure

1. Start from a stable baseline.
2. Confirm robot is stationary or in an approved safe state.
3. Inject one fault.
4. Observe framework state, logs, command rejection, and recovery.
5. Restore baseline.
6. Record result and evidence before continuing.

## Safety Limits

Do not perform fault injection during unsupported motion, remote operation, or
when emergency-stop access is obstructed.
