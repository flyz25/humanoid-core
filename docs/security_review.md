# Security Review

This document captures the Milestone 2.9 security review.

## Shell Execution

Repository scripts execute local build tools only:

- `scripts/build.sh` calls CMake and CTest.
- `scripts/format.sh` calls `clang-format` on repository C++ files and excludes
  the Unitree SDK2 submodule.
- `scripts/run_clang_tidy.sh` calls `clang-tidy` when available and requires a
  local `compile_commands.json`.
- `scripts/run_markdownlint.sh` calls markdown lint tools when available.

No script downloads dependencies, invokes remote shell content, or writes
outside the repository except normal build/install commands selected by the
caller.

## Filesystem Usage

Framework filesystem helpers use `std::filesystem` with `std::error_code`
overloads to avoid throwing exceptions during path inspection and directory
creation. The example configuration reader opens only the path supplied by the
caller and does not perform shell expansion.

## Hardcoded Values

`config/robot.yaml` contains non-secret example robot metadata:

- Unitree G1 model selection.
- Default Unitree control IP.
- Network interface name.
- Timeout and domain identifier.

No credentials, tokens, private keys, or secrets are present.

## Undefined Behavior Review

Static scans found no raw owning pointers, manual `new` or `delete`, `malloc`,
`free`, `reinterpret_cast`, `const_cast`, or `using namespace std` in repository
C++ files outside the vendored SDK.

## Vendor SDK Boundary

Unitree SDK2 headers and types are isolated to implementation files under
`plugins/unitree/sdk/`. SDK exceptions are translated to normalized SDK
abstraction results at that boundary and then to framework `Result` values by
the adapter-facing facade.

## Known Limitations

- Physical robot command execution was not run during hardening.
- Security posture of Unitree SDK2 itself is outside the scope of this
  repository audit.
- Downstream logging sinks and configuration providers must perform their own
  input validation and secret handling.
