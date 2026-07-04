# ADR-0001: Clean Architecture

## Status

Accepted

## Context

humanoid-core is intended to be a long-term foundation for humanoid robot
applications. The framework must support multiple robot vendors, simulators, and
future deployment targets without forcing applications to depend on vendor SDKs
or transport details.

## Decision

The repository uses Clean Architecture:

```text
Applications
  -> Managers
    -> Interfaces
      -> Factories
        -> Adapters
          -> SDK Wrappers
            -> Vendor SDKs
```

Stable interfaces live toward the center. Vendor SDK dependencies live at the
outer edge.

## Consequences

- Applications depend on interfaces and managers.
- Core modules remain vendor independent.
- Robot-specific code is added through adapter packages.
- Dependency direction is explicit and reviewable in CMake target links.
