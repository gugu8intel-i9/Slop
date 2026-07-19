#ifndef SLOP_LOWERING_H
#define SLOP_LOWERING_H

// Slop full-language lowering contract
// ------------------------------------
// Phase 2 makes Slop's syntax lower into SIR using tiny, inline helpers.
// Backends consume SIR; beginners see a small language; the compiler owns the
// performance complexity.

#include "slop_ir.h"
#include <stdbool.h>

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

static inline SIRId slop_lower_named_id(SlopLowering* l, const char* name) {
    return sir_add_string(l->module, name, strlen(name));
}

// ---------- Functions / blocks / control flow ----------
static inline SIRId slop_lower_function_begin(SlopLowering* l, const char* name) {
    SIRId name_id = slop_lower_named_id(l, name);
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
    SIRId label_id = slop_lower_named_id(l, label);
    SIRId block_id = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_BLOCK);
    inst->dst = block_id;
    inst->a = label_id;
    l->current_block = block_id;
    return block_id;
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

static inline SIRId slop_lower_call(SlopLowering* l, const char* name, SIRId args_tuple_or_first, uint32_t arg_count, SIRType ret_type) {
    SIRId dst = ret_type == SIR_TYPE_VOID ? 0 : sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_CALL);
    inst->type = ret_type;
    inst->dst = dst;
    inst->a = slop_lower_named_id(l, name);
    inst->b = args_tuple_or_first;
    inst->imm = arg_count;
    return dst;
}

// ---------- Literals / locals ----------
static inline SIRId slop_lower_i64(SlopLowering* l, int64_t value) { return sir_emit_const_i64(l->module, value); }

static inline SIRId slop_lower_f64_bits(SlopLowering* l, int64_t raw_bits) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_CONST_F64);
    inst->type = SIR_TYPE_F64;
    inst->dst = dst;
    inst->imm = raw_bits;
    return dst;
}

static inline SIRId slop_lower_bool(SlopLowering* l, bool value) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_CONST_BOOL);
    inst->type = SIR_TYPE_BOOL;
    inst->dst = dst;
    inst->imm = value ? 1 : 0;
    return dst;
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
    SIRId name_id = slop_lower_named_id(l, name);
    return sir_emit_local_declare(l->module, name_id, type, init);
}


static inline SIRId slop_lower_local_get(SlopLowering* l, SIRId local, SIRType type) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_LOCAL_GET);
    inst->type = type;
    inst->dst = dst;
    inst->a = local;
    return dst;
}

static inline void slop_lower_assign(SlopLowering* l, SIRId local, SIRId value) { sir_emit_local_set(l->module, local, value); }

// ---------- Arithmetic / logic ----------
static inline SIRId slop_lower_bin(SlopLowering* l, SIROp op, SIRType type, SIRId a, SIRId b) { return sir_emit_binary(l->module, op, type, a, b); }
static inline SIRId slop_lower_add_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_ADD_I64, SIR_TYPE_I64, a, b); }
static inline SIRId slop_lower_sub_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_SUB_I64, SIR_TYPE_I64, a, b); }
static inline SIRId slop_lower_mul_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_MUL_I64, SIR_TYPE_I64, a, b); }
static inline SIRId slop_lower_div_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_DIV_I64, SIR_TYPE_I64, a, b); }
static inline SIRId slop_lower_mod_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_MOD_I64, SIR_TYPE_I64, a, b); }
static inline SIRId slop_lower_add_f64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_ADD_F64, SIR_TYPE_F64, a, b); }
static inline SIRId slop_lower_sub_f64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_SUB_F64, SIR_TYPE_F64, a, b); }
static inline SIRId slop_lower_mul_f64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_MUL_F64, SIR_TYPE_F64, a, b); }
static inline SIRId slop_lower_div_f64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_DIV_F64, SIR_TYPE_F64, a, b); }
static inline SIRId slop_lower_and_bool(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_AND_BOOL, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_or_bool(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_OR_BOOL, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_not_bool(SlopLowering* l, SIRId a) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_NOT_BOOL);
    inst->type = SIR_TYPE_BOOL;
    inst->dst = dst;
    inst->a = a;
    return dst;
}
static inline SIRId slop_lower_cmp_eq(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_CMP_EQ, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_cmp_ne(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_CMP_NE, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_cmp_lt_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_CMP_LT_I64, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_cmp_le_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_CMP_LE_I64, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_cmp_gt_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_CMP_GT_I64, SIR_TYPE_BOOL, a, b); }
static inline SIRId slop_lower_cmp_ge_i64(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_CMP_GE_I64, SIR_TYPE_BOOL, a, b); }

