#ifndef SLOP_SIR_OPTIMIZER_H
#define SLOP_SIR_OPTIMIZER_H

// Phase-3 SIR optimizer.
// ----------------------
// High-performance, backend-independent optimization for Slop IR. The design is
// deliberately simple and fast: dense linear scans, small value-id remapping,
// explicit side-effect roots, and Slop-specific safety operations kept visible.
//
// Novel Slop-specific angle:
// - SEAA arena ops remain first-class so lifetime compression can happen safely.
// - bounds.check remains explicit so redundant checks can be proven and removed.
// - pure value code is aggressively folded/CSE'd while effects stay anchored.
// - the same optimized SIR feeds C, native assembly, future object, and WASM backends.

#include "slop_ir_tools.h"
#include <stdbool.h>

typedef struct {
    uint32_t constants_folded;
    uint32_t algebraic_simplified;
    uint32_t cse_eliminated;
    uint32_t copies_propagated;
    uint32_t pure_nops_inserted;
    uint32_t nops_compacted;
    uint32_t bounds_checks_removed;
    uint32_t proven_bounds_removed;
    uint32_t branches_folded;
    uint32_t jumps_removed;
    uint32_t unreachable_removed;
    uint32_t strings_fused;
    uint32_t arena_scopes_compressed;
    uint32_t fixed_point_rounds;
} SIROptStats;

static inline void sir_opt_stats_add(SIROptStats* a, SIROptStats b) {
    a->constants_folded += b.constants_folded;
    a->algebraic_simplified += b.algebraic_simplified;
    a->cse_eliminated += b.cse_eliminated;
    a->copies_propagated += b.copies_propagated;
    a->pure_nops_inserted += b.pure_nops_inserted;
    a->nops_compacted += b.nops_compacted;
    a->bounds_checks_removed += b.bounds_checks_removed;
    a->proven_bounds_removed += b.proven_bounds_removed;
    a->branches_folded += b.branches_folded;
    a->jumps_removed += b.jumps_removed;
    a->unreachable_removed += b.unreachable_removed;
    a->strings_fused += b.strings_fused;
    a->arena_scopes_compressed += b.arena_scopes_compressed;
    a->fixed_point_rounds += b.fixed_point_rounds;
}

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

static inline bool sir_find_const_string_id(const SIRModule* m, SIRId value_id, SIRId* string_id) {
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        if (inst->dst == value_id && inst->op == SIR_OP_CONST_STRING && inst->a < m->string_len) {
            *string_id = inst->a;
            return true;
        }
    }
    return false;
}

static inline void sir_replace_value_from(SIRModule* m, uint32_t start, SIRId old_id, SIRId new_id) {
    if (old_id == 0 || old_id == new_id) return;
    for (uint32_t i = start; i < m->inst_len; i++) {
        if (m->insts[i].a == old_id) m->insts[i].a = new_id;
        if (m->insts[i].b == old_id) m->insts[i].b = new_id;
        if (m->insts[i].c == old_id) m->insts[i].c = new_id;
    }
}

static inline void sir_make_nop(SIRInst* inst) {
    inst->op = SIR_OP_NOP;
    inst->type = SIR_TYPE_VOID;
    inst->dst = inst->a = inst->b = inst->c = 0;
    inst->imm = 0;
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
        case SIR_OP_AND_BOOL: *out = (a && b); return true;
        case SIR_OP_OR_BOOL: *out = (a || b); return true;
        default: return false;
    }
}

