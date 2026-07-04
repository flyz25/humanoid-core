#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
repo_root="$(cd -- "${script_dir}/.." >/dev/null 2>&1 && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
  printf 'clang-format is not installed\n' >&2
  exit 127
fi

find "${repo_root}" \
  \( -path "${repo_root}/build" -o -path "${repo_root}/build-*" -o -path "${repo_root}/cmake-build-*" -o -path "${repo_root}/third_party/unitree_sdk2" -o -path "${repo_root}/.git" \) -prune \
  -o \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) -print0 \
  | xargs -0 -r clang-format -i
