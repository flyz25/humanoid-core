# Cloud Validation

Cloud validation applies only when optional cloud platform components are
enabled in a deployment environment. The framework must continue to work fully
offline.

## Required Areas

- Fleet.
- REST.
- gRPC.
- WebSocket.
- OTA.
- Dashboard backend.
- Authentication.
- RBAC.

## Procedure

1. Deploy approved cloud backend or contract harness.
2. Register robot and verify fleet status.
3. Exercise REST, gRPC, and WebSocket contract paths.
4. Validate authentication, authorization, audit records, and RBAC decisions.
5. Validate OTA planning without applying an unapproved update.
6. Record all remote control paths as disabled unless explicitly approved.
