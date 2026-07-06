# Cloud Guide

The cloud module provides optional enterprise integration contracts while
keeping humanoid-core usable offline.

## Enable or Disable

```bash
cmake -S . -B build -DENABLE_CLOUD=ON
cmake --build build
```

Disable:

```bash
cmake -S . -B build -DENABLE_CLOUD=OFF
cmake --build build
```

## REST API

The REST contract is stored in `cloud/api/openapi.yaml`. It covers robot,
mission, behavior tree, planner, telemetry, perception, fleet, runtime, health,
diagnostics, and configuration APIs.

## gRPC and WebSocket

The gRPC contract is stored in `cloud/grpc/humanoid_cloud.proto`. WebSocket
stream metadata is stored in `cloud/websocket/streams.yaml`.

Both are optional contracts. Concrete gRPC and WebSocket runtime adapters may be
implemented by deployment applications without changing core framework targets.

## Offline Operation

No cloud service is required for local robot applications. All cloud targets can
be removed from the build with `ENABLE_CLOUD=OFF`.
