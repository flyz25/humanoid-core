# Logging Module

The logging module defines logging interfaces and routing infrastructure only.
It provides `ILogger`, `Logger`, `LogSink`, `LogLevel`, `LogMessage`, and
`LoggerManager`. The manager owns a collection of sink interfaces and forwards
messages to sinks that accept the message severity.

Backends such as console logging, file logging, network logging, or spdlog
adapters must live outside this module behind `LogSink`.
