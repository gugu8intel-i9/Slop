#ifndef SLOP_IR_TOOLS_H
#define SLOP_IR_TOOLS_H

// Phase-1 SIR tooling: verifier, effect classifier, textual writer, and stable
// fingerprints. These utilities are deliberately single-pass and allocation-light
// so every backend can run them cheaply before emitting machine code/C/WASM.

#include "slop_ir.h"
#include <stdbool.h>
#include <inttypes.h>

typedef struct {
    uint32_t errors;
    uint32_t warnings;
} SIRVerifyResult;

static inline bool sir_op_has_side_effect(SIROp op) {
    switch (op) {
        case SIR_OP_LOCAL_SET:
        case SIR_OP_CALL:
        case SIR_OP_RETURN:
        case SIR_OP_ARENA_ALLOC:
        case SIR_OP_ARENA_RESTORE:
        case SIR_OP_BOUNDS_CHECK:
        case SIR_OP_ARRAY_SET:
        case SIR_OP_ARRAY_PUSH:
        case SIR_OP_FIELD_SET:
        case SIR_OP_PARALLEL_FOR:
        case SIR_OP_TENSOR_OP:
        case SIR_OP_PRINT_I64:
        case SIR_OP_PRINT_F64:
        case SIR_OP_PRINT_BOOL:
        case SIR_OP_PRINT_STRING:
        case SIR_OP_READ_FILE:
        case SIR_OP_WRITE_FILE:
        case SIR_OP_EXIT:
            return true;
        default:
            return false;
    }
}

static inline bool sir_op_is_terminator(SIROp op) {
    return op == SIR_OP_JUMP || op == SIR_OP_BRANCH || op == SIR_OP_RETURN || op == SIR_OP_EXIT;
}

static inline bool sir_valid_value_id(const SIRModule* m, SIRId id) {
    return id == 0 || id < m->next_value;
}

static inline bool sir_valid_string_id(const SIRModule* m, SIRId id) {
    return id < m->string_len;
}

static inline void sir_verify_error(FILE* err, SIRVerifyResult* r, uint32_t i, const char* msg) {
    r->errors++;
    if (err) fprintf(err, "SIR verify error at inst %u: %s\n", i, msg);
}

static inline void sir_verify_warn(FILE* err, SIRVerifyResult* r, uint32_t i, const char* msg) {
    r->warnings++;
    if (err) fprintf(err, "SIR verify warning at inst %u: %s\n", i, msg);
}

static inline SIRVerifyResult sir_verify_module(const SIRModule* m, FILE* err) {
    SIRVerifyResult r = {0, 0};
    if (!m) {
        r.errors++;
        if (err) fprintf(err, "SIR verify error: null module\n");
        return r;
    }
    if (m->next_value == 0) {
        r.errors++;
        if (err) fprintf(err, "SIR verify error: next_value must never be zero\n");
    }

    uint32_t unterminated_effect_blocks = 0;
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        if (inst->dst && !sir_valid_value_id(m, inst->dst)) sir_verify_error(err, &r, i, "dst value id out of range");

        switch (inst->op) {
            case SIR_OP_CONST_STRING:
                if (!sir_valid_string_id(m, inst->a)) sir_verify_error(err, &r, i, "const.string references invalid string id");
                break;
            case SIR_OP_FUNCTION_BEGIN:
            case SIR_OP_BLOCK:
                if (!sir_valid_string_id(m, inst->a)) sir_verify_error(err, &r, i, "named IR entity references invalid string id");
                break;
            case SIR_OP_PRINT_STRING:
                // MVP native backend permits either a string-pool id or a lowered value id.
                if (!sir_valid_string_id(m, inst->a) && !sir_valid_value_id(m, inst->a)) {
                    sir_verify_error(err, &r, i, "print.string operand is neither a string id nor value id");
                }
                break;
            case SIR_OP_PRINT_I64:
            case SIR_OP_PRINT_F64:
            case SIR_OP_PRINT_BOOL:
            case SIR_OP_RETURN:
            case SIR_OP_LOCAL_SET:
            case SIR_OP_ARENA_RESTORE:
                if (!sir_valid_value_id(m, inst->a)) sir_verify_error(err, &r, i, "operand a value id out of range");
                if ((inst->op == SIR_OP_LOCAL_SET) && !sir_valid_value_id(m, inst->b)) sir_verify_error(err, &r, i, "operand b value id out of range");
                break;
            case SIR_OP_ADD_I64: case SIR_OP_SUB_I64: case SIR_OP_MUL_I64: case SIR_OP_DIV_I64: case SIR_OP_MOD_I64:
            case SIR_OP_ADD_F64: case SIR_OP_SUB_F64: case SIR_OP_MUL_F64: case SIR_OP_DIV_F64:
            case SIR_OP_AND_BOOL: case SIR_OP_OR_BOOL:
            case SIR_OP_CMP_EQ: case SIR_OP_CMP_NE: case SIR_OP_CMP_LT_I64: case SIR_OP_CMP_LE_I64: case SIR_OP_CMP_GT_I64: case SIR_OP_CMP_GE_I64:
            case SIR_OP_ARRAY_GET: case SIR_OP_ARRAY_SET: case SIR_OP_ARRAY_PUSH:
            case SIR_OP_FIELD_GET: case SIR_OP_FIELD_SET:
            case SIR_OP_BOUNDS_CHECK:
                if (!sir_valid_value_id(m, inst->a)) sir_verify_error(err, &r, i, "operand a value id out of range");
                if (!sir_valid_value_id(m, inst->b)) sir_verify_error(err, &r, i, "operand b value id out of range");
                break;
            case SIR_OP_BRANCH:
                if (!sir_valid_value_id(m, inst->a)) sir_verify_error(err, &r, i, "branch condition value id out of range");
                if (!sir_valid_value_id(m, inst->b) || !sir_valid_value_id(m, inst->c)) sir_verify_error(err, &r, i, "branch target block id out of range");
                break;
            default:
                break;
        }
        if (sir_op_has_side_effect(inst->op) && !sir_op_is_terminator(inst->op)) unterminated_effect_blocks++;
    }
    if (m->inst_len > 0 && !sir_op_is_terminator(m->insts[m->inst_len - 1].op)) {
        sir_verify_warn(err, &r, m->inst_len - 1, "module does not end in a terminator");
    }
    (void)unterminated_effect_blocks;
    return r;
}

