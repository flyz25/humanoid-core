# Mission Validation

Mission validation verifies that mission loading and execution use the existing
command framework and safety validation.

## Required Cases

- Mission loading.
- Mission execution.
- Pause.
- Resume.
- Cancel.
- Retry.
- Loop.
- Timeout.
- Nested mission.

## Procedure

1. Review mission content before execution.
2. Validate mission schema and command list.
3. Execute only hardware-safe missions approved by the safety lead.
4. Record mission status, current step, command results, retry counts, timeout
   decisions, and cancellation result.
5. Attach mission file hash and execution log.
