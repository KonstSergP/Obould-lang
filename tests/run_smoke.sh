#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
COMPILER="$ROOT_DIR/build/obould"
OBDIR="$ROOT_DIR/.obould"

cmake --build "$ROOT_DIR/build" -j


find tests/smoke -type f -name "test_*.obl" | while read -r test; do
  [[ -f "$test" ]] || continue
  name=$(basename "$test" .obl)
  echo "==> $name"
  "$COMPILER" "$test" --main --link
  "$OBDIR/$name"
  echo
  done
