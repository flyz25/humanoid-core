# ROS2 Validation

ROS2 validation applies only when a ROS2 environment is installed. The core
framework must continue to build and operate without ROS2.

## Required Areas

- Topics.
- Services.
- Actions.
- RViz.
- Launch files.
- Restart.
- Bridge recovery.

## Procedure

1. Source the ROS2 environment.
2. Launch the documented humanoid-core ROS2 bridge.
3. Verify topic, service, and action catalogs.
4. Open RViz and verify visualization metadata.
5. Restart the bridge process and record recovery.
6. Do not execute motion actions without safety approval.
