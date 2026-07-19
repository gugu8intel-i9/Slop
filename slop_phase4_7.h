#ifndef SLOP_PHASE4_7_H
#define SLOP_PHASE4_7_H

// Aggregated Phase 4-7 native toolchain surface.

#include "slop_ir.h"
#include "slop_ir_tools.h"
#include "slop_sir_optimizer.h"
#include "slop_lowering.h"
#include "slop_native_codegen.h"
#include "slop_runtime_abi.h"
#include "slop_object_link.h"
#include "slop_sir_c_backend.h"

static inline void slop_print_phase4_7_summary(FILE* out) {
    fprintf(out, "Slop Phase 4-7 MVP toolchain surface:\n");
    fprintf(out, "  Phase 4: native target descriptors, liveness, linear-scan regalloc, stack frames\n");
    fprintf(out, "  Phase 5: object/link metadata plus direct ELF64 x86_64 executable emitter\n");
    fprintf(out, "  Phase 6: runtime ABI layouts, type classification, syscall ABI tables\n");
    fprintf(out, "  Phase 7: target tooling hooks, emit-ir/emit-asm/native targets, smoke tests\n");
}

#endif // SLOP_PHASE4_7_H
