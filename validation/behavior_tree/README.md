# Behavior Tree Validation

Behavior tree validation verifies runtime execution of tree semantics using
existing command and mission services.

## Required Nodes

- Sequence.
- Selector.
- Parallel.
- Decorator.
- Retry.
- Timeout.
- Mission node.
- Command node.
- Condition node.

## Procedure

1. Load the approved tree file.
2. Validate unknown and missing nodes are rejected.
3. Execute under runtime supervision.
4. Record each tick sequence, terminal status, command result, mission result,
   and cancellation behavior.
5. Mark `FAIL` if a tree bypasses command safety or produces undocumented
   lifecycle transitions.
