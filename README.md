# humanoid-core

humanoid-core is a C++20 Clean Architecture foundation for humanoid robot
applications. The framework remains vendor independent: applications depend on
interfaces and managers, while robot vendors are integrated as adapter plugins
behind factories and SDK wrappers.

The current SDK integration supports Unitree G1 through Unitree SDK2. Unitree
SDK2 is included as a pinned Git submodule at `third_party/unitree_sdk2`; it is
not installed into `/usr/local` and is not required as a system dependency.

Current release: `0.3.0-alpha`

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
              -> SdkWrapper
                -> LocoAdapter / HandAdapter / AudioAdapter
                -> Unitree SDK2
```

Only implementation files under `plugins/unitree/sdk/` include Unitree SDK2
headers. `src/sdk/LocoClientWrapper.cpp` is a compatibility facade over the SDK
abstraction layer and does not include vendor SDK headers.

Milestone 4.6 adds read-only SDK2 communication monitoring inside the SDK
abstraction. Heartbeats, connection timeout handling, automatic reconnect, and
state synchronization use read-only SDK queries. The monitoring path does not
issue walking, standing, hand, audio, or other actuator commands. A
`RobotStateManager` can be injected through `UnitreeRobotFactory` so synchronized
state is published as vendor-independent `humanoid::core::RobotState`.
On Linux, SDK initialization also validates network-interface existence and
route netlink socket access before constructing the Unitree SDK client, so
restricted environments fail gracefully through `Result`.

Milestone 4.7 adds vendor-specific SDK adapters inside the same boundary:
`LocoAdapter`, `HandAdapter`, and `AudioAdapter`. They translate framework-owned
motion, hand/gesture, and audio commands into SDK2 calls without adding mission
logic, planning, AI, or behavior execution. Unitree G1 SDK2 exposes
hand-related behavior through the arm action service; unsupported finger-level
open, close, and grip commands are rejected explicitly instead of being faked.

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

Milestone 4 adds a separate plugin infrastructure target:

```text
Application or plugin host
  -> humanoid::plugins
    -> humanoid::common
```

The aggregate core target `humanoid::humanoid_core` does not link against
`humanoid::plugins`. Plugin hosts can use `PluginRegistry` for metadata and
lifecycle visibility, and `PluginFactory` for dependency-injected creator
registration, creation, destruction, and enumeration.

Milestone 4.4 adds the first concrete plugin package:

```text
Application or plugin host
  -> PluginFactory
    -> UnitreeG1Plugin
      -> UnitreeG1Adapter
        -> mock RobotState
```

The Unitree G1 plugin skeleton is SDK-free. It validates plugin packaging,
metadata, lifecycle, adapter construction, and conservative mock state feedback.
It does not communicate with Unitree SDK2 and does not command robot movement.

Milestone 4.8 adds buildable integration examples that exercise the composition
paths for plugin registry/factory wiring, plugin lifecycle, robot factory
connection, telemetry, adapter capability queries, and Unitree SDK-boundary
command adapters. These examples do not change dependency direction: core still
does not depend on plugins, and application-facing examples never include
Unitree SDK2 headers.

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
  plugins/                   Plugin interfaces, registry, factory, and plugin packages
  plugins/unitree/g1/        SDK-free Unitree G1 plugin skeleton
  plugins/unitree/sdk/       Unitree SDK2 abstraction boundary
  robot/                     Robot interfaces plus manager
  safety/                    Safety interfaces and manager
  scripts/                   Build and formatting scripts
  src/adapters/unitree/      Optional SDK-backed Unitree adapter target
  src/factory/               Robot factory registry
  src/sdk/                   Legacy adapter-facing SDK facades
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

The SDK-free Unitree G1 plugin skeleton under `plugins/unitree/g1` is still
built when `ENABLE_UNITREE=OFF`.

Manual SDK override:

```bash
cmake -S . -B build -DUNITREE_SDK2_ROOT=/path/to/unitree_sdk2
```

## Examples

Basic framework initialization:

```bash
./build/examples/humanoid_core_basic_initialization
```

Plugin registry and factory loading example:

```bash
./build/examples/humanoid_core_plugin_loading_example
```

Full composition example using plugin factory, Unitree plugin skeleton,
`RobotStateManager`, and `TelemetryService`:

```bash
./build/examples/humanoid_core_framework_integration_example
```

Factory-based robot connection example:

```bash
./build/examples/humanoid_core_basic_robot_connection config/robot.yaml
./build/examples/humanoid_core_robot_connection_example config/robot.yaml
```

By default, the connection example validates configuration, registers available
factories, creates the adapter through the registry, and exits without
opening physical robot communication. To execute the read-only physical
communication workflow:

```bash
./build/examples/humanoid_core_basic_robot_connection config/robot.yaml --execute
./build/examples/humanoid_core_robot_connection_example config/robot.yaml --execute
```

The read-only hardware workflow is:

```text
Initialize -> Connect -> Disconnect -> Shutdown
```

Telemetry example:

```bash
./build/examples/humanoid_core_telemetry_example
```

Capability query example:

```bash
./build/examples/humanoid_core_capability_query_example
```

When Unitree SDK2 is available and `ENABLE_UNITREE=ON`, the SDK-boundary command
adapter validation example is also built:

```bash
./build/examples/humanoid_core_unitree_sdk_boundary_example
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
3. Hide vendor SDK headers inside a vendor SDK abstraction boundary.
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

humanoid-core uses Semantic Versioning. Current version: `0.3.0-alpha`.

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
- `docs/integration/Milestone_4_8_Integration_Report.md`
- `docs/architecture/plugin_architecture.md`
- `docs/thread_safety.md`
- `docs/dependency_graph.md`
- `docs/security_review.md`
- `docs/adr/`
