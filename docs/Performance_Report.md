# Performance Report

## Benchmark Suite

The v1.0.0 repository includes an opt-in benchmark executable:

```bash
cmake -S . -B build-benchmarks \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_CLOUD=ON \
  -DHUMANOID_CORE_BUILD_BENCHMARKS=ON
cmake --build build-benchmarks --target humanoid_core_benchmark_suite
./build-benchmarks/benchmarks/humanoid_core_benchmark_suite
```

## Covered Areas

- Command execution pipeline.
- Mission step dispatch.
- Behavior tree tick.
- Rule-based planner.
- Telemetry state update/read path.
- Perception pipeline.
- Plugin registry lookup.
- Runtime scheduler.
- Cloud API catalog lookup and fleet heartbeat path.

## Results

Measured on the local release-validation host with:

```text
Release, ENABLE_UNITREE=OFF, ENABLE_ROS2=OFF, ENABLE_CLOUD=ON,
HUMANOID_CORE_BUILD_BENCHMARKS=ON, GCC 11.4.0
```

| Benchmark | Iterations | Total ms | Avg us/op |
| --- | ---: | ---: | ---: |
| Command execution pipeline | 1000 | 43.415 | 43.415 |
| Mission step dispatch | 1000 | 0.353 | 0.353 |
| Behavior tree tick | 1000 | 0.018 | 0.018 |
| Rule-based planner | 1000 | 1.750 | 1.750 |
| Telemetry subscription dispatch | 1000 | 0.116 | 0.116 |
| Perception pipeline | 1000 | 0.481 | 0.481 |
| Plugin registry lookup | 1000 | 0.057 | 0.057 |
| Runtime scheduler | 1000 | 40.761 | 40.761 |
| Cloud API catalog | 1000 | 0.196 | 0.196 |
| Cloud fleet heartbeat | 1000 | 0.387 | 0.387 |

## Interpretation

The benchmark suite is intended for regression detection, not absolute hardware
certification. Release managers should compare results against previous release
runs on the same host class and investigate significant regressions.
