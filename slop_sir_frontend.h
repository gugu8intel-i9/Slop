#ifndef SLOP_SIR_FRONTEND_H
#define SLOP_SIR_FRONTEND_H

// SIR-first Slop frontend slice.
// This is the first real production path that lowers .slop source directly to
// SIR before backend emission. It intentionally supports the safe beginner/core
// subset used by smoke tests and the direct native subset, and it fails closed
// with diagnostics for unsupported syntax instead of silently falling back.

#include "slop_lowering.h"
#include "slop_diagnostics.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    SIRId value;
    SIRType type;
} SlopSIRBinding;

typedef struct {
    SlopSIRBinding* data;
    uint32_t len;
    uint32_t cap;
} SlopSIRBindings;

static inline char* slop_sir_trim(char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
    return s;
}

static inline bool slop_sir_starts(const char* s, const char* p) { return strncmp(s, p, strlen(p)) == 0; }

static inline char* slop_sir_strdup_len(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = 0;
    return out;
}

static inline void slop_sir_bind(SlopSIRBindings* b, const char* name, SIRId value, SIRType type) {
    for (uint32_t i = 0; i < b->len; i++) {
        if (strcmp(b->data[i].name, name) == 0) { b->data[i].value = value; b->data[i].type = type; return; }
    }
    if (b->len == b->cap) {
        b->cap = b->cap ? b->cap * 2 : 16;
        b->data = (SlopSIRBinding*)realloc(b->data, (size_t)b->cap * sizeof(SlopSIRBinding));
        if (!b->data) exit(1);
    }
    b->data[b->len].name = strdup(name);
    b->data[b->len].value = value;
    b->data[b->len].type = type;
    b->len++;
}

static inline SlopSIRBinding* slop_sir_find(SlopSIRBindings* b, const char* name) {
    for (uint32_t i = 0; i < b->len; i++) if (strcmp(b->data[i].name, name) == 0) return &b->data[i];
    return NULL;
}

static inline void slop_sir_bindings_free(SlopSIRBindings* b) {
    for (uint32_t i = 0; i < b->len; i++) free(b->data[i].name);
    free(b->data);
    memset(b, 0, sizeof(*b));
}

static inline void slop_sir_strip_comment(char* s) {
    bool in_str = false, esc = false;
    for (size_t i = 0; s[i]; i++) {
        char c = s[i];
        if (esc) { esc = false; continue; }
        if (c == '\\' && in_str) { esc = true; continue; }
        if (c == '"') { in_str = !in_str; continue; }
        if (c == '#' && !in_str) { s[i] = 0; return; }
    }
}

static inline char* slop_sir_parse_string(const char* expr) {
    while (*expr && isspace((unsigned char)*expr)) expr++;
    if (*expr != '"') return NULL;
    expr++;
    char* out = (char*)malloc(strlen(expr) + 1);
    if (!out) return NULL;
    uint32_t j = 0;
    bool esc = false;
    for (; *expr; expr++) {
        char c = *expr;
        if (esc) {
            if (c == 'n') out[j++] = '\n';
            else if (c == 't') out[j++] = '\t';
            else if (c == 'r') out[j++] = '\r';
            else out[j++] = c;
            esc = false;
        } else if (c == '\\') esc = true;
        else if (c == '"') { out[j] = 0; return out; }
        else out[j++] = c;
    }
    free(out);
    return NULL;
}

static inline bool slop_sir_parse_i64(const char* expr, int64_t* out) {
    while (*expr && isspace((unsigned char)*expr)) expr++;
    char* end = NULL;
    long long v = strtoll(expr, &end, 10);
    if (end == expr) return false;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end) return false;
    *out = (int64_t)v;
    return true;
}

