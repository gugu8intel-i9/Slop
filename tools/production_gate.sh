#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CC=${CC:-gcc}
TMP=${TMPDIR:-/tmp}/slop_production_gate_$$
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

PREBUILT_COMPILER="bootstrap/prebuilt/linux-x86_64/slop-compiler"
PREBUILT_NATIVE="bootstrap/prebuilt/linux-x86_64/slop-native-backend"
PREBUILT_PIPELINE="bootstrap/prebuilt/linux-x86_64/slop-pipeline"

if [ ! -x "$PREBUILT_COMPILER" ]; then
  echo "missing executable $PREBUILT_COMPILER" >&2
  exit 1
fi
if [ ! -x "$PREBUILT_NATIVE" ]; then
  echo "missing executable $PREBUILT_NATIVE" >&2
  exit 1
fi
if [ ! -x "$PREBUILT_PIPELINE" ]; then
  echo "missing executable $PREBUILT_PIPELINE" >&2
  exit 1
fi

printf '== checksum gate ==\n'
(cd bootstrap/prebuilt/linux-x86_64 && sha256sum -c SHA256SUMS)

printf '\n== self-host gate without Python ==\n'
"$PREBUILT_COMPILER" compiler.slop "$TMP/compiler_self.c"
$CC -O3 -std=gnu11 -ffast-math -flto -march=native -I. "$TMP/compiler_self.c" -o "$TMP/slop-compiler-self"
"$TMP/slop-compiler-self" compiler.slop "$TMP/compiler_self2.c"
diff -q "$TMP/compiler_self.c" "$TMP/compiler_self2.c"

printf '\n== portable C backend examples ==\n'
for src in easy_start.slop low_level_demo.slop; do
  base="$TMP/${src%.slop}"
  "$PREBUILT_COMPILER" "$src" "$base.c"
  $CC -O3 -std=gnu11 -I. "$base.c" -o "$base"
  "$base" > "$base.out"
  cat "$base.out"
done

grep -q "Hello Gugu" "$TMP/easy_start.out"
grep -q "Optional unsafe low-level mode" "$TMP/low_level_demo.out"
grep -q "1337" "$TMP/low_level_demo.out"

printf '\n== direct ELF native backend ==\n'
"$PREBUILT_NATIVE" native_backend_demo.slop "$TMP/native_demo" x86_64-linux-elf
chmod +x "$TMP/native_demo"
"$TMP/native_demo" > "$TMP/native_demo.out"
cat "$TMP/native_demo.out"
grep -q "Hello from Slop's direct native backend" "$TMP/native_demo.out"


printf '\n== SIR production pipeline ==\n'
"$PREBUILT_NATIVE" native_backend_demo.slop "$TMP/native_demo.sir" sir
"$PREBUILT_PIPELINE" "$TMP/native_demo.sir" "$TMP/native_demo_pipeline.c" c
$CC -O3 -std=gnu11 "$TMP/native_demo_pipeline.c" -o "$TMP/native_demo_pipeline_c"
"$TMP/native_demo_pipeline_c" > "$TMP/native_demo_pipeline_c.out"
"$PREBUILT_PIPELINE" "$TMP/native_demo.sir" "$TMP/native_demo_pipeline_elf" elf x86_64-linux-elf
"$TMP/native_demo_pipeline_elf" > "$TMP/native_demo_pipeline_elf.out"
grep -q "Hello from Slop's direct native backend" "$TMP/native_demo_pipeline_c.out"
grep -q "Hello from Slop's direct native backend" "$TMP/native_demo_pipeline_elf.out"

printf '\n== phase 4-7 smoke ==\n'
tools/phase4_7_smoke.sh

printf '\nPRODUCTION GATE PASSED for current supported surface.\n'