static inline uint64_t sir_fnv1a_bytes(uint64_t h, const void* data, size_t len) {
    const unsigned char* p = (const unsigned char*)data;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= UINT64_C(1099511628211);
    }
    return h;
}

static inline uint64_t sir_fingerprint(const SIRModule* m) {
    uint64_t h = UINT64_C(1469598103934665603);
    h = sir_fnv1a_bytes(h, &m->next_value, sizeof(m->next_value));
    h = sir_fnv1a_bytes(h, &m->inst_len, sizeof(m->inst_len));
    h = sir_fnv1a_bytes(h, m->insts, (size_t)m->inst_len * sizeof(SIRInst));
    for (uint32_t i = 0; i < m->string_len; i++) {
        h = sir_fnv1a_bytes(h, &m->strings[i].len, sizeof(m->strings[i].len));
        h = sir_fnv1a_bytes(h, m->strings[i].data, m->strings[i].len);
    }
    return h;
}

static inline void sir_write_text_escaped(FILE* out, const char* s, uint32_t len) {
    fputc('"', out);
    for (uint32_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\') fputs("\\\\", out);
        else if (c == '"') fputs("\\\"", out);
        else if (c == '\n') fputs("\\n", out);
        else if (c == '\t') fputs("\\t", out);
        else if (c == '\r') fputs("\\r", out);
        else if (c < 32 || c >= 127) fprintf(out, "\\x%02x", c);
        else fputc(c, out);
    }
    fputc('"', out);
}

static inline void sir_write_text(FILE* out, const SIRModule* m) {
    fprintf(out, "sir.module fingerprint=%016" PRIx64 " next=%u {\n", sir_fingerprint(m), m->next_value);
    fprintf(out, "  strings %u {\n", m->string_len);
    for (uint32_t i = 0; i < m->string_len; i++) {
        fprintf(out, "    @s%u len=%u data=", i, m->strings[i].len);
        sir_write_text_escaped(out, m->strings[i].data, m->strings[i].len);
        fputc('\n', out);
    }
    fprintf(out, "  }\n");
    fprintf(out, "  insts %u {\n", m->inst_len);
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        fprintf(out, "    %04u %-16s type=%u dst=%u a=%u b=%u c=%u imm=%lld\n",
                i, sir_op_name(inst->op), (unsigned)inst->type, inst->dst, inst->a, inst->b, inst->c,
                (long long)inst->imm);
    }
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}


