# Versioning Policy

humanoid-core uses Semantic Versioning.

Current semantic version:

```text
0.10.0-alpha
```

## Version Fields

- Major: `0`
- Minor: `10`
- Patch: `0`
- Prerelease: `alpha`

CMake package compatibility uses the numeric project version `0.10.0`. The full
semantic version string is exposed separately as `0.10.0-alpha`.

## Tag Format

Release tags use:

```text
vMAJOR.MINOR.PATCH[-PRERELEASE]
```

Examples:

```text
v0.10.0-alpha
v0.5.0
v1.0.0
```

## Compatibility Policy

While major version is `0`, public APIs may still evolve, but breaking changes
require an architecture issue, review, migration notes, and changelog entry.

The framework should avoid public API breakage unless a defect or safety issue
requires it.
