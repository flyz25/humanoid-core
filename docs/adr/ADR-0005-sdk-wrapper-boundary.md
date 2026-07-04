# ADR-0005: SDK Wrapper Boundary

## Status

Accepted

## Context

Vendor SDK APIs are not stable framework contracts. Unitree SDK2 includes
headers, transport initialization, and return codes that should not leak into
the humanoid-core public API.

## Decision

Each vendor SDK is hidden behind a narrow SDK wrapper. For Unitree G1,
`LocoClientWrapper` owns the SDK client internally and exposes only framework
types such as `RobotConfig` and `Result`.

## Consequences

- SDK exceptions and return codes are translated to framework `Result` values.
- SDK headers are included only in wrapper implementation files.
- Adapter implementations translate framework commands and delegate to wrappers.
- Public API stability is decoupled from vendor SDK API changes.