static inline bool sir_op_from_name(const char* name, SIROp* out) {
    if (strcmp(name, "nop") == 0) { *out = SIR_OP_NOP; return true; }
    if (strcmp(name, "const.i64") == 0) { *out = SIR_OP_CONST_I64; return true; }
    if (strcmp(name, "const.f64") == 0) { *out = SIR_OP_CONST_F64; return true; }
    if (strcmp(name, "const.bool") == 0) { *out = SIR_OP_CONST_BOOL; return true; }
    if (strcmp(name, "const.string") == 0) { *out = SIR_OP_CONST_STRING; return true; }
    if (strcmp(name, "local.declare") == 0) { *out = SIR_OP_LOCAL_DECLARE; return true; }
    if (strcmp(name, "local.get") == 0) { *out = SIR_OP_LOCAL_GET; return true; }
    if (strcmp(name, "local.set") == 0) { *out = SIR_OP_LOCAL_SET; return true; }
    if (strcmp(name, "move") == 0) { *out = SIR_OP_MOVE; return true; }
    if (strcmp(name, "phi") == 0) { *out = SIR_OP_PHI; return true; }
    if (strcmp(name, "add.i64") == 0) { *out = SIR_OP_ADD_I64; return true; }
    if (strcmp(name, "sub.i64") == 0) { *out = SIR_OP_SUB_I64; return true; }
    if (strcmp(name, "mul.i64") == 0) { *out = SIR_OP_MUL_I64; return true; }
    if (strcmp(name, "div.i64") == 0) { *out = SIR_OP_DIV_I64; return true; }
    if (strcmp(name, "mod.i64") == 0) { *out = SIR_OP_MOD_I64; return true; }
    if (strcmp(name, "add.f64") == 0) { *out = SIR_OP_ADD_F64; return true; }
    if (strcmp(name, "sub.f64") == 0) { *out = SIR_OP_SUB_F64; return true; }
    if (strcmp(name, "mul.f64") == 0) { *out = SIR_OP_MUL_F64; return true; }
    if (strcmp(name, "div.f64") == 0) { *out = SIR_OP_DIV_F64; return true; }
    if (strcmp(name, "and.bool") == 0) { *out = SIR_OP_AND_BOOL; return true; }
    if (strcmp(name, "or.bool") == 0) { *out = SIR_OP_OR_BOOL; return true; }
    if (strcmp(name, "not.bool") == 0) { *out = SIR_OP_NOT_BOOL; return true; }
    if (strcmp(name, "cmp.eq") == 0) { *out = SIR_OP_CMP_EQ; return true; }
    if (strcmp(name, "cmp.ne") == 0) { *out = SIR_OP_CMP_NE; return true; }
    if (strcmp(name, "cmp.lt.i64") == 0) { *out = SIR_OP_CMP_LT_I64; return true; }
    if (strcmp(name, "cmp.le.i64") == 0) { *out = SIR_OP_CMP_LE_I64; return true; }
    if (strcmp(name, "cmp.gt.i64") == 0) { *out = SIR_OP_CMP_GT_I64; return true; }
    if (strcmp(name, "cmp.ge.i64") == 0) { *out = SIR_OP_CMP_GE_I64; return true; }
    if (strcmp(name, "function.begin") == 0) { *out = SIR_OP_FUNCTION_BEGIN; return true; }
    if (strcmp(name, "function.end") == 0) { *out = SIR_OP_FUNCTION_END; return true; }
    if (strcmp(name, "block") == 0) { *out = SIR_OP_BLOCK; return true; }
    if (strcmp(name, "jump") == 0) { *out = SIR_OP_JUMP; return true; }
    if (strcmp(name, "branch") == 0) { *out = SIR_OP_BRANCH; return true; }
    if (strcmp(name, "call") == 0) { *out = SIR_OP_CALL; return true; }
    if (strcmp(name, "return") == 0) { *out = SIR_OP_RETURN; return true; }
    if (strcmp(name, "arena.save") == 0) { *out = SIR_OP_ARENA_SAVE; return true; }
    if (strcmp(name, "arena.alloc") == 0) { *out = SIR_OP_ARENA_ALLOC; return true; }
    if (strcmp(name, "arena.restore") == 0) { *out = SIR_OP_ARENA_RESTORE; return true; }
    if (strcmp(name, "bounds.check") == 0) { *out = SIR_OP_BOUNDS_CHECK; return true; }
    if (strcmp(name, "string.concat") == 0) { *out = SIR_OP_STRING_CONCAT; return true; }
    if (strcmp(name, "string.len") == 0) { *out = SIR_OP_STRING_LEN; return true; }
    if (strcmp(name, "char.at") == 0) { *out = SIR_OP_CHAR_AT; return true; }
    if (strcmp(name, "array.new") == 0) { *out = SIR_OP_ARRAY_NEW; return true; }
    if (strcmp(name, "array.len") == 0) { *out = SIR_OP_ARRAY_LEN; return true; }
    if (strcmp(name, "array.get") == 0) { *out = SIR_OP_ARRAY_GET; return true; }
    if (strcmp(name, "array.set") == 0) { *out = SIR_OP_ARRAY_SET; return true; }
    if (strcmp(name, "array.push") == 0) { *out = SIR_OP_ARRAY_PUSH; return true; }
    if (strcmp(name, "struct.new") == 0) { *out = SIR_OP_STRUCT_NEW; return true; }
    if (strcmp(name, "field.get") == 0) { *out = SIR_OP_FIELD_GET; return true; }
    if (strcmp(name, "field.set") == 0) { *out = SIR_OP_FIELD_SET; return true; }
    if (strcmp(name, "parallel.for") == 0) { *out = SIR_OP_PARALLEL_FOR; return true; }
    if (strcmp(name, "tensor.op") == 0) { *out = SIR_OP_TENSOR_OP; return true; }
    if (strcmp(name, "print.i64") == 0) { *out = SIR_OP_PRINT_I64; return true; }
    if (strcmp(name, "print.f64") == 0) { *out = SIR_OP_PRINT_F64; return true; }
    if (strcmp(name, "print.bool") == 0) { *out = SIR_OP_PRINT_BOOL; return true; }
    if (strcmp(name, "print.string") == 0) { *out = SIR_OP_PRINT_STRING; return true; }
    if (strcmp(name, "read.file") == 0) { *out = SIR_OP_READ_FILE; return true; }
    if (strcmp(name, "write.file") == 0) { *out = SIR_OP_WRITE_FILE; return true; }
    if (strcmp(name, "exit") == 0) { *out = SIR_OP_EXIT; return true; }
    return false;
}

