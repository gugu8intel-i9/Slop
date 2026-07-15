#ifndef SLOP_LOWERING_H
#define SLOP_LOWERING_H

// Slop full-language lowering contract
// ------------------------------------
// This header is the first implementation-facing contract for lowering the
// whole Slop language into SIR. It deliberately separates frontend syntax from
// backend code generation so Slop can be easier to learn while still compiling
// through a high-performance, target-neutral IR.
//
// The functions here are small and inline so the compiler can use them without a
// new runtime dependency. The self-hosted compiler will progressively call these
// from its parser/codegen path; the native backend already consumes SIR.

#include "slop_ir.h"

typedef struct {
    SIRModule* module;
    SIRId current_function;
    SIRId current_block;
    SIRId current_arena_scope;
} SlopLowering;

static inline void slop_lowering_init(SlopLowering* l, SIRModule* module) {
    l->module = module;
    l->current_function = 0;
    l->current_block = 0;
    l->current_arena_scope = 0;
}

static inline SIRId slop_lower_function_begin(SlopLowering* l, const char* name) {
    SIRId name_id = sir_add_string(l->module, name, strlen(name));
    SIRId fn_id = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_FUNCTION_BEGIN);
    inst->dst = fn_id;
    inst->a = name_id;
    l->current_function = fn_id;
    return fn_id;
}

static inline void slop_lower_function_end(SlopLowering* l) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_FUNCTION_END);
    inst->a = l->current_function;
    l->current_function = 0;
}

static inline SIRId slop_lower_block(SlopLowering* l, const char* label) {
    SIRId label_id = sir_add_string(l->module, label, strlen(label));
    SIRId block_id = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_BLOCK);
    inst->dst = block_id;
    inst->a = label_id;
    l->current_block = block_id;
    return block_id;
}

static inline SIRId slop_lower_i64(SlopLowering* l, int64_t value) {
    return sir_emit_const_i64(l->module, value);
}

static inline SIRId slop_lower_string_literal(SlopLowering* l, const char* value) {
    SIRId sid = sir_add_string(l->module, value, strlen(value));
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_CONST_STRING);
    inst->type = SIR_TYPE_STRING;
    inst->dst = dst;
    inst->a = sid;
    return dst;
}

static inline SIRId slop_lower_let(SlopLowering* l, const char* name, SIRType type, SIRId init) {
    SIRId name_id = sir_add_string(l->module, name, strlen(name));
    return sir_emit_local_declare(l->module, name_id, type, init);
}

static inline void slop_lower_assign(SlopLowering* l, SIRId local, SIRId value) {
    sir_emit_local_set(l->module, local, value);
}

static inline SIRId slop_lower_add_i64(SlopLowering* l, SIRId a, SIRId b) {
    return sir_emit_binary(l->module, SIR_OP_ADD_I64, SIR_TYPE_I64, a, b);
}

static inline SIRId slop_lower_sub_i64(SlopLowering* l, SIRId a, SIRId b) {
    return sir_emit_binary(l->module, SIR_OP_SUB_I64, SIR_TYPE_I64, a, b);
}

static inline SIRId slop_lower_mul_i64(SlopLowering* l, SIRId a, SIRId b) {
    return sir_emit_binary(l->module, SIR_OP_MUL_I64, SIR_TYPE_I64, a, b);
}

static inline SIRId slop_lower_cmp_lt_i64(SlopLowering* l, SIRId a, SIRId b) {
    return sir_emit_binary(l->module, SIR_OP_CMP_LT_I64, SIR_TYPE_BOOL, a, b);
}

static inline void slop_lower_branch(SlopLowering* l, SIRId cond, SIRId then_block, SIRId else_block) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_BRANCH);
    inst->a = cond;
    inst->b = then_block;
    inst->c = else_block;
}

static inline void slop_lower_jump(SlopLowering* l, SIRId target_block) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_JUMP);
    inst->a = target_block;
}

static inline void slop_lower_return(SlopLowering* l, SIRId value) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_RETURN);
    inst->a = value;
}

static inline void slop_lower_print_i64(SlopLowering* l, SIRId value) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_PRINT_I64);
    inst->a = value;
}

static inline void slop_lower_print_string_value(SlopLowering* l, SIRId value) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_PRINT_STRING);
    inst->a = value;
}

static inline void slop_lower_bounds_check(SlopLowering* l, SIRId index, SIRId length) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_BOUNDS_CHECK);
    inst->a = index;
    inst->b = length;
}

static inline SIRId slop_lower_array_new(SlopLowering* l, SIRType elem_type) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_NEW);
    inst->type = SIR_TYPE_ARRAY;
    inst->dst = dst;
    inst->imm = elem_type;
    return dst;
}

static inline void slop_lower_array_push(SlopLowering* l, SIRId array, SIRId value) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_PUSH);
    inst->a = array;
    inst->b = value;
}

static inline SIRId slop_lower_array_get(SlopLowering* l, SIRId array, SIRId index, SIRType elem_type) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_GET);
    inst->type = elem_type;
    inst->dst = dst;
    inst->a = array;
    inst->b = index;
    return dst;
}

static inline SIRId slop_lower_field_get(SlopLowering* l, SIRId object, const char* field_name, SIRType field_type) {
    SIRId dst = sir_new_value(l->module);
    SIRId field_id = sir_add_string(l->module, field_name, strlen(field_name));
    SIRInst* inst = sir_emit(l->module, SIR_OP_FIELD_GET);
    inst->type = field_type;
    inst->dst = dst;
    inst->a = object;
    inst->b = field_id;
    return dst;
}

static inline void slop_lower_field_set(SlopLowering* l, SIRId object, const char* field_name, SIRId value) {
    SIRId field_id = sir_add_string(l->module, field_name, strlen(field_name));
    SIRInst* inst = sir_emit(l->module, SIR_OP_FIELD_SET);
    inst->a = object;
    inst->b = field_id;
    inst->c = value;
}

static inline SIRId slop_lower_arena_save(SlopLowering* l) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_ARENA_SAVE);
    inst->dst = dst;
    l->current_arena_scope = dst;
    return dst;
}

static inline void slop_lower_arena_restore(SlopLowering* l, SIRId scope) {
    SIRInst* inst = sir_emit(l->module, SIR_OP_ARENA_RESTORE);
    inst->a = scope;
}

#endif // SLOP_LOWERING_H
