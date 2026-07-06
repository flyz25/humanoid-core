# Contributing

humanoid-core is maintained as production robotics infrastructure. Contributions
must preserve Clean Architecture, vendor isolation, and the existing public API.

## Branch Strategy

- `main` is the integration branch and must remain buildable.
- Feature and hardening work should be developed on short-lived branches.
- Pull requests must include the validation commands used by the author.
- Do not commit generated build directories, logs, or local IDE metadata.

Recommended branch names:

- `feature/<short-topic>`
- `bugfix/<short-topic>`
- `hardening/<short-topic>`
- `docs/<short-topic>`
- `release/<version>`

## Commit Style

Use concise, imperative commit subjects:

```text
Harden Unitree wrapper exception handling
Add factory registry smoke coverage
Document SDK submodule pinning
```

Keep commits focused. Avoid mixing formatting, build-system changes, and
behavioral changes in the same commit.

Commit subjects should use an imperative verb and avoid vague prefixes:

```text
Add release checklist
Document thread-safety contract
Fix package config metadata
```

## Review Process

- Every pull request should use the repository pull request template.
- Public API, architecture, and dependency-boundary changes require maintainer
  review.
- Vendor adapter and SDK wrapper changes require vendor integration review.
- Build-system and packaging changes require build review.
- Security-sensitive changes should not be discussed in public pull requests
  until disclosure is coordinated through `SECURITY.md`.

## Code Review Expectations

Reviewers should check:

- architecture and dependency direction,
- public API compatibility,
- SDK header leakage,
- CMake `PUBLIC` / `PRIVATE` / `INTERFACE` correctness,
- test and CI evidence,
- documentation updates,
- robot safety impact.

Authors should respond to review comments with code changes, rationale, or a
linked follow-up issue.

## Coding Style

- C++20 for framework code. Vendor SDK wrapper translation units may use a
  vendor-compatible dialect when required by official SDK headers.
- LLVM formatting, configured by `.clang-format`.
- Public headers, classes, and functions require Doxygen comments.
- Prefer RAII and value ownership.
- Use `std::unique_ptr` for exclusive ownership and `std::shared_ptr` only for
  shared injected interfaces.
- Use `enum class`, `std::chrono`, and `std::filesystem`.
- Do not introduce global state, singletons, raw owning pointers, or vendor SDK
  headers outside SDK wrapper implementation files.

## Formatting

Install `clang-format`, then run:

```bash
scripts/format.sh
```

The formatting script excludes build directories and the pinned Unitree SDK2
submodule.

## Static Analysis

Configure once so `compile_commands.json` exists:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=ON
```

Run clang-tidy when available:

```bash
scripts/run_clang_tidy.sh
```

Run cppcheck when available:

```bash
scripts/run_cppcheck.sh
```

The `.cppcheck` file contains the repository default options consumed by the
script.

## Tests

Run the standard local validation:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Validate the SDK-disabled build:

```bash
cmake -S . -B build-no-unitree -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=OFF
cmake --build build-no-unitree --parallel
ctest --test-dir build-no-unitree --output-on-failure
```

GoogleTest-based tests are built when GoogleTest is available. The smoke test
does not require GoogleTest or physical robot hardware.

## Hardware Validation Program

Physical robot validation is performed through the Hardware Validation Program
under `validation/`. Automation may create runs and record operator-supplied
results, but it must not claim physical hardware tests passed without human
evidence.

List HVP cases:

```bash
python3 validation/scripts/hvp.py list-cases
```

Create a hardware validation run:

```bash
python3 validation/scripts/hvp.py new-run --operator "<name>"
```

Generated run directories under `validation/reports/runs/` are local evidence
artifacts and must not be committed.

## Pre-Commit Hooks

Install pre-commit:

```bash
python3 -m pip install --user pre-commit
pre-commit install
```

Run all hooks manually:

```bash
pre-commit run --all-files
```

The hook set checks trailing whitespace, final newlines, YAML syntax,
clang-format, clang-tidy when `compile_commands.json` and `clang-tidy` are
available, and Markdown linting when a markdownlint executable is installed.

## Running CI Locally

The GitHub Actions matrix builds these combinations:

- `Debug` and `Release`
- `ENABLE_UNITREE=ON` and `ENABLE_UNITREE=OFF`
- `ENABLE_ROS2=ON` and `ENABLE_ROS2=OFF`
- `ENABLE_CLOUD=ON` and `ENABLE_CLOUD=OFF`

To reproduce one matrix leg locally:

```bash
cmake -S . -B build-ci -DCMAKE_BUILD_TYPE=Release -DENABLE_UNITREE=ON \
  -DHUMANOID_CORE_BUILD_EXAMPLES=ON \
  -DHUMANOID_CORE_BUILD_TESTS=ON \
  -DHUMANOID_CORE_WARNINGS_AS_ERRORS=ON
cmake --build build-ci --parallel
ctest --test-dir build-ci --output-on-failure
cmake --install build-ci --prefix install-ci
```

## Dependency Rules

Applications must not include vendor SDK headers or directly construct concrete
robot adapters. New robot vendors must be introduced through factories,
adapters, and SDK wrappers while preserving the existing interfaces.
