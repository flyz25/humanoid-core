# Dependency Rules

The dependency graph must remain acyclic and point inward toward stable
interfaces.

Allowed dependency direction:

```text
core -> logging, configuration, common
managers -> module interfaces, common
module interfaces -> common when status or lifecycle types are required
utilities -> common
common -> standard library only
```

Forbidden dependencies:

- Applications directly including vendor SDK headers.
- Managers depending on concrete adapters or communication backends.
- Core modules depending on ROS2, OpenCV, AI runtimes, GUI frameworks, or Unitree
  SDKs.
- Common depending on any other humanoid-core module.
- Interfaces owning global state or singleton access.

All dependencies must be injected through constructors, setters, or explicit
composition roots owned by applications.
