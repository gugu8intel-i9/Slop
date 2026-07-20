#ifndef SLOP_SIR_C_BACKEND_H
#define SLOP_SIR_C_BACKEND_H

// SIR -> C backend MVP. This is the Phase-1 proof that the portable C backend
// can consume SIR instead of each backend owning its own frontend lowering.
// It intentionally supports the side-effect subset used by the current SIR MVP
// plus integer constants/prints for lowering tests.

#include "slop_ir.h"
#include "slop_ir_tools.h"

static inline const char* sir_c_type_name(SIRType t) {
    switch (t) {
        case SIR_TYPE_BOOL: return "int64_t";
        case SIR_TYPE_I64: return "int64_t";
        case SIR_TYPE_F64: return "double";
        case SIR_TYPE_STRING: return "const char*";
        case SIR_TYPE_ARRAY: return "SirArrayI64";
        case SIR_TYPE_STRUCT: return "SirStruct";
        default: return "int64_t";
    }
}

static inline bool sir_c_id_is_local(const SIRModule* m, SIRId id) {
    for (uint32_t i = 0; i < m->inst_len; i++) {
        if (m->insts[i].op == SIR_OP_LOCAL_DECLARE && m->insts[i].dst == id) return true;
    }
    return false;
}

static inline void sir_c_ref(FILE* out, const SIRModule* m, SIRId id) {
    fputc(sir_c_id_is_local(m, id) ? 'l' : 'v', out);
    fprintf(out, "%u", id);
}

static inline SIRType sir_c_value_type(const SIRModule* m, SIRId id) {
    for (uint32_t i = 0; i < m->inst_len; i++) {
        if (m->insts[i].dst == id) return m->insts[i].type;
    }
    return SIR_TYPE_I64;
}

static inline void sir_c_escape(FILE* out, const char* s, uint32_t len) {
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
}

static inline void sir_c_emit_function_signature(FILE* out, const SIRModule* m, uint32_t fn_index) {
    const SIRInst* fn = &m->insts[fn_index];
    const char* name = fn->a < m->string_len ? m->strings[fn->a].data : "unknown";
    if (strcmp(name, "main") == 0) {
        fprintf(out, "int main(void)");
        return;
    }
    fprintf(out, "int64_t fn_%s(", name);
    bool first = true;
    for (uint32_t j = fn_index + 1; j < m->inst_len && m->insts[j].op == SIR_OP_LOCAL_DECLARE && m->insts[j].b == 0; j++) {
        if (!first) fprintf(out, ", ");
        fprintf(out, "%s l%u", sir_c_type_name(m->insts[j].type), m->insts[j].dst);
        first = false;
    }
    fprintf(out, ")");
}

