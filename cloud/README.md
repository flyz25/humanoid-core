# Humanoid Core Cloud Platform

The `cloud/` module contains optional enterprise platform contracts for
humanoid-core. It provides cloud-facing catalogs, fleet management primitives,
authentication and audit records, OTA planning, observability records, deployment
assets, and examples without adding HTTP, gRPC, database, or cloud SDK
dependencies to the core framework.

Build with cloud targets:

```bash
cmake -S . -B build -DENABLE_CLOUD=ON
cmake --build build
```

Build without cloud targets:

```bash
cmake -S . -B build -DENABLE_CLOUD=OFF
cmake --build build
```

## Dependency Boundary

Allowed direction:

```text
Cloud Platform (optional)
  -> humanoid-core
  -> Plugin Architecture
  -> SDK wrappers
```

Forbidden dependencies:

- Core framework headers including HTTP, gRPC, authentication, database, or cloud
  SDK headers.
- Cloud transport adapters bypassing command, mission, runtime, perception, or
  plugin interfaces.
- Global cloud platform instances.

## Contents

- `api/openapi.yaml`: versioned REST API contract.
- `grpc/humanoid_cloud.proto`: optional gRPC service contract.
- `websocket/streams.yaml`: real-time stream catalog.
- `dashboard/dashboard_api.yaml`: backend dashboard API catalog.
- `deployment/`: Docker, Compose, Kubernetes, Helm, systemd, and environment
  configuration assets.
- `examples/`: hardware-free examples for fleet, REST, WebSocket, telemetry,
  registration, mission upload, remote control catalog lookup, and OTA update.