static inline SIROptStats sir_opt_fold_constants_fast(SIRModule* m) {
    SIROptStats stats = {0};
    uint8_t* known = (uint8_t*)calloc(m->next_value ? m->next_value : 1, sizeof(uint8_t));
    int64_t* values = (int64_t*)calloc(m->next_value ? m->next_value : 1, sizeof(int64_t));
    if (!known || !values) {
        free(known);
        free(values);
        return stats;
    }

    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op == SIR_OP_CONST_I64 && inst->dst < m->next_value) {
            known[inst->dst] = 1;
            values[inst->dst] = inst->imm;
            continue;
        }
        int64_t folded = 0;
        if (inst->dst && inst->a < m->next_value && inst->b < m->next_value && known[inst->a] && known[inst->b] && sir_fold_i64_op(inst->op, values[inst->a], values[inst->b], &folded)) {
            inst->op = SIR_OP_CONST_I64;
            inst->type = (inst->type == SIR_TYPE_BOOL) ? SIR_TYPE_BOOL : SIR_TYPE_I64;
            inst->a = 0;
            inst->b = 0;
            inst->c = 0;
            inst->imm = folded;
            if (inst->dst < m->next_value) {
                known[inst->dst] = 1;
                values[inst->dst] = folded;
            }
            stats.constants_folded++;
        }
    }

    free(known);
    free(values);
    return stats;
}

static inline SIROptStats sir_opt_fold_constants(SIRModule* m) {
    SIROptStats stats = {0};
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

static inline SIROptStats sir_opt_algebraic_simplify(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (!inst->dst) continue;
        int64_t a = 0, b = 0;
        bool has_a = sir_find_const_i64(m, inst->a, &a);
        bool has_b = sir_find_const_i64(m, inst->b, &b);
        SIRId replacement = 0;
        bool make_zero = false;

        switch (inst->op) {
            case SIR_OP_ADD_I64:
                if (has_a && a == 0) replacement = inst->b;
                if (has_b && b == 0) replacement = inst->a;
                break;
            case SIR_OP_SUB_I64:
                if (has_b && b == 0) replacement = inst->a;
                break;
            case SIR_OP_MUL_I64:
                if (has_a && a == 1) replacement = inst->b;
                if (has_b && b == 1) replacement = inst->a;
                if ((has_a && a == 0) || (has_b && b == 0)) make_zero = true;
                break;
            case SIR_OP_DIV_I64:
                if (has_b && b == 1) replacement = inst->a;
                break;
            case SIR_OP_OR_BOOL:
                if (has_a && a == 0) replacement = inst->b;
                if (has_b && b == 0) replacement = inst->a;
                break;
            case SIR_OP_AND_BOOL:
                if (has_a && a == 1) replacement = inst->b;
                if (has_b && b == 1) replacement = inst->a;
                if ((has_a && a == 0) || (has_b && b == 0)) make_zero = true;
                break;
            default:
                break;
        }

        if (make_zero) {
            inst->op = SIR_OP_CONST_I64;
            inst->type = (inst->type == SIR_TYPE_BOOL) ? SIR_TYPE_BOOL : SIR_TYPE_I64;
            inst->a = inst->b = inst->c = 0;
            inst->imm = 0;
            stats.algebraic_simplified++;
        } else if (replacement) {
            sir_replace_value_from(m, i + 1, inst->dst, replacement);
            sir_make_nop(inst);
            stats.algebraic_simplified++;
        }
    }
    return stats;
}

static inline bool sir_op_is_cse_candidate(SIROp op) {
    switch (op) {
        case SIR_OP_CONST_I64: case SIR_OP_CONST_F64: case SIR_OP_CONST_BOOL: case SIR_OP_CONST_STRING:
        case SIR_OP_ADD_I64: case SIR_OP_SUB_I64: case SIR_OP_MUL_I64: case SIR_OP_DIV_I64: case SIR_OP_MOD_I64:
        case SIR_OP_ADD_F64: case SIR_OP_SUB_F64: case SIR_OP_MUL_F64: case SIR_OP_DIV_F64:
        case SIR_OP_AND_BOOL: case SIR_OP_OR_BOOL: case SIR_OP_NOT_BOOL:
        case SIR_OP_CMP_EQ: case SIR_OP_CMP_NE: case SIR_OP_CMP_LT_I64: case SIR_OP_CMP_LE_I64: case SIR_OP_CMP_GT_I64: case SIR_OP_CMP_GE_I64:
        case SIR_OP_STRING_LEN:
            return true;
        default:
            return false;
    }
}

