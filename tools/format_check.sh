#!/usr/bin/env bash
set -euo pipefail

FILES=$(git ls-files \
  'nxsd/src/**/*.cpp' \
  'nxsd/include/**/*.h' \
  'nxsd/tests/**/*.cpp' \
)

if [ -z "$FILES" ]; then
  echo "No files to format-check."
  exit 0
fi

clang-format --dry-run --Werror $FILES
