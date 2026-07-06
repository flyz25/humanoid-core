# Deployment Guide

Milestone 12 adds deployment assets for optional cloud deployments:

- Docker: `cloud/deployment/docker/Dockerfile`
- Docker Compose: `cloud/deployment/docker/docker-compose.yml`
- Kubernetes: `cloud/deployment/kubernetes/`
- Helm: `cloud/deployment/helm/humanoid-core/`
- systemd: `cloud/deployment/systemd/humanoid-core-cloud.service`
- Environment template: `cloud/deployment/env/humanoid-core-cloud.env.example`

The Dockerfile is intentionally dependency-free and packages repository
metadata. Production applications may replace it with an application image that
links `humanoid::cloud_platform` and concrete transport adapters.

## Docker Build

```bash
docker build -f cloud/deployment/docker/Dockerfile -t humanoid-core:1.0.0 .
```

## CMake Package

The cloud target is exported through the existing package export when
`ENABLE_CLOUD=ON`.