static inline SIRId slop_sir_lower_expr(SlopLowering* l, SlopSIRBindings* bindings, char* expr, SIRType* out_type) {
    expr = slop_sir_trim(expr);
    int depth = 0;
    bool in_str = false, esc = false;
    for (char* p = expr; *p; p++) {
        if (esc) { esc = false; continue; }
        if (*p == '\\' && in_str) { esc = true; continue; }
        if (*p == '"') { in_str = !in_str; continue; }
        if (in_str) continue;
        if (*p == '(') depth++;
        else if (*p == ')') depth--;
        else if (*p == '+' && depth == 0) {
            *p = 0;
            SIRType lt = SIR_TYPE_VOID, rt = SIR_TYPE_VOID;
            SIRId a = slop_sir_lower_expr(l, bindings, expr, &lt);
            SIRId b = slop_sir_lower_expr(l, bindings, p + 1, &rt);
            if (lt == SIR_TYPE_STRING || rt == SIR_TYPE_STRING) { *out_type = SIR_TYPE_STRING; return slop_lower_string_concat(l, a, b); }
            *out_type = SIR_TYPE_I64;
            return slop_lower_add_i64(l, a, b);
        }
    }
    char* str = slop_sir_parse_string(expr);
    if (str) { SIRId id = slop_lower_string_literal(l, str); free(str); *out_type = SIR_TYPE_STRING; return id; }
    int64_t iv = 0;
    if (slop_sir_parse_i64(expr, &iv)) { *out_type = SIR_TYPE_I64; return slop_lower_i64(l, iv); }
    SlopSIRBinding* b = slop_sir_find(bindings, expr);
    if (b) { *out_type = b->type; return b->value; }
    *out_type = SIR_TYPE_VOID;
    return 0;
}

static inline bool slop_sir_lower_source(const char* filename, const char* source, SIRModule* out, FILE* diag) {
    sir_module_init(out);
    SlopLowering lowering;
    slop_lowering_init(&lowering, out);
    SlopSIRBindings bindings = {0};
    slop_lower_function_begin(&lowering, "main");
    slop_lower_block(&lowering, "entry");

    char* copy = strdup(source);
    if (!copy) return false;
    char* save = NULL;
    uint32_t line_no = 0;
    for (char* line = strtok_r(copy, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        line_no++;
        slop_sir_strip_comment(line);
        char* t = slop_sir_trim(line);
        if (!*t || slop_sir_starts(t, "fn ") || strcmp(t, "{") == 0 || strcmp(t, "}") == 0) continue;
        if (slop_sir_starts(t, "let ")) {
            char* p = t + 4;
            while (*p && isspace((unsigned char)*p)) p++;
            char* name_start = p;
            while (*p && (isalnum((unsigned char)*p) || *p == '_')) p++;
            char* name = slop_sir_strdup_len(name_start, (size_t)(p - name_start));
            while (*p && *p != '=') p++;
            if (*p != '=') { slop_print_diagnostic(diag, (SlopDiagnostic){SLOP_DIAG_ERROR, filename, line_no, 1, "expected '=' in let declaration", "write: let name = value", t}); free(name); goto fail; }
            p++;
            SIRType type = SIR_TYPE_VOID;
            SIRId value = slop_sir_lower_expr(&lowering, &bindings, p, &type);
            if (!value) { slop_print_diagnostic(diag, slop_diag_unknown_name(filename, line_no, 1, p, t)); free(name); goto fail; }
            SIRId local = slop_lower_let(&lowering, name, type, value);
            (void)local;
            slop_sir_bind(&bindings, name, value, type);
            free(name);
            continue;
        }
        if (slop_sir_starts(t, "print")) {
            char* p = strchr(t, '(');
            char* end = strrchr(t, ')');
            if (!p || !end || end <= p) { slop_print_diagnostic(diag, (SlopDiagnostic){SLOP_DIAG_ERROR, filename, line_no, 1, "expected print(expr)", "example: print(42)", t}); goto fail; }
            *end = 0;
            p++;
            SIRType type = SIR_TYPE_VOID;
            SIRId value = slop_sir_lower_expr(&lowering, &bindings, p, &type);
            if (!value) { slop_print_diagnostic(diag, slop_diag_unknown_name(filename, line_no, 1, p, t)); goto fail; }
            if (type == SIR_TYPE_STRING) {
                SIRId nl = slop_lower_string_literal(&lowering, "\n");
                SIRId with_nl = slop_lower_string_concat(&lowering, value, nl);
                slop_lower_print_string_value(&lowering, with_nl);
            } else slop_lower_print_i64(&lowering, value);
            continue;
        }
        slop_print_diagnostic(diag, (SlopDiagnostic){SLOP_DIAG_ERROR, filename, line_no, 1, "SIR-first frontend does not support this statement yet", "use the stable slop-compiler C backend for full language support", t});
        goto fail;
    }
    sir_emit_exit(out, 0);
    free(copy);
    slop_sir_bindings_free(&bindings);
    return true;
fail:
    free(copy);
    slop_sir_bindings_free(&bindings);
    sir_module_free(out);
    return false;
}

#endif // SLOP_SIR_FRONTEND_H
