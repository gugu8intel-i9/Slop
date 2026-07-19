#ifndef SLOP_PIPELINE_H
#define SLOP_PIPELINE_H

// Slop SIR-first production pipeline contract.
// --------------------------------------------
// This is the bridge that turns the repo from "separate compiler paths" into a
// single high-performance compiler architecture:
//
//   frontend -> SIR -> verify -> optimize -> backend
//
// It is intentionally tiny, deterministic, and allocation-light. Production
// compiler.slop can mirror this contract while C/native/WASM/object emitters use
// the same phase ordering.

#include "slop_ir.h"
#include "slop_ir_tools.h"
#include "slop_sir_optimizer.h"
#include "slop_sir_c_backend.h"
#include "slop_object_link.h"

typedef enum {
    SLOP_BACKEND_C,
    SLOP_BACKEND_ASM,
    SLOP_BACKEND_DIRECT_ELF,
    SLOP_BACKEND_OBJECT,
    SLOP_BACKEND_WASM
} SlopBackendKind;

typedef struct {
    SlopBackendKind backend;
    SlopTargetKind target;
    bool optimize;
    bool verify;
    bool emit_ir;
    bool no_libc;
} SlopPipelineOptions;

typedef struct {
    SIRVerifyResult verify_result;
    SIROptStats opt_stats;
    uint64_t input_fingerprint;
    uint64_t output_fingerprint;
} SlopPipelineResult;

static inline SlopPipelineOptions slop_pipeline_default(void) {
    SlopPipelineOptions o;
    o.backend = SLOP_BACKEND_C;
    o.target = SLOP_TARGET_X86_64_LINUX;
    o.optimize = true;
    o.verify = true;
    o.emit_ir = false;
    o.no_libc = false;
    return o;
}

static inline SlopPipelineResult slop_pipeline_prepare_sir(SIRModule* m, SlopPipelineOptions opt, FILE* diag) {
    SlopPipelineResult r;
    memset(&r, 0, sizeof(r));
    r.input_fingerprint = sir_fingerprint(m);
    if (opt.verify) {
        r.verify_result = sir_verify_module(m, diag);
        if (r.verify_result.errors) return r;
    }
    if (opt.optimize) {
        r.opt_stats = sir_opt_phase3_high_performance(m);
    }
    if (opt.verify) {
        r.verify_result = sir_verify_module(m, diag);
    }
    r.output_fingerprint = sir_fingerprint(m);
    return r;
}

static inline int slop_pipeline_emit(FILE* out, SIRModule* m, SlopPipelineOptions opt, FILE* diag) {
    SlopPipelineResult r = slop_pipeline_prepare_sir(m, opt, diag);
    if (r.verify_result.errors) return 1;
    if (opt.emit_ir) {
        sir_write_text(out, m);
        return 0;
    }
    switch (opt.backend) {
        case SLOP_BACKEND_C:
            return sir_emit_c_backend(out, m);
        case SLOP_BACKEND_DIRECT_ELF:
            return slop_emit_direct_executable(out, m, opt.target);
        case SLOP_BACKEND_ASM:
        case SLOP_BACKEND_OBJECT:
        case SLOP_BACKEND_WASM:
            fprintf(diag ? diag : stderr, "selected backend is declared but not fully emitted by slop_pipeline_emit yet\n");
            return 2;
    }
    return 2;
}

#endif // SLOP_PIPELINE_H
