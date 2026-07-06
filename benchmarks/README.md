# Benchmark Suite

The benchmark suite is an opt-in local executable that measures representative
framework hot paths without external benchmark dependencies.

Build:

```bash
cmake -S . -B build-benchmarks \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_CLOUD=ON \
  -DHUMANOID_CORE_BUILD_BENCHMARKS=ON
cmake --build build-benchmarks --target humanoid_core_benchmark_suite
```

Run:

```bash
./build-benchmarks/benchmarks/humanoid_core_benchmark_suite
```

Measured areas:

- Command execution pipeline.
- Mission step dispatch.
- Behavior tree tick.
- Rule-based planner.
- Telemetry throughput.
- Perception pipeline.
- Plugin registry.
- Runtime scheduler.
- Cloud API and fleet operations.
