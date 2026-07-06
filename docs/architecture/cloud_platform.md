# Cloud Platform Architecture

The cloud platform is an optional outer layer for enterprise deployments. It
does not alter the core robotics architecture.

```text
Deployment adapter or cloud application
  -> Cloud Platform
    -> ROS2 Bridge (optional)
      -> humanoid-core
        -> Plugin Architecture
          -> SDK Wrapper
            -> Vendor SDK
```

## Responsibilities

- Describe versioned REST, gRPC, and WebSocket contracts.
- Maintain fleet registry, heartbeat state, groups, and mission distribution
  records.
- Provide optional authentication, RBAC, sessions, and audit log primitives.
- Plan OTA package, plugin, mission, and configuration updates.
- Record metrics, traces, health snapshots, and operational statistics.
- Provide deployment metadata for Docker, Compose, Kubernetes, Helm, systemd,
  and environment configuration.

## Non-Responsibilities

- Starting HTTP, gRPC, WebSocket, or database servers.
- Owning robot control business logic.
- Bypassing command, mission, behavior tree, runtime, planner, perception, ROS2,
  plugin, or SDK abstraction boundaries.
- Adding cloud SDK, authentication SDK, database SDK, HTTP library, or gRPC
  runtime dependencies to humanoid-core.

## Threading

Cloud managers use internal mutexes or shared mutexes for concurrent access.
Long-running I/O is intentionally absent from the module. Production adapters
that add I/O must perform network work outside these locks and translate
failures into framework-owned result types.
