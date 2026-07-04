# ADR-0005: SDK Wrapper Boundary

## Status

Accepted

## Context

Vendor SDK APIs are not stable framework contracts. Unitree SDK2 includes
headers, transport initialization, and return codes that should not leak into
the humanoid-core public API.

## Decision

Each vendor SDK is hidden behind a narrow SDK abstraction boundary. For Unitree
G1, the implementation files under `plugins/unitree/sdk/` own SDK clients
internally and expose only framework-owned SDK abstraction types.
`LocoClientWrapper` preserves the legacy adapter-facing API and delegates to
that boundary.

## Consequences

- SDK exceptions and return codes are translated to normalized SDK abstraction
  results and then to framework `Result` values.
- SDK headers are included only in SDK abstraction implementation files.
- Adapter implementations translate framework commands and delegate to SDK
  boundary classes.
- Public API stability is decoupled from vendor SDK API changes.
