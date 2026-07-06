# Security Guide

Milestone 12 introduces optional security primitives for cloud deployments.

## Supported Concepts

- API keys.
- OAuth2 and JWT capability metadata.
- Role Based Access Control.
- Audit logging.
- Session-management extension points.

The repository does not include authentication SDKs or token-verification
libraries. Deployment applications must use vetted production libraries and
translate authorization decisions into `humanoid::cloud::auth::AuthManager` or
equivalent injected services.

## Robot Safety

Cloud control surfaces must preserve humanoid-core safety boundaries:

- Do not bypass `SafetyValidator`.
- Do not call concrete robot adapters directly.
- Do not expose vendor SDK types through API responses.
- Treat emergency-stop, robot-fault, authentication-failure, and authorization
  failure paths as terminal request rejection.
- Record remote control attempts in an audit log.
