# Motion Validation

Motion validation is safety-critical. A human operator and safety lead must
approve every procedure before execution.

## Required Commands

- Stand.
- Sit.
- Walk.
- Stop.
- Forward.
- Backward.
- Left.
- Right.
- Rotate.
- Velocity and acceleration limits.
- Emergency stop and recovery.
- Repeated execution for 100 cycles and 1000 cycles when approved.

## Procedure

1. Use the approved test surface and marked safety area.
2. Verify emergency stop and spotter assignment.
3. Start with the lowest safe velocity.
4. Execute one command category at a time.
5. Stop and reset the robot between directional commands.
6. Record command `Result`, `RobotState`, video reference, operator notes, and
   any drift or instability.
7. Abort immediately on unexpected acceleration, loss of balance, fault,
   thermal limit, low battery, or operator concern.

The framework provides scripts and templates only. It does not certify motion
without human-executed hardware evidence.
