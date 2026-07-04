# Architecture

humanoid-core follows Clean Architecture with strict dependency direction:

```text
Applications
  -> Core Interfaces
    -> Managers
      -> Robot Adapters
        -> Vendor SDKs
```

Applications depend on interfaces and managers. Managers depend only on abstract
interfaces. Robot adapters implement framework interfaces and isolate vendor
SDKs, simulators, middleware, or embedded transport details.

The foundation layer does not implement Unitree SDK integration, ROS2, DDS
participants, robot communication, AI, OpenCV, GUI workflows, mission engines,
or event controllers.

## Module Ownership

- `common`: dependency-free lifecycle, status, and version primitives.
- `utilities`: small implementation-agnostic helpers.
- `logging`: logger and sink interfaces plus sink routing infrastructure.
- `configuration`: read-only configuration interfaces and provider ownership.
- `robot`: robot and robot adapter abstractions.
- `motion`: motion controller abstractions.
- `gesture`: named gesture controller abstractions.
- `safety`: safety state and safety controller abstractions.
- `diagnostics`: diagnostic records and diagnostic controller abstractions.
- `network`: transport metadata and network manager abstraction.
- `core`: package metadata and application-facing interface context.

## Adapter Rule

Vendor SDK headers must never be included by application code, manager headers,
or manager source files. Vendor SDK dependencies belong in adapter packages that
implement `IRobotAdapter` or other module interfaces.
