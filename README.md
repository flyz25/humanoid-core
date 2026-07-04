# humanoid-core

humanoid-core is a C++20 Clean Architecture foundation for humanoid robot
applications. The framework remains vendor independent: applications depend on
interfaces and managers, while robot vendors are integrated as adapter plugins
behind factories and SDK wrappers.

The current SDK integration supports Unitree G1 through Unitree SDK2. Unitree
SDK2 is included as a pinned Git submodule at `third_party/unitree_sdk2`; it is
not installed into `/usr/local` and is not required as a system dependency.

Current release: `0.1.0-alpha`

## Architecture

```text
Applications
  -> Managers
    -> Interfaces
      -> Robot Factory
        -> Robot Adapter
          -> SDK Wrapper
            -> Vendor SDK
```

Applications receive `std::unique_ptr<humanoid::adapters::IRobotAdapter>` from
`RobotFactoryRegistry`. Applications never instantiate concrete adapters and
never include Unitree SDK headers.

Unitree G1 dependency flow:

```text
Application
  -> RobotFactoryRegistry
    -> IRobotFactory
      -> IRobotAdapter
        -> UnitreeRobotFactory
          -> UnitreeG1Adapter
            -> LocoClientWrapper
              -> Unitree SDK2
```

Only `src/sdk/LocoClientWrapper.cpp` includes Unitree SDK2 headers.

Milestone 3 adds a vendor-independent runtime state path:

```text
Robot adapter or state producer
  -> RobotState
    -> RobotStateManager
      -> TelemetryService
        -> Subscriber callbacks
```

The state and telemetry path remains SDK-free. `RobotStateManager` is injected
through `CoreContext`, and `TelemetryService` receives that manager explicitly.

## Directory Structure

```text
humanoid-core/
  apps/                      Application targets owned by downstream products
  cmake/                     CMake package and dependency discovery modules
  common/                    Shared status, lifecycle, and version primitives
  config/                    Robot configuration examples
  configuration/             Configuration interfaces and manager
  core/                      Core context, runtime metadata, and state model
  diagnostics/               Diagnostic interfaces and manager
  docs/                      Architecture and engineering documentation
  examples/                  Buildable examples
  gesture/                   Gesture interfaces and manager
  include/humanoid/adapters/ Public robot adapter and factory contracts
  logging/                   Logging interfaces and routing manager
  motion/                    Motion interfaces and manager
  network/                   Network interfaces and endpoint metadata
  robot/                     Robot interfaces plus manager
  safety/                    Safety interfaces and manager
  scripts/                   Build and formatting scripts
  src/adapters/unitree/      Unitree adapter plugin
  src/factory/               Robot factory registry
  src/sdk/                   Vendor SDK wrappers
  src/services/              Vendor-independent runtime services
  tests/                     Smoke test and optional GoogleTest tests
  third_party/unitree_sdk2/  Pinned Unitree SDK2 submodule
  utilities/                 Reusable framework utilities
```

## Unitree SDK2

Pinned SDK:

```text
Repository: https://github.com/unitreerobotics/unitree_sdk2.git
Tag: 2.0.2
Commit: 811bc77
```

Initialize the submodule:

```bash
git submodule update --init --recursive
```

Build with Unitree enabled, which is the default:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITREE=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If the SDK is unavailable, CMake disables only the Unitree adapter targets and
continues building the vendor-independent framework.

Manual SDK override:

```bash
cmake -S . -B build -DUNITREE_SDK2_ROOT=/path/to/unitree_sdk2
```

## Examples

Basic framework initialization:

```bash
./build/examples/humanoid_core_basic_initialization
```

Factory-based robot connection example:

```bash
./build/examples/humanoid_core_basic_robot_connection config/robot.yaml
```

By default, the connection example validates configuration, registers available
factories, creates the adapter through the registry, and exits without
commanding physical hardware. To execute the physical robot workflow:

```bash
./build/examples/humanoid_core_basic_robot_connection config/robot.yaml --execute
```

The hardware workflow is:

```text
Initialize -> Connect -> StandUp -> BalanceStand -> Disconnect -> Shutdown
```

## Configuration

`config/robot.yaml` uses the current robot schema:

```yaml
robot:
  vendor: Unitree
  model: G1
  ip: 192.168.123.161
  network_interface: eth0
  timeout_ms: 500
  domain_id: 0
  serial_number:
  firmware:
```

The core configuration module still exposes interfaces only. There is no general
YAML parser in the framework.

## Tests

The smoke test is always built when `BUILD_TESTING` is enabled and does not
require a physical robot:

```bash
ctest --test-dir build --output-on-failure
```

GoogleTest tests are built when GoogleTest is available. To require GoogleTest:

```bash
cmake -S . -B build -DHUMANOID_CORE_REQUIRE_GTEST=ON
```

Milestone 3 state and telemetry coverage is provided by the always-built
`humanoid_core_robot_state_unit_test` CTest target. It validates state defaults,
state updates, reset behavior, concurrent manager access, telemetry subscriber
callbacks, unsubscribe behavior, invalid telemetry startup, and a bounded
state-manager performance sanity check.

API-level documentation:

- `docs/api/robot_state_and_telemetry.md`
- `docs/services/telemetry_service.md`

## Install and Export

```bash
cmake --install build --prefix install
```

Installed consumers can link the exported package:

```cmake
find_package(humanoid_core CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE humanoid::humanoid_core)
```

When the Unitree adapter was built, consumers must make the pinned SDK available
through `UNITREE_SDK2_ROOT` or the same repository submodule layout.

Installed CMake package metadata exposes:

```cmake
HUMANOID_CORE_VERSION
HUMANOID_CORE_VERSION_PRERELEASE
HUMANOID_CORE_SEMANTIC_VERSION
```

## Updating SDK Version

To switch SDK versions:

```bash
git -C third_party/unitree_sdk2 fetch --tags
git -C third_party/unitree_sdk2 checkout <tag-or-commit>
git add third_party/unitree_sdk2 third_party/unitree_sdk2.version
```

Update `third_party/unitree_sdk2.version` with the exact tag and commit. Do not
track a floating branch head for production builds.

## Adding New Robot Vendors

Add new vendors without modifying application code:

1. Implement `humanoid::adapters::IRobotAdapter`.
2. Implement `humanoid::adapters::IRobotFactory`.
3. Hide vendor SDK headers inside a wrapper under `src/sdk/` or a vendor plugin.
4. Register the factory with `RobotFactoryRegistry`.
5. Keep application code dependent only on `IRobotFactory`, `IRobotAdapter`, and
   manager interfaces.

## Coding Style

- C++20 for framework code. Vendor SDK wrapper translation units may use a
  vendor-compatible dialect when required to compile official SDK headers.
- LLVM formatting.
- Doxygen comments on public headers, classes, and functions.
- RAII for ownership and cleanup.
- Smart pointers for injected interfaces.
- `enum class` for scoped enumerations.
- `std::chrono` for time.
- `std::filesystem` for paths.
- No singletons.
- No global variables.
- No `using namespace std`.
- No raw owning pointers.
- No vendor SDK includes outside SDK wrappers.

## Development Workflow

See `CONTRIBUTING.md` for formatting, static analysis, pre-commit hooks, local
CI reproduction, and contribution rules.

Repository governance:

- Use GitHub issue templates for bugs, feature requests, and architecture
  changes.
- Use the pull request template checklist before requesting review.
- Follow CODEOWNERS review routing for public APIs, build rules, vendor
  adapters, and documentation.
- Report security issues through `SECURITY.md`, not public issues.

## Versioning Policy

humanoid-core uses Semantic Versioning. Current version: `0.1.0-alpha`.

Release tags use:

```text
vMAJOR.MINOR.PATCH[-PRERELEASE]
```

See `docs/versioning.md` and `docs/release_checklist.md` for release process
details.

## Issue and Security Reporting

- Bugs: use `.github/ISSUE_TEMPLATE/bug_report.md`.
- Feature requests: use `.github/ISSUE_TEMPLATE/feature_request.md`.
- Architecture changes: use `.github/ISSUE_TEMPLATE/architecture_change.md`.
- Vulnerabilities or unsafe robot-control issues: follow `SECURITY.md`.

Production hardening documentation:

- `docs/Production_Hardening_Report.md`
- `docs/Repository_Governance_Report.md`
- `docs/thread_safety.md`
- `docs/dependency_graph.md`
- `docs/security_review.md`
- `docs/adr/`
