# Release Checklist

Use this checklist for every tagged release.

## Version

- [ ] Confirm semantic version in `CMakeLists.txt`.
- [ ] Confirm prerelease suffix policy in README and package config.
- [ ] Confirm `common/include/humanoid/common/Version.hpp` exposes the intended
      version metadata.
- [ ] Confirm `CHANGELOG.md` has an entry for the release.
- [ ] Confirm Git tag follows `vMAJOR.MINOR.PATCH[-PRERELEASE]`.

## Build

- [ ] Configure Debug with `ENABLE_UNITREE=ON`.
- [ ] Configure Release with `ENABLE_UNITREE=ON`.
- [ ] Configure Debug with `ENABLE_UNITREE=OFF`.
- [ ] Configure Release with `ENABLE_UNITREE=OFF`.
- [ ] Build every configuration with warnings as errors.

## Tests

- [ ] Run CTest for every build configuration.
- [ ] Confirm GoogleTest tests run when GoogleTest is available.
- [ ] Confirm no physical robot command path is run unintentionally.
- [ ] Record any hardware-in-the-loop validation separately.

## Static Analysis

- [ ] Run `scripts/format.sh` or verify formatting.
- [ ] Run `scripts/run_clang_tidy.sh`.
- [ ] Run `scripts/run_cppcheck.sh` when cppcheck is available.
- [ ] Run pre-commit hooks when available.

## CI

- [ ] Confirm GitHub Actions matrix passes.
- [ ] Confirm install artifacts are generated.
- [ ] Confirm package config validation passes.

## Documentation

- [ ] Review README.
- [ ] Review CONTRIBUTING.
- [ ] Review SECURITY.
- [ ] Review ADRs for changes.
- [ ] Review dependency and thread-safety documentation.

## SDK and Dependencies

- [ ] Confirm `git submodule status --recursive`.
- [ ] Confirm Unitree SDK2 tag and commit.
- [ ] Confirm `third_party/unitree_sdk2.version`.
- [ ] Confirm no SDK headers leak outside SDK wrapper implementation files.

## Release Notes

- [ ] Summarize user-visible changes.
- [ ] Summarize compatibility notes.
- [ ] Summarize validation performed.
- [ ] Include known limitations.

## Tag

- [ ] Create annotated tag: `git tag -a vMAJOR.MINOR.PATCH[-PRERELEASE]`.
- [ ] Push tag after CI is green.
- [ ] Publish release notes.