static inline char* sir_unescape_quoted(const char* q, uint32_t* out_len) {
    if (!q || *q != '"') return NULL;
    q++;
    size_t cap = strlen(q) + 1;
    char* out = (char*)malloc(cap);
    if (!out) return NULL;
    uint32_t n = 0;
    while (*q && *q != '"') {
        if (*q == '\\') {
            q++;
            if (*q == 'n') out[n++] = '\n';
            else if (*q == 't') out[n++] = '\t';
            else if (*q == 'r') out[n++] = '\r';
            else if (*q == '\\') out[n++] = '\\';
            else if (*q == '"') out[n++] = '"';
            else out[n++] = *q;
            if (*q) q++;
        } else {
            out[n++] = *q++;
        }
    }
    out[n] = 0;
    *out_len = n;
    return out;
}
static inline bool sir_load_text(FILE* in, SIRModule* out_module, FILE* err) {
    sir_module_init(out_module);
    char* line = NULL;
    size_t cap = 0;
    while (getline(&line, &cap, in) != -1) {
        unsigned parsed_next = 0;
        if (sscanf(line, " sir.module fingerprint=%*16x next=%u", &parsed_next) == 1 ||
            sscanf(line, "sir.module fingerprint=%*16x next=%u", &parsed_next) == 1) {
            if (parsed_next > 0) out_module->next_value = parsed_next;
            continue;
        }
        char* data = strstr(line, "data=");
        if (data) {
            data += 5;
            uint32_t len = 0;
            char* decoded = sir_unescape_quoted(data, &len);
            if (!decoded) { if (err) fprintf(err, "SIR load: bad string line: %s", line); free(line); return false; }
            sir_add_string(out_module, decoded, len);
            free(decoded);
            continue;
        }
        unsigned idx = 0, type = 0, dst = 0, a = 0, b = 0, c = 0;
        long long imm = 0;
        char opname[64] = {0};
        if (sscanf(line, " %u %63s type=%u dst=%u a=%u b=%u c=%u imm=%lld", &idx, opname, &type, &dst, &a, &b, &c, &imm) == 8) {
            (void)idx;
            SIROp op;
            if (!sir_op_from_name(opname, &op)) { if (err) fprintf(err, "SIR load: unknown op %s\n", opname); free(line); return false; }
            SIRInst* inst = sir_emit(out_module, op);
            inst->type = (SIRType)type;
            inst->dst = (SIRId)dst;
            inst->a = (SIRId)a;
            inst->b = (SIRId)b;
            inst->c = (SIRId)c;
            inst->imm = (int64_t)imm;
        }
    }
    free(line);
    SIRVerifyResult vr = sir_verify_module(out_module, err);
    return vr.errors == 0;
}

#endif // SLOP_IR_TOOLS_H
