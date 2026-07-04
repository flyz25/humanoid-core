# Utilities Module

The utilities module contains small, implementation-agnostic helpers used by the
framework: filesystem validation, monotonic time helpers, and RAII cleanup
support. It may depend on `common`, but it must not depend on robotics domain
modules, middleware, or adapters.
