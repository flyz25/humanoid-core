# Versioning Policy

humanoid-core uses Semantic Versioning.

Current semantic version:

```text
1.0.1-validation
```

## Version Fields

- Major: `1`
- Minor: `0`
- Patch: `1`
- Prerelease: `validation`

CMake package compatibility uses the numeric project version `1.0.1`. The full
semantic version string is exposed separately as `1.0.1-validation`.

## Tag Format

Release tags use:

```text
vMAJOR.MINOR.PATCH[-PRERELEASE]
```

Examples:

```text
v1.0.0
v1.0.1-validation
v0.5.0
v0.12.0-alpha
```

## Compatibility Policy

Starting with `1.0.0`, public APIs are stable within the major version. Breaking
changes require an architecture issue, review, migration notes, and changelog
entry and must normally wait for the next major release.

The framework should avoid public API breakage unless a defect or safety issue
requires it.
