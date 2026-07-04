# Future Roadmap

The foundation is intentionally interface-first. Future work can add
implementation packages without changing the core contracts.

Planned extension areas:

- Robot adapters for Unitree G1, Unitree H1, Unitree H2, simulators, and mock
  robots.
- Logging sinks for console, files, spdlog, and network diagnostics.
- Configuration providers for YAML, command-line overrides, and environment
  profiles.
- DDS or other transport implementations behind network and adapter interfaces.
- Motion, gesture, safety, and diagnostics implementations specific to robot
  families.

Any extension must preserve the dependency rule that applications never call
vendor SDKs directly.