static inline int sir_emit_c_backend(FILE* out, const SIRModule* m) {
    SIRVerifyResult vr = sir_verify_module(m, stderr);
    if (vr.errors) return 1;

    fprintf(out, "// Generated from Slop SIR by the SIR C backend MVP\n");
    fprintf(out, "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");
    fprintf(out, "typedef struct { int64_t* data; uint64_t length; uint64_t capacity; } SirArrayI64;\n");
    fprintf(out, "typedef struct { const char* names[32]; int64_t ivals[32]; const char* svals[32]; uint8_t types[32]; uint64_t length; } SirStruct;\n");
    fprintf(out, "static uint64_t sir_struct_slot(SirStruct* s, const char* name) { for (uint64_t i=0;i<s->length;i++) if (strcmp(s->names[i], name)==0) return i; if (s->length >= 32) exit(1); s->names[s->length]=name; return s->length++; }\n");
    fprintf(out, "static void sir_struct_set_i64(SirStruct* s, const char* name, int64_t v) { uint64_t i=sir_struct_slot(s,name); s->ivals[i]=v; s->types[i]=1; }\n");
    fprintf(out, "static void sir_struct_set_str(SirStruct* s, const char* name, const char* v) { uint64_t i=sir_struct_slot(s,name); s->svals[i]=v; s->types[i]=4; }\n");
    fprintf(out, "static int64_t sir_struct_get_i64(SirStruct* s, const char* name) { for (uint64_t i=0;i<s->length;i++) if (strcmp(s->names[i], name)==0) return s->ivals[i]; fprintf(stderr, \"SIR struct field not found: %%s\\n\", name); exit(1); }\n");
    fprintf(out, "static const char* sir_struct_get_str(SirStruct* s, const char* name) { for (uint64_t i=0;i<s->length;i++) if (strcmp(s->names[i], name)==0) return s->svals[i] ? s->svals[i] : \"\"; fprintf(stderr, \"SIR struct field not found: %%s\\n\", name); exit(1); }\n");
    fprintf(out, "static void sir_array_i64_push(SirArrayI64* a, int64_t v) { if (a->length == a->capacity) { a->capacity = a->capacity ? a->capacity * 2 : 8; a->data = (int64_t*)realloc(a->data, a->capacity * sizeof(int64_t)); if (!a->data) exit(1); } a->data[a->length++] = v; }\n");
    fprintf(out, "static int64_t sir_array_i64_get(SirArrayI64* a, int64_t i) { if (i < 0 || (uint64_t)i >= a->length) { fprintf(stderr, \"SIR array bounds error: index %%lld length %%llu\\n\", (long long)i, (unsigned long long)a->length); exit(1); } return a->data[i]; }\n");
    fprintf(out, "static char* slop_sir_c_concat(const char* a, const char* b) { size_t la=strlen(a), lb=strlen(b); char* o=(char*)malloc(la+lb+1); memcpy(o,a,la); memcpy(o+la,b,lb); o[la+lb]=0; return o; }\n\n");
    bool has_functions = false;
    for (uint32_t i = 0; i < m->inst_len; i++) if (m->insts[i].op == SIR_OP_FUNCTION_BEGIN) has_functions = true;
    if (!has_functions) fprintf(out, "int main(void) {\n");
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        switch (inst->op) {
            case SIR_OP_FUNCTION_BEGIN:
                sir_c_emit_function_signature(out, m, i);
                fprintf(out, " {\n");
                break;
            case SIR_OP_FUNCTION_END:
                fprintf(out, "}\n\n");
                break;
            case SIR_OP_CALL:
                if (inst->a < m->string_len) {
                    fprintf(out, "    int64_t v%u = fn_%s(", inst->dst, m->strings[inst->a].data);
                    if (inst->imm > 0) fprintf(out, "v%u", inst->b);
                    if (inst->imm > 1) fprintf(out, ", v%u", inst->c);
                    fprintf(out, ");\n");
                }
                break;
            case SIR_OP_CONST_STRING:
                if (inst->a < m->string_len) {
                    fprintf(out, "    const char* v%u = \"", inst->dst);
                    sir_c_escape(out, m->strings[inst->a].data, m->strings[inst->a].len);
                    fprintf(out, "\";\n");
                }
                break;
            case SIR_OP_STRING_CONCAT:
                fprintf(out, "    char* v%u = slop_sir_c_concat(v%u, v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CONST_I64:
                fprintf(out, "    int64_t v%u = %lld;\n", inst->dst, (long long)inst->imm);
                break;
            case SIR_OP_ADD_I64:
                fprintf(out, "    int64_t v%u = v%u + v%u;\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_SUB_I64:
                fprintf(out, "    int64_t v%u = v%u - v%u;\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_MUL_I64:
                fprintf(out, "    int64_t v%u = v%u * v%u;\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_DIV_I64:
                fprintf(out, "    int64_t v%u = v%u / v%u;\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_MOD_I64:
                fprintf(out, "    int64_t v%u = v%u %% v%u;\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CMP_EQ:
                fprintf(out, "    int64_t v%u = (v%u == v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CMP_NE:
                fprintf(out, "    int64_t v%u = (v%u != v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CMP_LT_I64:
                fprintf(out, "    int64_t v%u = (v%u < v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CMP_LE_I64:
                fprintf(out, "    int64_t v%u = (v%u <= v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CMP_GT_I64:
                fprintf(out, "    int64_t v%u = (v%u > v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_CMP_GE_I64:
                fprintf(out, "    int64_t v%u = (v%u >= v%u);\n", inst->dst, inst->a, inst->b);
                break;
            case SIR_OP_LOCAL_DECLARE:
                if (inst->b != 0) fprintf(out, "    %s l%u = v%u;\n", sir_c_type_name(inst->type), inst->dst, inst->b);
                break;
            case SIR_OP_LOCAL_GET:
                fprintf(out, "    %s v%u = l%u;\n", sir_c_type_name(inst->type), inst->dst, inst->a);
                break;
            case SIR_OP_LOCAL_SET:
                fprintf(out, "    l%u = v%u;\n", inst->a, inst->b);
                break;
            case SIR_OP_BLOCK:
                fprintf(out, "L%u:;\n", inst->dst);
                break;
            case SIR_OP_JUMP:
                fprintf(out, "    goto L%u;\n", inst->a);
                break;
            case SIR_OP_BRANCH:
                fprintf(out, "    if (v%u) goto L%u; else goto L%u;\n", inst->a, inst->b, inst->c);
                break;
            case SIR_OP_RETURN:
                if (inst->a) fprintf(out, "    return (int)v%u;\n", inst->a); else fprintf(out, "    return 0;\n");
                break;
            case SIR_OP_ARRAY_NEW:
                fprintf(out, "    SirArrayI64 v%u = {0};\n", inst->dst);
                break;
            case SIR_OP_ARRAY_PUSH:
                fprintf(out, "    sir_array_i64_push(&"); sir_c_ref(out, m, inst->a); fprintf(out, ", v%u);\n", inst->b);
                break;
            case SIR_OP_ARRAY_LEN:
                fprintf(out, "    int64_t v%u = (int64_t)", inst->dst); sir_c_ref(out, m, inst->a); fprintf(out, ".length;\n");
                break;
            case SIR_OP_ARRAY_GET:
                fprintf(out, "    int64_t v%u = sir_array_i64_get(&", inst->dst); sir_c_ref(out, m, inst->a); fprintf(out, ", v%u);\n", inst->b);
                break;
            case SIR_OP_ARRAY_SET:
                fprintf(out, "    "); sir_c_ref(out, m, inst->a); fprintf(out, ".data[v%u] = v%u;\n", inst->b, inst->c);
                break;
            case SIR_OP_STRUCT_NEW:
                fprintf(out, "    SirStruct v%u = {0};\n", inst->dst);
                break;
            case SIR_OP_FIELD_SET:
                if (inst->b < m->string_len) {
                    if (sir_c_value_type(m, inst->c) == SIR_TYPE_STRING) { fprintf(out, "    sir_struct_set_str(&"); sir_c_ref(out, m, inst->a); fprintf(out, ", \"%s\", v%u);\n", m->strings[inst->b].data, inst->c); }
                    else { fprintf(out, "    sir_struct_set_i64(&"); sir_c_ref(out, m, inst->a); fprintf(out, ", \"%s\", v%u);\n", m->strings[inst->b].data, inst->c); }
                }
                break;
            case SIR_OP_FIELD_GET:
                if (inst->b < m->string_len) {
                    if (inst->type == SIR_TYPE_STRING) { fprintf(out, "    const char* v%u = sir_struct_get_str(&", inst->dst); sir_c_ref(out, m, inst->a); fprintf(out, ", \"%s\");\n", m->strings[inst->b].data); }
                    else { fprintf(out, "    int64_t v%u = sir_struct_get_i64(&", inst->dst); sir_c_ref(out, m, inst->a); fprintf(out, ", \"%s\");\n", m->strings[inst->b].data); }
                }
                break;
            case SIR_OP_PRINT_I64:
                fprintf(out, "    printf(\"%%lld\\n\", (long long)v%u);\n", inst->a);
                break;
            case SIR_OP_PRINT_STRING: {
                SIRId sid = inst->a;
                for (uint32_t j = 0; j < m->inst_len; j++) {
                    if (m->insts[j].dst == inst->a && m->insts[j].op == SIR_OP_CONST_STRING) { sid = m->insts[j].a; break; }
                }
                if (sid < m->string_len) {
                    fprintf(out, "    fputs(\"");
                    sir_c_escape(out, m->strings[sid].data, m->strings[sid].len);
                    fprintf(out, "\", stdout);\n");
                } else {
                    fprintf(out, "    fputs(v%u, stdout);\n", inst->a);
                }
                break;
            }
            case SIR_OP_EXIT:
                fprintf(out, "    return %lld;\n", (long long)inst->imm);
                break;
            default:
                fprintf(out, "    // SIR op not lowered by MVP C backend yet: %s\n", sir_op_name(inst->op));
                break;
        }
    }
    if (!has_functions) fprintf(out, "    return 0;\n}\n");
    return 0;
}

#endif // SLOP_SIR_C_BACKEND_H
