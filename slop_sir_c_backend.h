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
        default: return "int64_t";
    }
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

static inline int sir_emit_c_backend(FILE* out, const SIRModule* m) {
    SIRVerifyResult vr = sir_verify_module(m, stderr);
    if (vr.errors) return 1;

    fprintf(out, "// Generated from Slop SIR by the SIR C backend MVP\n");
    fprintf(out, "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");
    fprintf(out, "static char* slop_sir_c_concat(const char* a, const char* b) { size_t la=strlen(a), lb=strlen(b); char* o=(char*)malloc(la+lb+1); memcpy(o,a,la); memcpy(o+la,b,lb); o[la+lb]=0; return o; }\n\n");
    fprintf(out, "int main(void) {\n");
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        switch (inst->op) {
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
                fprintf(out, "    %s l%u = v%u;\n", sir_c_type_name(inst->type), inst->dst, inst->b);
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
    fprintf(out, "    return 0;\n}\n");
    return 0;
}

#endif // SLOP_SIR_C_BACKEND_H
