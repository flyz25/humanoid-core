# Robot Module

The robot module defines the clean boundary between framework managers and
robot-specific adapters. `Robot` is the high-level robot abstraction.
`IRobotAdapter` is the adapter boundary for future vendor SDKs, simulators, and
test robots.

No Unitree SDK, communication protocol, DDS participant, simulator API, or robot
driver belongs in this module. Those dependencies must be introduced behind
`IRobotAdapter` in adapter packages.
