#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
repo_root="$(cd -- "${script_dir}/.." >/dev/null 2>&1 && pwd)"
build_dir="${HUMANOID_CORE_CLANG_TIDY_BUILD_DIR:-${repo_root}/build}"

if ! command -v clang-tidy >/dev/null 2>&1; then
  printf 'clang-tidy is not installed; skipping\n' >&2
  exit 0
fi

if [[ ! -f "${build_dir}/compile_commands.json" ]]; then
  printf 'compile_commands.json not found in %s; skipping clang-tidy\n' "${build_dir}" >&2
  exit 0
fi

files=()
if (($# > 0)); then
  for file in "$@"; do
    case "${file}" in
      third_party/unitree_sdk2/* | build/* | build-*/* | cmake-build-*/*) ;;
      *.cpp | *.cc | *.cxx) files+=("${file}") ;;
    esac
  done
else
  while IFS= read -r file; do
    files+=("${file}")
  done < <(git -C "${repo_root}" ls-files '*.cpp' '*.cc' '*.cxx' \
    ':!:third_party/unitree_sdk2/*' ':!:build/*' ':!:build-*/*' ':!:cmake-build-*/*')
fi

if ((${#files[@]} == 0)); then
  printf 'no C++ source files selected for clang-tidy\n'
  exit 0
fi

clang-tidy -p "${build_dir}" "${files[@]}"
