# Test Case Catalog

The authoritative machine-readable catalog is
`validation/Test_Case_Catalog.json`. It is consumed by
`validation/scripts/hvp.py` and initializes every validation run with all cases
marked `NOT EXECUTED`.

## Catalog Areas

| Area | Coverage |
| --- | --- |
| Connection | connection, reconnect, heartbeat, timeout, power cycle, repeated reconnect |
| Telemetry | battery, charging, IMU, fault state, emergency stop, latency, packet loss |
| Motion | stand, sit, walk, stop, directions, rotation, emergency stop, repetition |
| Hands | open, close, grip, release, gesture, repetition |
| Audio | playback, stop, volume, mute, invalid input |
| Mission | load, execute, pause, resume, cancel, retry, loop, timeout, nesting |
| Runtime | context, blackboard, cancellation, resource manager, scheduler |
| Behavior Tree | sequence, selector, parallel, decorators, retry, timeout, leaf nodes |
| Planner | goal to mission to behavior tree to execution |
| Perception | camera, YOLO, face, pose, QR, AprilTag, fusion, lighting |
| ROS2 | topics, services, actions, RViz, launch, restart, bridge recovery |
| Cloud | fleet, REST, gRPC, WebSocket, OTA, dashboard, auth, RBAC |
| Stress | 4h, 8h, 12h, 24h, resource and thermal observation |
| Fault Injection | network, SDK, robot, process, disk, configuration, battery faults |

## Listing Cases

```bash
python3 validation/scripts/hvp.py list-cases
```

## Adding Cases

New cases must:

- use a stable `HVP-AREA-NNN` identifier,
- define objective, procedure, expected result, safety notes, and evidence,
- start as `NOT EXECUTED` in every new run,
- avoid claiming hardware success without operator evidence.
