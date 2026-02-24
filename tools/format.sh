#!/usr/bin/env bash
set -euo pipefail


FILES=$(git ls-files \
  'nxsd/src/**/*.cpp' \
  'nxsd/include/**/*.h' \
  'nxsd/tests/**/*.cpp' \
)

if [ -z "$FILES" ]; then
  echo "No files to format."
  exit 0
fi

clang-format -i $FILES
