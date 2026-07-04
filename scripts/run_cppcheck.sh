#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
repo_root="$(cd -- "${script_dir}/.." >/dev/null 2>&1 && pwd)"

if ! command -v cppcheck >/dev/null 2>&1; then
  printf 'cppcheck is not installed; skipping\n' >&2
  exit 0
fi

options=()
while IFS= read -r option; do
  [[ -z "${option}" ]] && continue
  [[ "${option}" == \#* ]] && continue
  options+=("${option}")
done < "${repo_root}/.cppcheck"

cppcheck "${options[@]}" "${repo_root}"
