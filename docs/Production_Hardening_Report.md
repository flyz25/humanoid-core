# Production Hardening Report

Milestone: 2.9 Production Hardening

## Changes Made

- Added repository static-analysis configuration:
  - `.clang-format` was verified as LLVM/C++20 formatting policy.
  - `.clang-tidy`
  - `.cppcheck`
  - `.editorconfig`
  - `.markdownlint.yaml`
- Added pre-commit configuration and helper scripts:
  - `.pre-commit-config.yaml`
  - `scripts/run_clang_tidy.sh`
  - `scripts/run_markdownlint.sh`
- Added GitHub Actions CI matrix for Debug, Release, Unitree enabled, and
  Unitree disabled builds.
- Added `CONTRIBUTING.md` with coding, testing, static-analysis, pre-commit,
  and local CI instructions.
- Added Architecture Decision Records:
  - ADR-0001 Clean Architecture
  - ADR-0002 Vendor Isolation
  - ADR-0003 Git Submodule Strategy
  - ADR-0004 Factory Registry
  - ADR-0005 SDK Wrapper Boundary
- Added thread-safety, dependency graph, and security review documentation.
- Hardened install rules by adding canonical installed integration headers under
  `include/humanoid/...` while retaining existing compatibility install paths.

## Validation

Validation was performed locally on Ubuntu/WSL2 using CMake and C++20:

```text
cmake -S . -B build-hardening-debug-unitree -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=ON -DHUMANOID_CORE_WARNINGS_AS_ERRORS=ON
cmake -S . -B build-hardening-release-unitree -DCMAKE_BUILD_TYPE=Release -DENABLE_UNITREE=ON -DHUMANOID_CORE_WARNINGS_AS_ERRORS=ON
cmake -S . -B build-hardening-debug-no-unitree -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=OFF -DHUMANOID_CORE_WARNINGS_AS_ERRORS=ON
cmake -S . -B build-hardening-release-no-unitree -DCMAKE_BUILD_TYPE=Release -DENABLE_UNITREE=OFF -DHUMANOID_CORE_WARNINGS_AS_ERRORS=ON
cmake --build build-hardening-debug-unitree --parallel
cmake --build build-hardening-release-unitree --parallel
cmake --build build-hardening-debug-no-unitree --parallel
cmake --build build-hardening-release-no-unitree --parallel
ctest --test-dir build-hardening-debug-unitree --output-on-failure
ctest --test-dir build-hardening-release-unitree --output-on-failure
ctest --test-dir build-hardening-debug-no-unitree --output-on-failure
ctest --test-dir build-hardening-release-no-unitree --output-on-failure
cmake --install build-hardening-debug-unitree --prefix /tmp/humanoid-core-hardening-install-unitree
cmake --install build-hardening-debug-no-unitree --prefix /tmp/humanoid-core-hardening-install-no-unitree
cmake --find-package -DNAME=humanoid_core -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST
```

The Unitree safe-mode example was run without commanding hardware.

## Static Analysis Status

Static-analysis configuration is present. Tool execution depends on local tool
availability:

- `clang-format` is configured by `.clang-format`.
- `clang-tidy` is configured by `.clang-tidy` and can be run through
  `scripts/run_clang_tidy.sh`.
- `cppcheck` defaults are captured in `.cppcheck` and can be run through
  `scripts/run_cppcheck.sh`.
- Markdown linting is configured by `.markdownlint.yaml` and the pre-commit
  hook wrapper.

Local tool status during hardening:

- `clang-tidy` was available and run on changed C++ sources; repository-code
  warnings were resolved. Remaining reported diagnostics were suppressed
  non-user/vendor-header diagnostics.
- `cppcheck` was not installed; the wrapper was verified to skip cleanly.
- `pre-commit` was not installed in the local environment.
- `markdownlint` was not installed; the wrapper was verified to skip cleanly.
- `shellcheck` was not installed; all repository shell scripts passed `bash -n`
  syntax validation.

## CI Status

GitHub Actions workflow added at `.github/workflows/ci.yml`.

The workflow performs:

- recursive checkout with submodules,
- CMake configure,
- build,
- CTest,
- install,
- package-config validation,
- install-tree artifact upload.

The workflow was added but not executed in hosted GitHub Actions during this
local hardening run.

## Known Limitations

- GoogleTest is optional and is only built when available in the environment.
- Hardware execution with `--execute` is intentionally not part of automated CI.
- Milestone 1 managers remain externally synchronized by contract; thread-safety
  guarantees are documented in `docs/thread_safety.md`.
- Compatibility install paths are retained for existing consumers while
  canonical `include/humanoid/...` install copies are provided.

## Production Readiness

The repository is ready for continued production framework development at the
current milestone scope. Remaining work is operational maturity rather than
architecture correction: CI execution in the hosted repository, optional
GoogleTest availability, and future hardware-in-the-loop validation outside the
core framework.
