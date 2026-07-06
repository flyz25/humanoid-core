# Planner Validation

Planner validation verifies the approved operator-reviewed flow:

```text
Goal -> Mission -> Behavior Tree -> Execution
```

## Procedure

1. Submit an approved goal.
2. Save planner diagnostics and generated artifacts.
3. Human operator reviews mission and behavior tree output before execution.
4. Execute only after approval.
5. Record planning result, fallback behavior, diagnostics, and execution result.

No AI or LLM provider output may be executed on hardware without human review.
