#ifndef SLOP_DIAGNOSTICS_H
#define SLOP_DIAGNOSTICS_H

// Beginner-first diagnostics for Slop.
// The compiler can keep the language easy by producing fixes, not puzzles.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    SLOP_DIAG_NOTE,
    SLOP_DIAG_WARNING,
    SLOP_DIAG_ERROR
} SlopDiagLevel;

typedef struct {
    SlopDiagLevel level;
    const char* file;
    uint32_t line;
    uint32_t column;
    const char* message;
    const char* hint;
    const char* snippet;
} SlopDiagnostic;

static inline const char* slop_diag_level_name(SlopDiagLevel l) {
    switch (l) {
        case SLOP_DIAG_NOTE: return "note";
        case SLOP_DIAG_WARNING: return "warning";
        case SLOP_DIAG_ERROR: return "error";
    }
    return "diagnostic";
}

static inline void slop_print_diagnostic(FILE* out, SlopDiagnostic d) {
    fprintf(out, "%s:%u:%u: %s: %s\n", d.file ? d.file : "<input>", d.line, d.column, slop_diag_level_name(d.level), d.message ? d.message : "");
    if (d.snippet && d.snippet[0]) {
        fprintf(out, "  %s\n", d.snippet);
        fprintf(out, "  ");
        for (uint32_t i = 1; i < d.column; i++) fputc(' ', out);
        fprintf(out, "^\n");
    }
    if (d.hint && d.hint[0]) fprintf(out, "hint: %s\n", d.hint);
}

static inline SlopDiagnostic slop_diag_type_mismatch(const char* file, uint32_t line, uint32_t col, const char* expected, const char* got, const char* snippet) {
    static char msg[256];
    static char hint[256];
    snprintf(msg, sizeof(msg), "expected %s, got %s", expected, got);
    snprintf(hint, sizeof(hint), "Slop is statically typed but inferred; convert explicitly or change the variable type.");
    SlopDiagnostic d = { SLOP_DIAG_ERROR, file, line, col, msg, hint, snippet };
    return d;
}

static inline SlopDiagnostic slop_diag_unknown_name(const char* file, uint32_t line, uint32_t col, const char* name, const char* snippet) {
    static char msg[256];
    static char hint[256];
    snprintf(msg, sizeof(msg), "unknown name '%s'", name);
    snprintf(hint, sizeof(hint), "declare it with 'let %s = ...' or check spelling", name);
    SlopDiagnostic d = { SLOP_DIAG_ERROR, file, line, col, msg, hint, snippet };
    return d;
}

#endif // SLOP_DIAGNOSTICS_H
