# Validation Environment

## Required Records

Each hardware validation run shall record:

- site,
- operator,
- safety lead,
- robot vendor and model,
- robot serial number,
- firmware version,
- SDK version,
- humanoid-core version,
- Git commit,
- build type,
- CMake options,
- operating system,
- network type,
- battery level,
- test surface,
- safety equipment.

## Recommended Host

- Ubuntu 22.04 or approved target operating system.
- C++20 compiler for local builds.
- Python 3 for validation tooling.
- Synchronized system time.
- Sufficient disk space for logs and video references.

## Evidence Storage

Large videos should be stored in the lab evidence system. The HVP records video
references instead of copying large files into the repository.