static inline bool sir_op_commutative(SIROp op) {
    return op == SIR_OP_ADD_I64 || op == SIR_OP_MUL_I64 || op == SIR_OP_ADD_F64 || op == SIR_OP_MUL_F64 ||
           op == SIR_OP_AND_BOOL || op == SIR_OP_OR_BOOL || op == SIR_OP_CMP_EQ || op == SIR_OP_CMP_NE;
}

static inline bool sir_inst_equiv_cse(SIRInst a, SIRInst b) {
    if (a.op != b.op || a.type != b.type || a.imm != b.imm) return false;
    if (sir_op_commutative(a.op)) {
        bool same = (a.a == b.a && a.b == b.b);
        bool swapped = (a.a == b.b && a.b == b.a);
        return (same || swapped) && a.c == b.c;
    }
    return a.a == b.a && a.b == b.b && a.c == b.c;
}

static inline uint64_t sir_cse_hash_inst(SIRInst inst) {
    if (sir_op_commutative(inst.op) && inst.a > inst.b) {
        SIRId t = inst.a;
        inst.a = inst.b;
        inst.b = t;
    }
    uint64_t h = UINT64_C(1469598103934665603);
    h = sir_fnv1a_bytes(h, &inst.op, sizeof(inst.op));
    h = sir_fnv1a_bytes(h, &inst.type, sizeof(inst.type));
    h = sir_fnv1a_bytes(h, &inst.a, sizeof(inst.a));
    h = sir_fnv1a_bytes(h, &inst.b, sizeof(inst.b));
    h = sir_fnv1a_bytes(h, &inst.c, sizeof(inst.c));
    h = sir_fnv1a_bytes(h, &inst.imm, sizeof(inst.imm));
    return h ? h : 1;
}

typedef struct {
    uint64_t hash;
    uint32_t inst_index;
} SIRCSEntry;

static inline SIROptStats sir_opt_cse_hash(SIRModule* m) {
    SIROptStats stats = {0};
    uint32_t cap = 1;
    while (cap < (m->inst_len * 2 + 1)) cap <<= 1;
    SIRCSEntry* table = (SIRCSEntry*)calloc(cap, sizeof(SIRCSEntry));
    if (!table) return stats;

    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (!inst->dst || !sir_op_is_cse_candidate(inst->op)) continue;
        uint64_t h = sir_cse_hash_inst(*inst);
        uint32_t slot = (uint32_t)(h & (cap - 1));
        bool inserted = false;
        for (uint32_t probe = 0; probe < cap; probe++) {
            SIRCSEntry* e = &table[(slot + probe) & (cap - 1)];
            if (e->hash == 0) {
                e->hash = h;
                e->inst_index = i;
                inserted = true;
                break;
            }
            if (e->hash == h) {
                SIRInst* prev = &m->insts[e->inst_index];
                if (prev->dst && sir_inst_equiv_cse(*inst, *prev)) {
                    sir_replace_value_from(m, i + 1, inst->dst, prev->dst);
                    sir_make_nop(inst);
                    stats.cse_eliminated++;
                    inserted = true;
                    break;
                }
            }
        }
        (void)inserted;
    }
    free(table);
    return stats;
}

