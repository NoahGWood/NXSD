#!/usr/bin/env bash
set -euo pipefail


FILES=$(git ls-files \
  'NTemplate/src/**/*.cpp' \
  'NTemplate/include/**/*.h' \
  'NTemplate/tests/**/*.cpp' \
)

if [ -z "$FILES" ]; then
  echo "No files to format."
  exit 0
fi

clang-format -i $FILES
