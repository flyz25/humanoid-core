# Source Root

The source root contains integration layers that sit outside the SDK-free core
modules:

- `adapters/`: application-facing communication adapter interfaces and concrete
  adapter implementations.
- `factory/`: robot factory registry used to look up adapter factories by vendor
  and model.
- `sdk/`: vendor SDK wrappers that hide external SDK headers from application
  code.
- `services/`: vendor-independent runtime services that depend on framework
  core interfaces and data models.

Core module implementation files continue to live inside each module's own
`src/` directory.
