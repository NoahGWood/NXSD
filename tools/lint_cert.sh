#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"


FILES=$(git ls-files \
  'nxsd/src/**/*.cpp' \
  'nxsd/include/**/*.h' \
  'nxsd/tests/**/*.cpp' \
)

clang-tidy-14 \
  --use-color \
  -p="$BUILD_DIR" \
  -warnings-as-errors='*' \
  $FILES
