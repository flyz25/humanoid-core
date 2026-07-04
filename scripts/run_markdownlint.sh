#!/usr/bin/env bash
set -euo pipefail

if command -v markdownlint-cli2 >/dev/null 2>&1; then
  markdownlint-cli2 "$@"
  exit 0
fi

if command -v markdownlint >/dev/null 2>&1; then
  markdownlint "$@"
  exit 0
fi

printf 'markdownlint is not installed; skipping\n' >&2
