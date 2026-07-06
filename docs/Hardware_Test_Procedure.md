# Hardware Test Procedure

## Preparation

1. Review the robot safety procedure and site risk assessment.
2. Confirm robot hardware, battery, network, firmware, and SDK version.
3. Confirm emergency stop and operator roles.
4. Build or install the release candidate.
5. Create a validation run:

```bash
python3 validation/scripts/hvp.py new-run \
  --operator "<name>" \
  --robot-id "<robot-id>" \
  --vendor "<vendor>" \
  --model "<model>" \
  --serial-number "<serial>" \
  --firmware "<firmware>"
```

## Execution

1. Execute one validation case at a time.
2. Collect logs and video references before marking a result.
3. Record the result:

```bash
python3 validation/scripts/hvp.py record \
  --run-dir validation/reports/runs/<run-id> \
  --case-id HVP-CONN-001 \
  --status PASS \
  --operator "<name>" \
  --notes "<operator evidence summary>"
```

1. Summarize the run:

```bash
python3 validation/scripts/hvp.py summarize \
  --run-dir validation/reports/runs/<run-id>
```

## Stop Conditions

Stop hardware validation immediately if:

- robot loses balance,
- unexpected motion occurs,
- emergency stop is activated,
- battery, thermal, or motor fault is reported,
- network behavior prevents safe command supervision,
- operator or safety lead requests stop.

## Completion

Complete the Hardware Validation Report and attach evidence. Do not mark a
release as hardware-accepted without human sign-off.
