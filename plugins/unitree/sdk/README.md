# Unitree SDK Abstraction

This directory is the only production boundary that may include Unitree SDK2
headers. The abstraction converts Unitree SDK calls, return codes, exceptions,
and normalized robot state into humanoid-core owned types.

## Dependency Flow

```text
UnitreeG1Adapter
  -> LocoClientWrapper
    -> SdkWrapper
      -> Unitree SDK2
```

`LocoClientWrapper` exists as a compatibility facade for the Milestone 2
adapter-facing API. It does not include Unitree SDK2 headers.

## Files

- `SdkWrapper.h` / `SdkWrapper.cpp`: RAII SDK boundary for initialization,
  shutdown, discovery, connection, disconnection, read-only heartbeat
  monitoring, state synchronization, and existing locomotion command methods.
- `SdkTypes.h`: normalized SDK abstraction result, state, descriptor, and enum
  types, including communication timing and worker status snapshots.
- `SdkConverter.h` / `SdkConverter.cpp`: conversions from abstraction types to
  framework `Result`, adapter connection state, and `humanoid::core::RobotState`.

## Read-Only Communication

Milestone 4.6 adds a read-only communication worker:

- `StartCommunication()` starts a background heartbeat loop.
- `SynchronizeState()` performs one read-only SDK heartbeat using the
  locomotion service FSM query.
- `StopCommunication()` stops the worker and joins the thread.
- `CommunicationStatus()` reports worker state, heartbeat count, reconnect
  attempts, and last heartbeat timestamps.
- `SetStateUpdateCallback()` publishes converted `humanoid::core::RobotState`
  snapshots without exposing SDK types.

The heartbeat and reconnect path does not call movement, posture, hand, audio,
or actuator commands. `Disconnect()` and `Shutdown()` stop monitoring and update
local state without issuing motion commands.

On Linux, `Initialize()` validates that the configured network interface exists
and that the process can open a route netlink socket before constructing the
Unitree SDK client. This keeps unsupported WSL2, container, or sandbox
environments on the framework `Result` error path instead of entering undefined
vendor SDK behavior.

## Build Note

The public framework remains C++20. `SdkWrapper.cpp` is compiled with the
vendor-compatible dialect required by the official Unitree SDK2 headers. This is
contained inside the SDK boundary and does not affect public framework targets.