// ---------- Strings ----------
static inline SIRId slop_lower_string_concat(SlopLowering* l, SIRId a, SIRId b) { return slop_lower_bin(l, SIR_OP_STRING_CONCAT, SIR_TYPE_STRING, a, b); }
static inline SIRId slop_lower_string_len(SlopLowering* l, SIRId s) {
    SIRId dst = sir_new_value(l->module);
    SIRInst* inst = sir_emit(l->module, SIR_OP_STRING_LEN);
    inst->type = SIR_TYPE_I64;
    inst->dst = dst;
    inst->a = s;
    return dst;
}
static inline SIRId slop_lower_char_at(SlopLowering* l, SIRId s, SIRId idx) { return slop_lower_bin(l, SIR_OP_CHAR_AT, SIR_TYPE_STRING, s, idx); }

// ---------- Arrays ----------
static inline void slop_lower_bounds_check(SlopLowering* l, SIRId index, SIRId length) { SIRInst* inst = sir_emit(l->module, SIR_OP_BOUNDS_CHECK); inst->a = index; inst->b = length; }
static inline SIRId slop_lower_array_new(SlopLowering* l, SIRType elem_type) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_NEW); inst->type = SIR_TYPE_ARRAY; inst->dst = dst; inst->imm = elem_type; return dst; }
static inline SIRId slop_lower_array_len(SlopLowering* l, SIRId array) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_LEN); inst->type = SIR_TYPE_I64; inst->dst = dst; inst->a = array; return dst; }
static inline void slop_lower_array_push(SlopLowering* l, SIRId array, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_PUSH); inst->a = array; inst->b = value; }
static inline SIRId slop_lower_array_get(SlopLowering* l, SIRId array, SIRId index, SIRType elem_type) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_GET); inst->type = elem_type; inst->dst = dst; inst->a = array; inst->b = index; return dst; }
static inline void slop_lower_array_set(SlopLowering* l, SIRId array, SIRId index, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_ARRAY_SET); inst->a = array; inst->b = index; inst->c = value; }

// ---------- Structs / fields / methods ----------
static inline SIRId slop_lower_struct_new(SlopLowering* l, const char* type_name, SIRId fields_tuple) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_STRUCT_NEW); inst->type = SIR_TYPE_STRUCT; inst->dst = dst; inst->a = slop_lower_named_id(l, type_name); inst->b = fields_tuple; return dst; }
static inline SIRId slop_lower_field_get(SlopLowering* l, SIRId object, const char* field_name, SIRType field_type) { SIRId dst = sir_new_value(l->module); SIRId field_id = slop_lower_named_id(l, field_name); SIRInst* inst = sir_emit(l->module, SIR_OP_FIELD_GET); inst->type = field_type; inst->dst = dst; inst->a = object; inst->b = field_id; return dst; }
static inline void slop_lower_field_set(SlopLowering* l, SIRId object, const char* field_name, SIRId value) { SIRId field_id = slop_lower_named_id(l, field_name); SIRInst* inst = sir_emit(l->module, SIR_OP_FIELD_SET); inst->a = object; inst->b = field_id; inst->c = value; }
static inline SIRId slop_lower_method_call(SlopLowering* l, const char* method_name, SIRId this_value, uint32_t extra_arg_count, SIRType ret_type) { return slop_lower_call(l, method_name, this_value, extra_arg_count + 1, ret_type); }

// ---------- High-level Slop constructs ----------
static inline void slop_lower_match_begin(SlopLowering* l, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_MATCH_BEGIN); inst->a = value; }
static inline void slop_lower_match_case(SlopLowering* l, SIRId case_value, SIRId block) { SIRInst* inst = sir_emit(l->module, SIR_OP_MATCH_CASE); inst->a = case_value; inst->b = block; }
static inline void slop_lower_match_end(SlopLowering* l) { (void)l; sir_emit(l->module, SIR_OP_MATCH_END); }
static inline SIRId slop_lower_list_comprehension(SlopLowering* l, SIRId source_array, SIRId mapper_function, SIRType elem_type) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_LIST_COMPREHENSION); inst->type = SIR_TYPE_ARRAY; inst->dst = dst; inst->a = source_array; inst->b = mapper_function; inst->imm = elem_type; return dst; }
static inline SIRId slop_lower_parallel_for(SlopLowering* l, SIRId start, SIRId end, SIRId body_function) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_PARALLEL_FOR); inst->type = SIR_TYPE_VOID; inst->dst = dst; inst->a = start; inst->b = end; inst->c = body_function; return dst; }
static inline SIRId slop_lower_tensor_op(SlopLowering* l, const char* op_name, SIRId a, SIRId b) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_TENSOR_OP); inst->type = SIR_TYPE_TENSOR; inst->dst = dst; inst->a = slop_lower_named_id(l, op_name); inst->b = a; inst->c = b; return dst; }
static inline void slop_lower_raw_emit(SlopLowering* l, const char* target, const char* code) { SIRInst* inst = sir_emit(l->module, SIR_OP_RAW_EMIT); inst->a = slop_lower_named_id(l, target); inst->b = sir_add_string(l->module, code, strlen(code)); }


