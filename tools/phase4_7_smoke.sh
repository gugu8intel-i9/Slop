#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CC=${CC:-gcc}
TMP=${TMPDIR:-/tmp}/slop_phase4_7_$$
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/phase4_7.c" <<'C'
#include "slop_phase4_7.h"
int main(void) {
    SIRModule m;
    sir_module_init(&m);
    SlopLowering l;
    slop_lowering_init(&l, &m);
    slop_lower_function_begin(&l, "main");
    slop_lower_block(&l, "entry");
    SIRId a = slop_lower_i64(&l, 40);
    SIRId b = slop_lower_i64(&l, 2);
    SIRId c = slop_lower_add_i64(&l, a, b);
    slop_lower_print_i64(&l, c);
    sir_emit_exit(&m, 0);
    SIROptStats opt = sir_opt_phase3_high_performance(&m);
    (void)opt;
    SlopTargetInfo target = slop_target_info(SLOP_TARGET_X86_64_LINUX_ELF);
    SlopRegAlloc ra = slop_build_liveness(&m);
    slop_linear_scan_allocate(&ra, target.int_reg_count);
    SlopStackFrame frame = slop_compute_stack_frame(&ra, &target);
    SlopABIType ty = slop_abi_type_for_sir(SIR_TYPE_STRING, SLOP_TARGET_X86_64_LINUX_ELF);
    if (target.pointer_size != 8 || ty.cls != SLOP_ABI_CLASS_STRING) return 2;
    if (frame.stack_size % 16 != 0) return 3;
    sir_write_text(stdout, &m);
    slop_regalloc_free(&ra);
    sir_module_free(&m);
    return 0;
}
C

$CC -O3 -std=gnu11 -I. "$TMP/phase4_7.c" -o "$TMP/phase4_7"
"$TMP/phase4_7" > "$TMP/phase4_7.sir"

gcc -O3 -std=gnu11 -I. slop_native_backend.c -o "$TMP/slop-native-backend"
"$TMP/slop-native-backend" native_backend_demo.slop "$TMP/native_demo" x86_64-linux-elf
chmod +x "$TMP/native_demo"
"$TMP/native_demo" > "$TMP/native_demo.out"

grep -q "Hello from Slop's direct native backend" "$TMP/native_demo.out"
grep -q "sir.module" "$TMP/phase4_7.sir"

echo "Phase 4-7 smoke test passed"
