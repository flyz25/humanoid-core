# ADR-0004: Factory Registry

## Status

Accepted

## Context

Applications need to request robot adapters by vendor and model without knowing
which concrete adapter class implements the robot. Future vendors must not
require application source changes.

## Decision

The framework uses `IRobotFactory` and `RobotFactoryRegistry`. Composition roots
register factories, and applications request adapters through the registry using
`RobotConfig`.

## Consequences

- Applications receive `std::unique_ptr<IRobotAdapter>`.
- Concrete adapter construction remains outside application logic.
- Factories are replaceable in tests.
- The registry is synchronized for factory registration and lookup.
