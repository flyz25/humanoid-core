# Core Module

The core module exposes package-level runtime metadata and the core application
context. It stores only interface references and does not depend on robot SDKs,
middleware, perception, mission execution, GUI code, or application logic.

Applications should depend on core interfaces and managers. Robot-specific
implementation details must stay behind adapter boundaries.
