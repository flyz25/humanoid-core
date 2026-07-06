# Connection Validation

Connection validation is executed by a human operator with the robot in a safe,
stationary state. These procedures verify connection lifecycle behavior without
issuing motion commands.

## Required Cases

- `HVP-CONN-001`: robot connection.
- `HVP-CONN-002`: reconnect after network interruption.
- `HVP-CONN-003`: heartbeat timeout.
- `HVP-CONN-004`: power cycle recovery.
- `HVP-CONN-005`: multiple reconnect cycles.

## Operator Procedure

1. Confirm emergency stop, clear test area, and battery threshold.
2. Record robot model, serial number, firmware, network type, and SDK version.
3. Create an HVP run with `validation/scripts/hvp.py new-run`.
4. Execute one connection case at a time.
5. Record logs, timestamps, packet captures when approved, and operator notes.
6. Mark cases `FAIL` if the framework crashes, blocks indefinitely, reports a
   stale connected state, or requires unplanned manual process cleanup.

## Network Modes

Record whether each case used Ethernet or Wi-Fi. For Wi-Fi, record access
point, channel, signal strength if available, and roaming state.
