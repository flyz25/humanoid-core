# ADR-0002: Vendor Isolation

## Status

Accepted

## Context

The framework currently supports Unitree G1 communication, but future robot
vendors include Unitree H1, Unitree H2, simulators, mock robots, and custom
robots. Vendor SDKs often bring transport, threading, and dependency constraints
that should not leak into application code.

## Decision

Vendor SDK headers and types must not appear in application-facing interfaces,
manager modules, or umbrella headers. Vendor dependencies are isolated behind
adapter implementations and SDK wrappers.

## Consequences

- Applications never include Unitree SDK2 headers.
- Unitree SDK2 appears only in `src/sdk/LocoClientWrapper.cpp`.
- New vendors can be added by implementing existing interfaces.
- SDK replacement does not require application-layer changes.
