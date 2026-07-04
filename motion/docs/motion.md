# Motion Module

The motion module defines high-level motion controller contracts and a manager
that delegates to an injected `MotionController`. It does not implement gaits,
whole-body control, inverse kinematics, trajectory generation, low-level command
serialization, or robot communication.

Motion implementations must remain behind the `MotionController` interface.
