# Common Module

The common module owns dependency-free primitives shared across the framework.
It contains lifecycle states, status codes, status values, and semantic version
metadata. These types are safe for every layer to include because they do not
depend on robot adapters, middleware, logging backends, or configuration
parsers.

Dependency rule: `common` must not depend on any other humanoid-core module.
