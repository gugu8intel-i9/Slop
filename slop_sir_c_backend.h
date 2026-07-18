#ifndef SLOP_SIR_C_BACKEND_H
#define SLOP_SIR_C_BACKEND_H

// SIR -> C backend MVP. This is the Phase-1 proof that the portable C backend
// can consume SIR instead of each backend owning its own frontend lowering.
// It intentionally supports the side-effect subset used by the current SIR MVP
// plus integer constants/prints for lowering tests.

#include "slop_ir.h"
#include "slop_ir_tools.h"

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
    fprintf(out, "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n\n");
    fprintf(out, "int main(void) {\n");
    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        switch (inst->op) {
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
            case SIR_OP_PRINT_I64:
                fprintf(out, "    printf(\"%%lld\\n\", (long long)v%u);\n", inst->a);
                break;
            case SIR_OP_PRINT_STRING:
                if (inst->a < m->string_len) {
                    fprintf(out, "    fputs(\"");
                    sir_c_escape(out, m->strings[inst->a].data, m->strings[inst->a].len);
                    fprintf(out, "\", stdout);\n");
                } else {
                    fprintf(out, "    // print.string value v%u requires runtime string lowering\n", inst->a);
                }
                break;
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
