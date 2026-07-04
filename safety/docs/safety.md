# Safety Module

The safety module defines safety state, safety control contracts, and a manager
that delegates to an injected `SafetyController`. The manager is deliberately
conservative: without a controller, motion is not reported as allowed.

Hardware safety circuits, certified stops, industrial fieldbus logic, and robot
SDK safety APIs belong behind `SafetyController` implementations.
