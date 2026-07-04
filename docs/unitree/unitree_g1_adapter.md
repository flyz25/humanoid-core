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
            -> Unitree SDK2
```

Applications depend on `IRobotFactory` and `IRobotAdapter`, not Unitree SDK2.
`LocoClientWrapper` preserves the Milestone 2 adapter-facing API. The only
component that includes Unitree SDK2 headers is
`plugins/unitree/sdk/SdkWrapper.cpp`.

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
