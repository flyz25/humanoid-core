# humanoid-core

humanoid-core is a C++17 Clean Architecture foundation for humanoid robot
applications. It defines stable interfaces, managers, and module boundaries
without binding the framework to Unitree, ROS2, DDS, OpenCV, AI runtimes, GUI
code, or robot communication backends.

## Architecture

```text
Applications
  -> Core Interfaces
    -> Managers
      -> Robot Adapters
        -> Vendor SDKs
```

Applications use framework interfaces and managers. Managers depend only on
interfaces. Robot adapters implement interfaces and isolate vendor SDKs.

## Directory Structure

```text
humanoid-core/
  apps/              Application targets owned by downstream products
  cmake/             CMake package and compiler configuration
  common/            Shared status, lifecycle, and version primitives
  config/            Configuration documentation and future examples
  configuration/     Configuration interfaces and manager
  core/              Core context and runtime metadata
  diagnostics/       Diagnostic interfaces and manager
  docs/              Architecture and engineering documentation
  examples/          Minimal buildable examples
  gesture/           Gesture interfaces and manager
  include/           Public umbrella include
  logging/           Logging interfaces and routing manager
  logs/              Runtime log location ignored by Git
  motion/            Motion interfaces and manager
  network/           Network interfaces and endpoint metadata
  robot/             Robot and adapter interfaces plus manager
  safety/            Safety interfaces and manager
  scripts/           Build and formatting scripts
  src/               Reserved package-level source root
  tests/             GoogleTest wiring and sample unit test
  third_party/       External dependency policy
  utilities/         Reusable framework utilities
```

Each module owns `include/`, `src/`, `docs/`, and a module `CMakeLists.txt`.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

GoogleTest is optional by default. When it is not installed, the unit test target
is skipped. To require it:

```bash
cmake -S . -B build -DHUMANOID_CORE_REQUIRE_GTEST=ON
```

## Install and Export

```bash
cmake --install build --prefix install
```

Installed consumers can link the exported package:

```cmake
find_package(humanoid_core CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE humanoid::humanoid_core)
```

## Coding Style

- C++17 only.
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
- No vendor SDK includes in framework modules.

## Naming Convention

- Namespace root: `humanoid`.
- Classes: `PascalCase`.
- Methods and functions: `camelCase`.
- Enum values: `kPascalCase`.
- Private members: trailing underscore.
- CMake targets: `humanoid_core_<module>` with aliases `humanoid::<module>`.

## Dependency Rules

Managers depend only on interfaces and shared primitives. Implementations,
adapters, vendor SDKs, middleware, and communication stacks must stay outside
core modules and be injected through abstract interfaces.

## Future Roadmap

Future repository layers may add Unitree G1, Unitree H1, Unitree H2, simulator,
and mock robot adapters; logging sinks; YAML configuration providers; DDS
transport implementations; and robot-family-specific motion, gesture, safety,
and diagnostics implementations.
