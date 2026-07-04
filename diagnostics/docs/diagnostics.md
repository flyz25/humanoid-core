# Diagnostics Module

The diagnostics module defines structured diagnostic records, diagnostic status,
and the diagnostic controller boundary. `DiagnosticManager` delegates collection
and self-test requests to an injected controller interface.

Robot-specific health checks, telemetry streams, and vendor diagnostics belong
behind `DiagnosticController` implementations.
