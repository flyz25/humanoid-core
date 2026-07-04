# Unitree SDK Abstraction

This directory is the only production boundary that may include Unitree SDK2
headers. The abstraction converts Unitree SDK calls, return codes, exceptions,
and normalized robot state into humanoid-core owned types.

## Dependency Flow

```text
UnitreeG1Adapter
  -> LocoClientWrapper
    -> SdkWrapper
      -> LocoAdapter / HandAdapter / AudioAdapter
      -> Unitree SDK2
```

`LocoClientWrapper` exists as a compatibility facade for the Milestone 2
adapter-facing API. It does not include Unitree SDK2 headers.

## Files

- `SdkWrapper.h` / `SdkWrapper.cpp`: RAII SDK boundary for initialization,
  shutdown, discovery, connection, disconnection, read-only heartbeat
  monitoring, state synchronization, and command delegation.
- `LocoAdapter.h` / `LocoAdapter.cpp`: locomotion command adapter for stand,
  sit, walk, stop, velocity, and emergency stop.
- `HandAdapter.h` / `HandAdapter.cpp`: hand and upper-body action adapter. It
  maps supported gestures to the Unitree arm action service and rejects
  unsupported finger-level commands explicitly.
- `AudioAdapter.h` / `AudioAdapter.cpp`: audio stream, stop, volume, and mute
  command adapter.
- `SdkTypes.h`: normalized SDK abstraction result, state, descriptor, and enum
  types, including communication timing, worker status snapshots, velocity,
  gesture, and audio payloads.
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
and that the process can open a route netlink socket before constructing
Unitree SDK clients. This keeps unsupported WSL2, container, or sandbox
environments on the framework `Result` error path instead of entering undefined
vendor SDK behavior.

## Command Adapters

Milestone 4.7 adds pure SDK command adapters:

- `LocoAdapter` owns `unitree::robot::g1::LocoClient` and converts normalized
  velocity and posture commands into SDK2 locomotion calls.
- `HandAdapter` owns `unitree::robot::g1::G1ArmActionClient` and converts
  supported generic gestures to SDK2 arm action ids.
- `AudioAdapter` owns `unitree::robot::g1::AudioClient` and converts playback,
  stop, volume, and mute requests to SDK2 audio calls.

These adapters contain no mission logic, behavior sequencing, AI, planning, or
application workflow decisions. They serialize SDK access with internal mutexes
and return `SdkResult` for all failures.

## Build Note

The public framework remains C++20. The Unitree SDK abstraction target is
compiled with the vendor-compatible dialect required by the official Unitree
SDK2 headers. This is contained inside the SDK boundary and does not affect
public framework targets.
