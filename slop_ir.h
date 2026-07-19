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
    SIR_TYPE_STRUCT,
    SIR_TYPE_TENSOR
} SIRType;

typedef enum {
    SIR_OP_NOP = 0,

    // Constants / data
    SIR_OP_CONST_I64,
    SIR_OP_CONST_F64,
    SIR_OP_CONST_BOOL,
    SIR_OP_CONST_STRING,

    // Locals and SSA-ish value motion
    SIR_OP_LOCAL_DECLARE,
    SIR_OP_LOCAL_GET,
    SIR_OP_LOCAL_SET,
    SIR_OP_MOVE,
    SIR_OP_PHI,

    // Arithmetic and logic
    SIR_OP_ADD_I64,
    SIR_OP_SUB_I64,
    SIR_OP_MUL_I64,
    SIR_OP_DIV_I64,
    SIR_OP_MOD_I64,
    SIR_OP_ADD_F64,
    SIR_OP_SUB_F64,
    SIR_OP_MUL_F64,
    SIR_OP_DIV_F64,
    SIR_OP_AND_BOOL,
    SIR_OP_OR_BOOL,
    SIR_OP_NOT_BOOL,
    SIR_OP_CMP_EQ,
    SIR_OP_CMP_NE,
    SIR_OP_CMP_LT_I64,
    SIR_OP_CMP_LE_I64,
    SIR_OP_CMP_GT_I64,
    SIR_OP_CMP_GE_I64,

    // Control flow
    SIR_OP_FUNCTION_BEGIN,
    SIR_OP_FUNCTION_END,
    SIR_OP_BLOCK,
    SIR_OP_JUMP,
    SIR_OP_BRANCH,
    SIR_OP_CALL,
    SIR_OP_RETURN,

    // SEAA/runtime lowering
    SIR_OP_ARENA_SAVE,
    SIR_OP_ARENA_ALLOC,
    SIR_OP_ARENA_RESTORE,
    SIR_OP_BOUNDS_CHECK,

    // Strings and arrays
    SIR_OP_STRING_CONCAT,
    SIR_OP_STRING_LEN,
    SIR_OP_CHAR_AT,
    SIR_OP_ARRAY_NEW,
    SIR_OP_ARRAY_LEN,
    SIR_OP_ARRAY_GET,
    SIR_OP_ARRAY_SET,
    SIR_OP_ARRAY_PUSH,

    // Structs / fields
    SIR_OP_STRUCT_NEW,
    SIR_OP_FIELD_GET,
    SIR_OP_FIELD_SET,

    // Parallel/tensor hooks
    SIR_OP_PARALLEL_FOR,
    SIR_OP_TENSOR_OP,
    SIR_OP_GPU_KERNEL,

    // High-level lowering markers retained for tools/debug until lowered away
    SIR_OP_MATCH_BEGIN,
    SIR_OP_MATCH_CASE,
    SIR_OP_MATCH_END,
    SIR_OP_LIST_COMPREHENSION,
    SIR_OP_RAW_EMIT,

    // Explicit unsafe/hardware effects
    SIR_OP_UNSAFE_LOAD,
    SIR_OP_UNSAFE_STORE,
    SIR_OP_MMIO_READ,
    SIR_OP_MMIO_WRITE,
    SIR_OP_CPU_FENCE,
    SIR_OP_CPU_RELAX,
    SIR_OP_CPU_PREFETCH,
    SIR_OP_CPU_CYCLES,
    SIR_OP_DEVICE_FENCE,
    SIR_OP_GPU_FENCE,
    SIR_OP_RAM_COPY,
    SIR_OP_RAM_ZERO,

    // Effects
    SIR_OP_PRINT_I64,
    SIR_OP_PRINT_F64,
    SIR_OP_PRINT_BOOL,
    SIR_OP_PRINT_STRING,
    SIR_OP_READ_FILE,
    SIR_OP_WRITE_FILE,
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

static inline SIRId sir_emit_const_i64(SIRModule* m, int64_t value) {
    SIRId dst = sir_new_value(m);
    SIRInst* inst = sir_emit(m, SIR_OP_CONST_I64);
    inst->type = SIR_TYPE_I64;
    inst->dst = dst;
    inst->imm = value;
    return dst;
}

static inline SIRId sir_emit_binary(SIRModule* m, SIROp op, SIRType type, SIRId a, SIRId b) {
    SIRId dst = sir_new_value(m);
    SIRInst* inst = sir_emit(m, op);
    inst->type = type;
    inst->dst = dst;
    inst->a = a;
    inst->b = b;
    return dst;
}

static inline SIRId sir_emit_local_declare(SIRModule* m, SIRId name_string_id, SIRType type, SIRId init) {
    SIRId local = sir_new_value(m);
    SIRInst* inst = sir_emit(m, SIR_OP_LOCAL_DECLARE);
    inst->type = type;
    inst->dst = local;
    inst->a = name_string_id;
    inst->b = init;
    return local;
}

static inline void sir_emit_local_set(SIRModule* m, SIRId local, SIRId value) {
    SIRInst* inst = sir_emit(m, SIR_OP_LOCAL_SET);
    inst->type = SIR_TYPE_VOID;
    inst->a = local;
    inst->b = value;
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
        case SIR_OP_CONST_BOOL: return "const.bool";
        case SIR_OP_CONST_STRING: return "const.string";
        case SIR_OP_LOCAL_DECLARE: return "local.declare";
        case SIR_OP_LOCAL_GET: return "local.get";
        case SIR_OP_LOCAL_SET: return "local.set";
        case SIR_OP_MOVE: return "move";
        case SIR_OP_PHI: return "phi";
        case SIR_OP_ADD_I64: return "add.i64";
        case SIR_OP_SUB_I64: return "sub.i64";
        case SIR_OP_MUL_I64: return "mul.i64";
        case SIR_OP_DIV_I64: return "div.i64";
        case SIR_OP_MOD_I64: return "mod.i64";
        case SIR_OP_ADD_F64: return "add.f64";
        case SIR_OP_SUB_F64: return "sub.f64";
        case SIR_OP_MUL_F64: return "mul.f64";
        case SIR_OP_DIV_F64: return "div.f64";
        case SIR_OP_AND_BOOL: return "and.bool";
        case SIR_OP_OR_BOOL: return "or.bool";
        case SIR_OP_NOT_BOOL: return "not.bool";
        case SIR_OP_CMP_EQ: return "cmp.eq";
        case SIR_OP_CMP_NE: return "cmp.ne";
        case SIR_OP_CMP_LT_I64: return "cmp.lt.i64";
        case SIR_OP_CMP_LE_I64: return "cmp.le.i64";
        case SIR_OP_CMP_GT_I64: return "cmp.gt.i64";
        case SIR_OP_CMP_GE_I64: return "cmp.ge.i64";
        case SIR_OP_FUNCTION_BEGIN: return "function.begin";
        case SIR_OP_FUNCTION_END: return "function.end";
        case SIR_OP_BLOCK: return "block";
        case SIR_OP_JUMP: return "jump";
        case SIR_OP_BRANCH: return "branch";
        case SIR_OP_CALL: return "call";
        case SIR_OP_RETURN: return "return";
        case SIR_OP_ARENA_SAVE: return "arena.save";
        case SIR_OP_ARENA_ALLOC: return "arena.alloc";
        case SIR_OP_ARENA_RESTORE: return "arena.restore";
        case SIR_OP_BOUNDS_CHECK: return "bounds.check";
        case SIR_OP_STRING_CONCAT: return "string.concat";
        case SIR_OP_STRING_LEN: return "string.len";
        case SIR_OP_CHAR_AT: return "char.at";
        case SIR_OP_ARRAY_NEW: return "array.new";
        case SIR_OP_ARRAY_LEN: return "array.len";
        case SIR_OP_ARRAY_GET: return "array.get";
        case SIR_OP_ARRAY_SET: return "array.set";
        case SIR_OP_ARRAY_PUSH: return "array.push";
        case SIR_OP_STRUCT_NEW: return "struct.new";
        case SIR_OP_FIELD_GET: return "field.get";
        case SIR_OP_FIELD_SET: return "field.set";
        case SIR_OP_PARALLEL_FOR: return "parallel.for";
        case SIR_OP_TENSOR_OP: return "tensor.op";
        case SIR_OP_GPU_KERNEL: return "gpu.kernel";
        case SIR_OP_MATCH_BEGIN: return "match.begin";
        case SIR_OP_MATCH_CASE: return "match.case";
        case SIR_OP_MATCH_END: return "match.end";
        case SIR_OP_LIST_COMPREHENSION: return "list.comprehension";
        case SIR_OP_RAW_EMIT: return "raw.emit";
        case SIR_OP_UNSAFE_LOAD: return "unsafe.load";
        case SIR_OP_UNSAFE_STORE: return "unsafe.store";
        case SIR_OP_MMIO_READ: return "mmio.read";
        case SIR_OP_MMIO_WRITE: return "mmio.write";
        case SIR_OP_CPU_FENCE: return "cpu.fence";
        case SIR_OP_CPU_RELAX: return "cpu.relax";
        case SIR_OP_CPU_PREFETCH: return "cpu.prefetch";
        case SIR_OP_CPU_CYCLES: return "cpu.cycles";
        case SIR_OP_DEVICE_FENCE: return "device.fence";
        case SIR_OP_GPU_FENCE: return "gpu.fence";
        case SIR_OP_RAM_COPY: return "ram.copy";
        case SIR_OP_RAM_ZERO: return "ram.zero";
        case SIR_OP_PRINT_I64: return "print.i64";
        case SIR_OP_PRINT_F64: return "print.f64";
        case SIR_OP_PRINT_BOOL: return "print.bool";
        case SIR_OP_PRINT_STRING: return "print.string";
        case SIR_OP_READ_FILE: return "read.file";
        case SIR_OP_WRITE_FILE: return "write.file";
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
