#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"


FILES=$(git ls-files \
  'NTemplate/src/**/*.cpp' \
  'NTemplate/include/**/*.h' \
  'NTemplate/tests/**/*.cpp' \
)

clang-tidy-14 \
  --use-color \
  -p="$BUILD_DIR" \
  -warnings-as-errors='*' \
  $FILES
