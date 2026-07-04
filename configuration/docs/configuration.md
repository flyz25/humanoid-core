# Configuration Module

The configuration module exposes read-only configuration interfaces and a
manager that owns the active provider interface. It does not parse YAML, JSON,
TOML, environment variables, command-line arguments, or robot-specific files.

Future configuration loaders must implement `Configuration` outside this module
and inject the provider through `ConfigManager`.
