#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OBUILD="$ROOT_DIR/build/obould"
OBDIR="$ROOT_DIR/.obould"
OUT_C="$ROOT_DIR/stdlib/Out.c"
OUT_JSON="$ROOT_DIR/stdlib/Out.json"
LIB_DIR="$ROOT_DIR/build/lib"

mkdir -p "$OBDIR"

cmake --build "$ROOT_DIR/build" -j


shopt -s nullglob
TESTS=("$ROOT_DIR/tests/sources"/test_*.obl)

for test in "${TESTS[@]}"; do
  [[ -f "$test" ]] || continue
  name=$(basename "$test" .obl)
  echo "==> $name"
  "$OBUILD" "$test" --main
  cc "$OBDIR/$name.o" "$LIB_DIR/libobould_std.a" "$LIB_DIR/libgc.a" -o "$OBDIR/$name"
  "$OBDIR/$name"
  echo
  done