// ---------- Printing / effects ----------
static inline void slop_lower_print_i64(SlopLowering* l, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_PRINT_I64); inst->a = value; }
static inline void slop_lower_print_f64(SlopLowering* l, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_PRINT_F64); inst->a = value; }
static inline void slop_lower_print_bool(SlopLowering* l, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_PRINT_BOOL); inst->a = value; }
static inline void slop_lower_print_string_value(SlopLowering* l, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_PRINT_STRING); inst->a = value; }
static inline SIRId slop_lower_read_file(SlopLowering* l, SIRId path) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_READ_FILE); inst->type = SIR_TYPE_STRING; inst->dst = dst; inst->a = path; return dst; }
static inline void slop_lower_write_file(SlopLowering* l, SIRId path, SIRId content) { SIRInst* inst = sir_emit(l->module, SIR_OP_WRITE_FILE); inst->a = path; inst->b = content; }

// ---------- Optional unsafe low-level lowering ----------
static inline SIRId slop_lower_unsafe_load(SlopLowering* l, SIRId addr, uint32_t bits) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_UNSAFE_LOAD); inst->type = SIR_TYPE_I64; inst->dst = dst; inst->a = addr; inst->imm = bits; return dst; }
static inline void slop_lower_unsafe_store(SlopLowering* l, SIRId addr, SIRId value, uint32_t bits) { SIRInst* inst = sir_emit(l->module, SIR_OP_UNSAFE_STORE); inst->a = addr; inst->b = value; inst->imm = bits; }
static inline SIRId slop_lower_mmio_read32(SlopLowering* l, SIRId addr) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_MMIO_READ); inst->type = SIR_TYPE_I64; inst->dst = dst; inst->a = addr; inst->imm = 32; return dst; }
static inline void slop_lower_mmio_write32(SlopLowering* l, SIRId addr, SIRId value) { SIRInst* inst = sir_emit(l->module, SIR_OP_MMIO_WRITE); inst->a = addr; inst->b = value; inst->imm = 32; }
static inline void slop_lower_cpu_fence(SlopLowering* l) { sir_emit(l->module, SIR_OP_CPU_FENCE); }
static inline void slop_lower_cpu_relax(SlopLowering* l) { sir_emit(l->module, SIR_OP_CPU_RELAX); }
static inline void slop_lower_cpu_prefetch(SlopLowering* l, SIRId addr) { SIRInst* inst = sir_emit(l->module, SIR_OP_CPU_PREFETCH); inst->a = addr; }
static inline SIRId slop_lower_cpu_cycles(SlopLowering* l) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_CPU_CYCLES); inst->type = SIR_TYPE_I64; inst->dst = dst; return dst; }
static inline void slop_lower_device_fence(SlopLowering* l) { sir_emit(l->module, SIR_OP_DEVICE_FENCE); }
static inline void slop_lower_gpu_fence(SlopLowering* l) { sir_emit(l->module, SIR_OP_GPU_FENCE); }
static inline void slop_lower_ram_copy(SlopLowering* l, SIRId dst, SIRId src, SIRId bytes) { SIRInst* inst = sir_emit(l->module, SIR_OP_RAM_COPY); inst->a = dst; inst->b = src; inst->c = bytes; }
static inline void slop_lower_ram_zero(SlopLowering* l, SIRId dst, SIRId bytes) { SIRInst* inst = sir_emit(l->module, SIR_OP_RAM_ZERO); inst->a = dst; inst->b = bytes; }

// ---------- SEAA arenas ----------
static inline SIRId slop_lower_arena_save(SlopLowering* l) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_ARENA_SAVE); inst->dst = dst; l->current_arena_scope = dst; return dst; }
static inline void slop_lower_arena_restore(SlopLowering* l, SIRId scope) { SIRInst* inst = sir_emit(l->module, SIR_OP_ARENA_RESTORE); inst->a = scope; }
static inline SIRId slop_lower_arena_alloc(SlopLowering* l, SIRId bytes) { SIRId dst = sir_new_value(l->module); SIRInst* inst = sir_emit(l->module, SIR_OP_ARENA_ALLOC); inst->type = SIR_TYPE_PTR; inst->dst = dst; inst->a = bytes; return dst; }

#endif // SLOP_LOWERING_H
