#ifndef SLOP_SIR_OPTIMIZER_H
#define SLOP_SIR_OPTIMIZER_H

// Phase-2 SIR optimizer primitives.
// These are intentionally tiny, deterministic, and backend-independent. The
// novel Slop-specific idea is to keep safety/arena operations explicit while
// aggressively folding pure value code around them.

#include "slop_ir_tools.h"
#include <stdbool.h>

typedef struct {
    uint32_t constants_folded;
    uint32_t pure_nops_inserted;
} SIROptStats;

static inline bool sir_find_const_i64(const SIRModule* m, SIRId id, int64_t* out) {
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        if (inst->dst == id && inst->op == SIR_OP_CONST_I64) {
            *out = inst->imm;
            return true;
        }
    }
    return false;
}

static inline bool sir_fold_i64_op(SIROp op, int64_t a, int64_t b, int64_t* out) {
    switch (op) {
        case SIR_OP_ADD_I64: *out = a + b; return true;
        case SIR_OP_SUB_I64: *out = a - b; return true;
        case SIR_OP_MUL_I64: *out = a * b; return true;
        case SIR_OP_DIV_I64: if (b == 0) return false; *out = a / b; return true;
        case SIR_OP_MOD_I64: if (b == 0) return false; *out = a % b; return true;
        case SIR_OP_CMP_EQ: *out = (a == b); return true;
        case SIR_OP_CMP_NE: *out = (a != b); return true;
        case SIR_OP_CMP_LT_I64: *out = (a < b); return true;
        case SIR_OP_CMP_LE_I64: *out = (a <= b); return true;
        case SIR_OP_CMP_GT_I64: *out = (a > b); return true;
        case SIR_OP_CMP_GE_I64: *out = (a >= b); return true;
        default: return false;
    }
}

static inline SIROptStats sir_opt_fold_constants(SIRModule* m) {
    SIROptStats stats = {0, 0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        int64_t a = 0, b = 0, folded = 0;
        if (inst->dst && sir_find_const_i64(m, inst->a, &a) && sir_find_const_i64(m, inst->b, &b) && sir_fold_i64_op(inst->op, a, b, &folded)) {
            inst->op = SIR_OP_CONST_I64;
            inst->type = (inst->type == SIR_TYPE_BOOL) ? SIR_TYPE_BOOL : SIR_TYPE_I64;
            inst->a = 0;
            inst->b = 0;
            inst->c = 0;
            inst->imm = folded;
            stats.constants_folded++;
        }
    }
    return stats;
}

static inline bool sir_inst_is_pure_value(const SIRInst* inst) {
    if (inst->dst == 0) return false;
    if (sir_op_has_side_effect(inst->op)) return false;
    if (sir_op_is_terminator(inst->op)) return false;
    switch (inst->op) {
        case SIR_OP_FUNCTION_BEGIN:
        case SIR_OP_FUNCTION_END:
        case SIR_OP_BLOCK:
        case SIR_OP_LOCAL_DECLARE:
        case SIR_OP_PHI:
            return false;
        default:
            return true;
    }
}

static inline void sir_mark_if_value(bool* used, uint32_t cap, SIRId id) {
    if (id > 0 && id < cap) used[id] = true;
}

static inline SIROptStats sir_opt_dead_pure_to_nop(SIRModule* m) {
    SIROptStats stats = {0, 0};
    bool* used = (bool*)calloc(m->next_value ? m->next_value : 1, sizeof(bool));
    if (!used) return stats;

    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        // Side effects and terminators are roots. Pure instructions used by roots
        // survive through the fixed-point loop below.
        if (sir_op_has_side_effect(inst->op) || sir_op_is_terminator(inst->op) || inst->op == SIR_OP_LOCAL_SET || inst->op == SIR_OP_RETURN) {
            sir_mark_if_value(used, m->next_value, inst->a);
            sir_mark_if_value(used, m->next_value, inst->b);
            sir_mark_if_value(used, m->next_value, inst->c);
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (uint32_t i = m->inst_len; i > 0; i--) {
            const SIRInst* inst = &m->insts[i - 1];
            if (inst->dst && inst->dst < m->next_value && used[inst->dst]) {
                bool before_a = inst->a < m->next_value ? used[inst->a] : false;
                bool before_b = inst->b < m->next_value ? used[inst->b] : false;
                bool before_c = inst->c < m->next_value ? used[inst->c] : false;
                sir_mark_if_value(used, m->next_value, inst->a);
                sir_mark_if_value(used, m->next_value, inst->b);
                sir_mark_if_value(used, m->next_value, inst->c);
                if ((inst->a < m->next_value && used[inst->a] != before_a) ||
                    (inst->b < m->next_value && used[inst->b] != before_b) ||
                    (inst->c < m->next_value && used[inst->c] != before_c)) changed = true;
            }
        }
    }

    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (sir_inst_is_pure_value(inst) && inst->dst < m->next_value && !used[inst->dst]) {
            inst->op = SIR_OP_NOP;
            inst->type = SIR_TYPE_VOID;
            inst->dst = inst->a = inst->b = inst->c = 0;
            inst->imm = 0;
            stats.pure_nops_inserted++;
        }
    }

    free(used);
    return stats;
}

static inline SIROptStats sir_opt_phase2_fast_pass(SIRModule* m) {
    SIROptStats a = sir_opt_fold_constants(m);
    SIROptStats b = sir_opt_dead_pure_to_nop(m);
    a.constants_folded += b.constants_folded;
    a.pure_nops_inserted += b.pure_nops_inserted;
    return a;
}

#endif // SLOP_SIR_OPTIMIZER_H
