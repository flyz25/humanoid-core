# Hardware Validation Program

The `validation/` tree contains the Hardware Validation Program (HVP) for
humanoid-core. It provides procedures, checklists, templates, and a lightweight
result-recording harness for human operators validating the framework with real
hardware.

The HVP does not claim hardware tests have passed. Every run starts with all
test cases marked `NOT EXECUTED` until a human operator records evidence.

## Directory Layout

- `connection/`, `telemetry/`, `motion/`, `hands/`, `audio/`: robot-facing
  validation procedures.
- `mission/`, `runtime/`, `behavior_tree/`, `planner/`: execution-layer
  validation procedures.
- `perception/`, `ros2/`, `cloud/`: optional subsystem validation procedures.
- `stress/`: long-running runtime validation procedures.
- `fault_injection/`: controlled fault procedures.
- `reports/`: report templates and generated run output.
- `scripts/`: validation automation utilities.
- `Test_Case_Catalog.json`: machine-readable operator test catalog.

## Status Values

- `PASS`
- `FAIL`
- `SKIPPED`
- `NOT EXECUTED`
- `UNKNOWN`

## Create a Run

```bash
python3 validation/scripts/hvp.py new-run \
  --operator "operator-name" \
  --robot-id "lab-robot-01" \
  --vendor "Unitree" \
  --model "G1"
```

The command prints the generated run directory. All cases are initialized as
`NOT EXECUTED`.

## Record a Result

```bash
python3 validation/scripts/hvp.py record \
  --run-dir validation/reports/runs/<run-id> \
  --case-id HVP-CONN-001 \
  --status PASS \
  --operator "operator-name" \
  --notes "Executed by human operator; see attached logs." \
  --evidence "logs/connection.log"
```

Use `FAIL` for any unexpected behavior, unsafe behavior, uncaught exception,
data corruption, or missing evidence.
