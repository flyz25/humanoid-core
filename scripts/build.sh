#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
repo_root="$(cd -- "${script_dir}/.." >/dev/null 2>&1 && pwd)"
build_dir="${repo_root}/build"
build_type="${1:-Debug}"

case "${build_type}" in
  Debug | Release | RelWithDebInfo | MinSizeRel) ;;
  *)
    printf 'Unsupported build type: %s\n' "${build_type}" >&2
    exit 2
    ;;
esac

cmake -S "${repo_root}" \
  -B "${build_dir}" \
  -DCMAKE_BUILD_TYPE="${build_type}" \
  -DHUMANOID_CORE_BUILD_EXAMPLES=ON \
  -DHUMANOID_CORE_BUILD_TESTS=ON

cmake --build "${build_dir}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure
