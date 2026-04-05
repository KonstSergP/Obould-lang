#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
COMPILER="$ROOT_DIR/build/obould"
OBDIR="$ROOT_DIR/.obould"

cmake --build "$ROOT_DIR/build" -j


shopt -s nullglob
TESTS=("$ROOT_DIR/tests/sources"/test_*.obl)

for test in "${TESTS[@]}"; do
  [[ -f "$test" ]] || continue
  name=$(basename "$test" .obl)
  echo "==> $name"
  "$COMPILER" "$test" --main --link
  "$OBDIR/$name"
  echo
  done
