# Dependency Rules

The dependency graph must remain acyclic and point inward toward stable
interfaces.

Allowed dependency direction:

```text
applications -> managers, interfaces, factory registry
plugin hosts -> plugin infrastructure
plugin infrastructure -> common
factory registry -> robot factory interfaces
robot factories -> robot adapter implementations
robot adapters -> SDK wrapper facades
SDK wrapper facades -> vendor SDK abstraction targets
vendor SDK abstraction targets -> vendor SDKs
core -> logging, configuration, common
managers -> module interfaces, common
utilities -> common
common -> standard library only
```

Forbidden dependencies:

- Applications directly including vendor SDK headers.
- Applications directly constructing concrete robot adapters.
- Managers depending on concrete adapters or communication backends.
- SDK wrapper facades directly including vendor SDK headers.
- Core modules depending on concrete plugins or plugin implementations.
- `humanoid::humanoid_core` linking concrete plugins.
- Core modules depending on ROS2, OpenCV, AI runtimes, GUI frameworks, or Unitree
  SDKs.
- Common depending on any other humanoid-core module.
- Interfaces owning global state or singleton access.

All dependencies must be injected through constructors, setters, or explicit
composition roots owned by applications.
