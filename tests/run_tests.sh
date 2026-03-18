#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OBUILD="$ROOT_DIR/cmake-build-debug/obould"
OBDIR="$ROOT_DIR/.obould"
OUT_C="$ROOT_DIR/stdlib/Out.c"
OUT_JSON="$ROOT_DIR/stdlib/Out.json"

mkdir -p "$OBDIR"

cc -c "$OUT_C" -o "$OBDIR/Out.o"
cp "$OUT_JSON" "$OBDIR/Out.json"

TESTS=(
  "$ROOT_DIR/tests/sources/test_fibonacci.obl"
  "$ROOT_DIR/tests/sources/test_arrays.obl"
  "$ROOT_DIR/tests/sources/test_structs_typeguards.obl"
)

for test in "${TESTS[@]}"; do
  name=$(basename "$test" .obl)
  echo "==> $name"
  "$OBUILD" "$test" --main
  cc -no-pie "$OBDIR/$name.o" "$OBDIR/Out.o" -o "$OBDIR/$name"
  "$OBDIR/$name"
  echo
  done
