# Cloud Platform API

Milestone 12 introduces `humanoid::cloud_platform`, an optional C++ target that
contains enterprise platform contracts without external transport dependencies.

## Build Option

```bash
cmake -S . -B build -DENABLE_CLOUD=ON
cmake --build build --target humanoid_core_cloud_platform
```

Set `-DENABLE_CLOUD=OFF` to remove cloud targets from the build.

## C++ Interfaces

- `humanoid::cloud::CloudPlatform`: lifecycle facade that aggregates injected
  cloud services.
- `humanoid::cloud::api::RestApiCatalog`: versioned REST endpoint metadata.
- `humanoid::cloud::grpc::DefaultGrpcCatalog`: optional gRPC service metadata.
- `humanoid::cloud::websocket::DefaultWebSocketStreams`: real-time stream
  metadata.
- `humanoid::cloud::fleet::FleetManager`: thread-safe robot registration,
  groups, heartbeat state, status, and mission distribution records.
- `humanoid::cloud::auth::AuthManager`: API key principal registration, RBAC
  permission checks, and audit records.
- `humanoid::cloud::ota::OtaManager`: package registration, update planning,
  version verification, and rollback records.
- `humanoid::cloud::monitoring::ObservabilityRegistry`: metrics, trace spans,
  and health snapshots.

## External Contracts

- REST: `cloud/api/openapi.yaml`
- gRPC: `cloud/grpc/humanoid_cloud.proto`
- WebSocket: `cloud/websocket/streams.yaml`
- Dashboard backend: `cloud/dashboard/dashboard_api.yaml`

These files are contracts only. Concrete network servers belong in deployment
applications that depend on the optional cloud module.
