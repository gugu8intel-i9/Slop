#ifndef SLOP_IR_H
#define SLOP_IR_H

// Slop IR (SIR) MVP
// ------------------
// A compact, target-neutral intermediate representation designed to become the
// shared backend contract for Slop's C backend, native backends, object emitters,
// and future optimizers.
//
// Design goals:
// - Linear, cache-friendly instruction stream.
// - 32-bit ids for values/blocks/strings to keep instructions small.
// - Arena/region-friendly ownership model: modules are built, optimized, emitted,
//   then freed as a unit.
// - Backend neutral: no x86/ARM/RISC-V register names in the IR.
// - Optimization-ready: constants and effects are explicit, so DCE, folding,
//   inlining, bounds-check elimination, and target lowering can share one path.
//
// Current MVP op coverage is intentionally small and powers slop_native_backend.c:
// - const strings in the module string pool
// - print-string side-effect operations
// - exit operation
//
// The roadmap expands this into typed SSA with blocks, calls, memory, arrays,
// structs, SEAA arena operations, bounds guards, and ABI lowering.

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t SIRId;

typedef enum {
    SIR_TYPE_VOID = 0,
    SIR_TYPE_I64,
    SIR_TYPE_F64,
    SIR_TYPE_BOOL,
    SIR_TYPE_STRING,
    SIR_TYPE_PTR,
    SIR_TYPE_ARRAY,
    SIR_TYPE_STRUCT
} SIRType;

typedef enum {
    SIR_OP_NOP = 0,
    SIR_OP_CONST_I64,
    SIR_OP_CONST_F64,
    SIR_OP_CONST_STRING,
    SIR_OP_ADD_I64,
    SIR_OP_SUB_I64,
    SIR_OP_MUL_I64,
    SIR_OP_DIV_I64,
    SIR_OP_CMP_LT_I64,
    SIR_OP_BLOCK,
    SIR_OP_JUMP,
    SIR_OP_BRANCH,
    SIR_OP_CALL,
    SIR_OP_RETURN,
    SIR_OP_ARENA_SAVE,
    SIR_OP_ARENA_RESTORE,
    SIR_OP_BOUNDS_CHECK,
    SIR_OP_PRINT_STRING,
    SIR_OP_EXIT
} SIROp;

typedef struct {
    SIROp op;
    SIRType type;
    SIRId dst;
    SIRId a;
    SIRId b;
    SIRId c;
    int64_t imm;
} SIRInst;

typedef struct {
    char* data;
    uint32_t len;
} SIRString;

typedef struct {
    SIRInst* insts;
    uint32_t inst_len;
    uint32_t inst_cap;

    SIRString* strings;
    uint32_t string_len;
    uint32_t string_cap;

    SIRId next_value;
} SIRModule;

static inline void* sir_xrealloc(void* p, size_t n) {
    void* q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "Slop IR: out of memory allocating %zu bytes\n", n);
        exit(1);
    }
    return q;
}

static inline char* sir_xstrdup_len(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) {
        fprintf(stderr, "Slop IR: out of memory allocating string\n");
        exit(1);
    }
    memcpy(out, s, n);
    out[n] = 0;
    return out;
}

static inline void sir_module_init(SIRModule* m) {
    memset(m, 0, sizeof(*m));
    m->next_value = 1;
}

static inline void sir_module_free(SIRModule* m) {
    for (uint32_t i = 0; i < m->string_len; i++) {
        free(m->strings[i].data);
    }
    free(m->strings);
    free(m->insts);
    memset(m, 0, sizeof(*m));
}

static inline SIRId sir_new_value(SIRModule* m) {
    return m->next_value++;
}

static inline SIRId sir_add_string(SIRModule* m, const char* data, size_t len) {
    if (m->string_len == m->string_cap) {
        m->string_cap = m->string_cap ? m->string_cap * 2 : 16;
        m->strings = (SIRString*)sir_xrealloc(m->strings, (size_t)m->string_cap * sizeof(SIRString));
    }
    SIRId id = m->string_len;
    m->strings[m->string_len].data = sir_xstrdup_len(data, len);
    m->strings[m->string_len].len = (uint32_t)len;
    m->string_len++;
    return id;
}

static inline SIRInst* sir_emit(SIRModule* m, SIROp op) {
    if (m->inst_len == m->inst_cap) {
        m->inst_cap = m->inst_cap ? m->inst_cap * 2 : 32;
        m->insts = (SIRInst*)sir_xrealloc(m->insts, (size_t)m->inst_cap * sizeof(SIRInst));
    }
    SIRInst* inst = &m->insts[m->inst_len++];
    memset(inst, 0, sizeof(*inst));
    inst->op = op;
    return inst;
}

static inline void sir_emit_print_string(SIRModule* m, SIRId string_id) {
    SIRInst* inst = sir_emit(m, SIR_OP_PRINT_STRING);
    inst->type = SIR_TYPE_VOID;
    inst->a = string_id;
}

static inline void sir_emit_exit(SIRModule* m, int64_t code) {
    SIRInst* inst = sir_emit(m, SIR_OP_EXIT);
    inst->type = SIR_TYPE_VOID;
    inst->imm = code;
}

static inline const char* sir_op_name(SIROp op) {
    switch (op) {
        case SIR_OP_NOP: return "nop";
        case SIR_OP_CONST_I64: return "const.i64";
        case SIR_OP_CONST_F64: return "const.f64";
        case SIR_OP_CONST_STRING: return "const.string";
        case SIR_OP_ADD_I64: return "add.i64";
        case SIR_OP_SUB_I64: return "sub.i64";
        case SIR_OP_MUL_I64: return "mul.i64";
        case SIR_OP_DIV_I64: return "div.i64";
        case SIR_OP_CMP_LT_I64: return "cmp.lt.i64";
        case SIR_OP_BLOCK: return "block";
        case SIR_OP_JUMP: return "jump";
        case SIR_OP_BRANCH: return "branch";
        case SIR_OP_CALL: return "call";
        case SIR_OP_RETURN: return "return";
        case SIR_OP_ARENA_SAVE: return "arena.save";
        case SIR_OP_ARENA_RESTORE: return "arena.restore";
        case SIR_OP_BOUNDS_CHECK: return "bounds.check";
        case SIR_OP_PRINT_STRING: return "print.string";
        case SIR_OP_EXIT: return "exit";
    }
    return "unknown";
}

static inline void sir_dump(FILE* out, const SIRModule* m) {
    fprintf(out, "sir.module {\n");
    fprintf(out, "  strings %u {\n", m->string_len);
    for (uint32_t i = 0; i < m->string_len; i++) {
        fprintf(out, "    @s%u len=%u\n", i, m->strings[i].len);
    }
    fprintf(out, "  }\n");
    fprintf(out, "  insts %u {\n", m->inst_len);
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        fprintf(out, "    %04u %-16s dst=%u a=%u b=%u c=%u imm=%lld\n",
                i, sir_op_name(inst->op), inst->dst, inst->a, inst->b, inst->c,
                (long long)inst->imm);
    }
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

#endif // SLOP_IR_H
