# Unitree G1 Plugin Skeleton

The Unitree G1 plugin skeleton is the first concrete plugin package in
humanoid-core. It validates the plugin packaging, lifecycle, metadata, factory
registration, and adapter boundary without communicating with Unitree SDK2.

## Scope

- Provides `humanoid::plugins::unitree::g1::UnitreeG1Plugin`.
- Provides `humanoid::plugins::unitree::g1::UnitreeG1Adapter`.
- Provides `plugin_manifest.json` for future loader integration.
- Returns a conservative mock `humanoid::core::RobotState`.

## Non-Goals

- No SDK communication.
- No physical robot connection.
- No movement commands.
- No behavior, mission, navigation, AI, ROS2, or vision integration.

The skeleton builds whether `ENABLE_UNITREE` is `ON` or `OFF` because it does
not depend on Unitree SDK2.
