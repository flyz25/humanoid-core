# Repository Governance Report

Milestone: Repository Governance

## Files Added

- `.github/ISSUE_TEMPLATE/bug_report.md`
- `.github/ISSUE_TEMPLATE/feature_request.md`
- `.github/ISSUE_TEMPLATE/architecture_change.md`
- `.github/pull_request_template.md`
- `.github/CODEOWNERS`
- `SECURITY.md`
- `CHANGELOG.md`
- `docs/release_checklist.md`
- `docs/license_header_template.md`
- `docs/versioning.md`
- `docs/Repository_Governance_Report.md`

## Files Improved

- `README.md`
- `CONTRIBUTING.md`
- `.gitignore`
- `CMakeLists.txt`
- `cmake/humanoid_coreConfig.cmake.in`
- `common/include/humanoid/common/Version.hpp`

## Governance Policies

- Issue templates route bug reports, feature requests, and architecture-change
  proposals into different review paths.
- Pull requests require architecture, API, SDK leakage, CI, test, warning, and
  documentation checks.
- CODEOWNERS defines initial ownership for API, build, vendor integration,
  documentation, and repository governance areas.
- SECURITY.md defines private reporting expectations and robot safety handling.
- Versioning follows SemVer with tag format `vMAJOR.MINOR.PATCH[-PRERELEASE]`.

## Validation Performed

Validation completed on Ubuntu/WSL2 with the pinned Unitree SDK2 submodule.

- Configured and built Debug and Release with `ENABLE_UNITREE=ON`.
- Configured and built Debug and Release with `ENABLE_UNITREE=OFF`.
- Built all targets with `HUMANOID_CORE_WARNINGS_AS_ERRORS=ON`.
- Ran CTest for all four build configurations; the smoke test passed in each.
- Ran `humanoid_core_basic_initialization`; output was
  `Humanoid Core Initialized`.
- Ran `humanoid_core_basic_robot_connection` with Unitree enabled and without
  `--execute`; it created the Unitree G1 adapter and did not command hardware.
- Ran `humanoid_core_basic_robot_connection` with Unitree disabled; it exited
  gracefully because the adapter was unavailable.
- Installed Unitree-enabled and Unitree-disabled builds into `/tmp` prefixes.
- Verified `find_package(humanoid_core CONFIG REQUIRED)` discovery for both
  installed prefixes.
- Verified exported package version variables:
  `HUMANOID_CORE_VERSION`, `HUMANOID_CORE_VERSION_PRERELEASE`, and
  `HUMANOID_CORE_SEMANTIC_VERSION`.
- Parsed `.github/workflows/ci.yml` and `.pre-commit-config.yaml` with local
  YAML tooling.
- Ran `git diff --check`; no whitespace errors were reported.
- Ran shell syntax checks for scripts under `scripts/`.
- Re-scanned Unitree SDK references; SDK headers remain isolated to the
  Unitree SDK boundary.
- Re-scanned CMake files for global include/link directives; none were found.
- Verified referenced governance and process documents exist.

No runtime behavior, robot adapter behavior, SDK wrapper behavior, manager
logic, registry logic, or public architecture design was changed.

## Repository Readiness

The repository is prepared for collaborative development. Governance files now
define how contributors report issues, propose architecture changes, submit pull
requests, handle security issues, and prepare releases.
