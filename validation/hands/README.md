# Hand Validation

Hand validation confirms the adapter capability contract and command result
mapping for hand-related operations.

## Required Commands

- Open.
- Close.
- Grip.
- Release.
- Gesture.
- Repeated execution.

## Procedure

1. Confirm the robot advertises hand capability.
2. Keep operators clear of pinch points.
3. Use only approved soft test objects for grip and release cases.
4. Execute each command individually and record command result, observed
   behavior, and video reference.
5. For unsupported commands, verify the framework returns an explicit rejected
   or unsupported result instead of faking success.