static inline SIROptStats sir_opt_cse(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (!inst->dst || !sir_op_is_cse_candidate(inst->op)) continue;
        for (uint32_t j = 0; j < i; j++) {
            SIRInst* prev = &m->insts[j];
            if (!prev->dst || !sir_op_is_cse_candidate(prev->op)) continue;
            if (sir_inst_equiv_cse(*inst, *prev)) {
                sir_replace_value_from(m, i + 1, inst->dst, prev->dst);
                sir_make_nop(inst);
                stats.cse_eliminated++;
                break;
            }
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_copy_propagate(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op == SIR_OP_MOVE && inst->dst && inst->a) {
            sir_replace_value_from(m, i + 1, inst->dst, inst->a);
            sir_make_nop(inst);
            stats.copies_propagated++;
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
    SIROptStats stats = {0};
    bool* used = (bool*)calloc(m->next_value ? m->next_value : 1, sizeof(bool));
    if (!used) return stats;

    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
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
            sir_make_nop(inst);
            stats.pure_nops_inserted++;
        }
    }

    free(used);
    return stats;
}

static inline SIROptStats sir_opt_fuse_string_literals(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op != SIR_OP_STRING_CONCAT || !inst->dst) continue;
        SIRId sa = 0, sb = 0;
        if (sir_find_const_string_id(m, inst->a, &sa) && sir_find_const_string_id(m, inst->b, &sb)) {
            uint32_t la = m->strings[sa].len;
            uint32_t lb = m->strings[sb].len;
            char* joined = (char*)malloc((size_t)la + lb + 1);
            if (!joined) continue;
            memcpy(joined, m->strings[sa].data, la);
            memcpy(joined + la, m->strings[sb].data, lb);
            joined[(size_t)la + lb] = 0;
            SIRId sid = sir_add_string(m, joined, (size_t)la + lb);
            free(joined);
            inst->op = SIR_OP_CONST_STRING;
            inst->type = SIR_TYPE_STRING;
            inst->a = sid;
            inst->b = inst->c = 0;
            inst->imm = 0;
            stats.strings_fused++;
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_fold_const_branches(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op != SIR_OP_BRANCH) continue;
        int64_t cond = 0;
        if (sir_find_const_i64(m, inst->a, &cond)) {
            inst->op = SIR_OP_JUMP;
            inst->a = cond ? inst->b : inst->c;
            inst->b = inst->c = 0;
            stats.branches_folded++;
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_remove_proven_bounds_checks(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op != SIR_OP_BOUNDS_CHECK) continue;
        int64_t idx = 0, len = 0;
        if (sir_find_const_i64(m, inst->a, &idx) && sir_find_const_i64(m, inst->b, &len)) {
            if (idx >= 0 && idx < len) {
                sir_make_nop(inst);
                stats.proven_bounds_removed++;
            }
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_remove_duplicate_bounds_checks(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op != SIR_OP_BOUNDS_CHECK) continue;
        for (uint32_t j = 0; j < i; j++) {
            SIRInst* prev = &m->insts[j];
            if (prev->op == SIR_OP_BOUNDS_CHECK && prev->a == inst->a && prev->b == inst->b) {
                sir_make_nop(inst);
                stats.bounds_checks_removed++;
                break;
            }
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_compress_empty_arena_scopes(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i < m->inst_len; i++) {
        if (m->insts[i].op != SIR_OP_ARENA_SAVE || !m->insts[i].dst) continue;
        bool saw_alloc_or_effect = false;
        for (uint32_t j = i + 1; j < m->inst_len; j++) {
            if (m->insts[j].op == SIR_OP_ARENA_ALLOC) saw_alloc_or_effect = true;
            if (sir_op_has_side_effect(m->insts[j].op) && m->insts[j].op != SIR_OP_ARENA_RESTORE) saw_alloc_or_effect = true;
            if (m->insts[j].op == SIR_OP_ARENA_RESTORE && m->insts[j].a == m->insts[i].dst) {
                if (!saw_alloc_or_effect) {
                    sir_make_nop(&m->insts[i]);
                    sir_make_nop(&m->insts[j]);
                    stats.arena_scopes_compressed++;
                }
                break;
            }
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_remove_jumps_to_next_block(SIRModule* m) {
    SIROptStats stats = {0};
    for (uint32_t i = 0; i + 1 < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op != SIR_OP_JUMP) continue;
        for (uint32_t j = i + 1; j < m->inst_len; j++) {
            if (m->insts[j].op == SIR_OP_NOP) continue;
            if (m->insts[j].op == SIR_OP_BLOCK && m->insts[j].dst == inst->a) {
                sir_make_nop(inst);
                stats.jumps_removed++;
            }
            break;
        }
    }
    return stats;
}

static inline SIROptStats sir_opt_remove_unreachable_pure_after_terminator(SIRModule* m) {
    SIROptStats stats = {0};
    bool unreachable = false;
    for (uint32_t i = 0; i < m->inst_len; i++) {
        SIRInst* inst = &m->insts[i];
        if (inst->op == SIR_OP_BLOCK || inst->op == SIR_OP_FUNCTION_BEGIN || inst->op == SIR_OP_FUNCTION_END) {
            unreachable = false;
            continue;
        }
        if (unreachable && sir_inst_is_pure_value(inst)) {
            sir_make_nop(inst);
            stats.unreachable_removed++;
            continue;
        }
        if (sir_op_is_terminator(inst->op)) unreachable = true;
    }
    return stats;
}

static inline SIROptStats sir_opt_compact_nops(SIRModule* m) {
    SIROptStats stats = {0};
    uint32_t w = 0;
    for (uint32_t r = 0; r < m->inst_len; r++) {
        if (m->insts[r].op == SIR_OP_NOP) {
            stats.nops_compacted++;
            continue;
        }
        if (w != r) m->insts[w] = m->insts[r];
        w++;
    }
    m->inst_len = w;
    return stats;
}

static inline bool sir_module_is_parallel_safe(const SIRModule* m) {
    for (uint32_t i = 0; i < m->inst_len; i++) {
        switch (m->insts[i].op) {
            case SIR_OP_WRITE_FILE:
            case SIR_OP_RAW_EMIT:
            case SIR_OP_CALL:
                return false;
            default:
                break;
        }
    }
    return true;
}

static inline SIROptStats sir_opt_phase3_high_performance(SIRModule* m) {
    SIROptStats total = {0};
    for (uint32_t round = 0; round < 4; round++) {
        SIROptStats before = total;
        before.fixed_point_rounds = 0;
        sir_opt_stats_add(&total, sir_opt_fold_constants_fast(m));
        sir_opt_stats_add(&total, sir_opt_algebraic_simplify(m));
        sir_opt_stats_add(&total, sir_opt_copy_propagate(m));
        sir_opt_stats_add(&total, sir_opt_fuse_string_literals(m));
        sir_opt_stats_add(&total, sir_opt_cse_hash(m));
        sir_opt_stats_add(&total, sir_opt_fold_const_branches(m));
        sir_opt_stats_add(&total, sir_opt_remove_proven_bounds_checks(m));
        sir_opt_stats_add(&total, sir_opt_remove_duplicate_bounds_checks(m));
        sir_opt_stats_add(&total, sir_opt_compress_empty_arena_scopes(m));
        sir_opt_stats_add(&total, sir_opt_remove_jumps_to_next_block(m));
        sir_opt_stats_add(&total, sir_opt_remove_unreachable_pure_after_terminator(m));
        sir_opt_stats_add(&total, sir_opt_dead_pure_to_nop(m));
        SIROptStats after = total;
        after.fixed_point_rounds = 0;
        total.fixed_point_rounds = round + 1;
        if (memcmp(&before, &after, sizeof(total)) == 0) break;
    }
    sir_opt_stats_add(&total, sir_opt_compact_nops(m));
    return total;
}

// Backward-compatible alias used by Phase-2 tests/tools.
static inline SIROptStats sir_opt_phase2_fast_pass(SIRModule* m) {
    return sir_opt_phase3_high_performance(m);
}

#endif // SLOP_SIR_OPTIMIZER_H
