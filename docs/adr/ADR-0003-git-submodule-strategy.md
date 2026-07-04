# ADR-0003: Git Submodule Strategy

## Status

Accepted

## Context

Unitree SDK2 is required for the optional Unitree adapter, but the project must
not require system-wide installation into `/usr/local`. Builds must be
reproducible and pinned for production.

## Decision

Unitree SDK2 is consumed as a Git submodule at `third_party/unitree_sdk2` and
pinned to a stable tag or exact commit. The current pin is documented in
`third_party/unitree_sdk2.version`.

## Consequences

- Developers initialize dependencies with `git submodule update --init --recursive`.
- CMake can discover the SDK from the submodule or `UNITREE_SDK2_ROOT`.
- The framework still builds when the SDK is unavailable by disabling only
  Unitree adapter targets.
- Floating SDK branch heads are not used for production builds.
