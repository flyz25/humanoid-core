# Unitree G1 Adapter

The Unitree G1 adapter supports the Unitree G1 communication layer through
Unitree SDK2. It does not add ROS2, AI, mission execution, vision, mapping,
planning, behavior trees, navigation, event engines, or task scheduling.

## Dependency Flow

```text
Application
  -> RobotFactoryRegistry
    -> UnitreeRobotFactory
      -> UnitreeG1Adapter
        -> LocoClientWrapper
          -> SdkWrapper
            -> LocoAdapter / HandAdapter / AudioAdapter
              -> Unitree SDK2
```

Applications depend on `IRobotFactory` and `humanoid::core::RobotAdapter`, not
Unitree SDK2. `humanoid::adapters::IRobotAdapter` is retained only as a
source-compatible alias to the same public interface. `LocoClientWrapper`
preserves the Milestone 2 adapter-facing API. Only implementation files under
`plugins/unitree/sdk/` include Unitree SDK2 headers.

The Unitree G1 plugin (`plugins/unitree/g1`) uses the same SDK abstraction as
the factory adapter. It owns plugin lifecycle, adapter creation, capability
declaration, state snapshots, command forwarding, and error translation without
exposing SDK headers or vendor types through public plugin headers.

## SDK Source

Unitree SDK2 is pinned as a submodule:

```text
Path: third_party/unitree_sdk2
Repository: https://github.com/unitreerobotics/unitree_sdk2.git
Tag: 2.0.2
Commit: 811bc77
```

Initialize it with:

```bash
git submodule update --init --recursive
```

## Build

`ENABLE_UNITREE` is on by default:

```bash
cmake -S . -B build -DENABLE_UNITREE=ON
cmake --build build --parallel
```

If SDK2 is unavailable, CMake prints an informative status message and disables
only the Unitree targets.

The SDK abstraction target compiles with a C++17 dialect even though the public
framework uses C++20. This is intentional: the pinned Unitree SDK2 release
contains CycloneDDS C++ headers that GCC 11 rejects in C++20 mode. The dialect
boundary is confined to `plugins/unitree/sdk/` and does not change the public
C++20 framework API.

## Configuration

The example uses `config/robot.yaml`:

```yaml
robot:
  vendor: Unitree
  model: G1
  ip: 192.168.123.161
  network_interface: eth0
  timeout_ms: 500
  domain_id: 0
  serial_number:
  firmware:
```

`SdkWrapper` initializes Unitree SDK2 communication through a network interface.
The `ip` value is retained as robot connection metadata and validated by the
adapter.

## Read-Only SDK2 Communication

Milestone 4.6 adds read-only communication monitoring to the SDK abstraction:

- `Connect()` verifies the locomotion service with a read-only FSM query.
- `StartCommunication()` starts a heartbeat worker after `Connect()` is called.
- `SynchronizeState()` refreshes the cached normalized SDK state.
- Automatic reconnect attempts run after heartbeat failures.
- Connection timeout is derived from `RobotConfig::timeout` through the
  adapter-facing `LocoClientWrapper`.
- `Disconnect()` and `Shutdown()` stop the heartbeat worker without issuing
  `StopMove()` or any other motion command.

The communication worker is intentionally read-only. It does not walk, stand,
move hands, play audio, or command actuators. Existing command APIs remain
separate and are not used by the heartbeat/reconnect path.

## Motion, Hand, and Audio Adapters

Milestone 4.7 adds SDK-boundary adapters for command translation:

- `LocoAdapter` maps stand, sit, walk, velocity, stop, and emergency stop
  requests to the Unitree G1 locomotion client.
- `HandAdapter` maps supported hand and upper-body gestures to the Unitree G1
  arm action client. Finger-level open, close, and grip commands are rejected
  explicitly because they are not exposed by the SDK2 arm action client.
- `AudioAdapter` maps PCM playback, stop, volume, and mute requests to the
  Unitree G1 audio client.

These adapters are vendor-specific and contain no mission logic, behavior
sequencing, planning, AI, or application workflow decisions.

`CommandDispatcher` forwards framework `Command` values to
`humanoid::core::RobotAdapter::ExecuteCommand()`. The Unitree G1 adapter routes
supported stand, sit, walk/move/velocity, rotate, stop, emergency stop, gesture,
play-audio, stop-audio, volume, and mute commands through SDK-boundary
adapters. Finger-level open/close and arbitrary custom commands are rejected
explicitly when unsupported by the SDK boundary.

On Linux, SDK initialization performs a preflight check for the configured
network interface and route netlink socket access before constructing the
Unitree SDK client. If the process is running in a restricted WSL2, container,
or sandbox environment, initialization returns a framework `Result` failure
rather than entering the vendor SDK runtime.

## Robot State Synchronization

`UnitreeRobotFactory` may be constructed with a shared
`humanoid::core::RobotStateManager`. When provided, the factory injects the
manager into created `UnitreeG1Adapter` instances. The adapter passes it to
`LocoClientWrapper`, which installs a callback on `SdkWrapper`. Each successful
or failed synchronization publishes a converted, vendor-independent
`humanoid::core::RobotState` snapshot.

State synchronization maps SDK-side connection state, robot mode, motion mode,
battery and charging fields when available, IMU orientation and angular/linear
signals, fixed-size joint state samples, contact samples, fault code,
emergency-stop state, heartbeat counters, reconnect counters, latency, and
diagnostic health data into the generic `RobotState` model.
