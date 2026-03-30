#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OBUILD="$ROOT_DIR/build/obould"
OBDIR="$ROOT_DIR/.obould"
OUT_C="$ROOT_DIR/stdlib/Out.c"
OUT_JSON="$ROOT_DIR/stdlib/Out.json"
GC_LIB_DIR="$ROOT_DIR/build/lib"

mkdir -p "$OBDIR"

cmake --build "$ROOT_DIR/build" -j
cc -c "$OUT_C" -o "$OBDIR/Out.o"
cp "$OUT_JSON" "$OBDIR/Out.json"

mapfile -t TESTS < <(find "$ROOT_DIR/tests/sources" -maxdepth 1 -type f -name "test_*.obl" | sort)

for test in "${TESTS[@]}"; do
  name=$(basename "$test" .obl)
  echo "==> $name"
  "$OBUILD" "$test" --main
  cc "$OBDIR/$name.o" "$OBDIR/Out.o" "$GC_LIB_DIR/libgc.a" -o "$OBDIR/$name"
  "$OBDIR/$name"
  echo
  done
