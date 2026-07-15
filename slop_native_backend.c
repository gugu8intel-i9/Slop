// Slop Native Backend MVP
// Direct x86_64 Linux assembly emitter for a small, fast, syscall-only Slop subset.
// This is intentionally separate from the portable C backend. The architecture is
// backend-pluggable: add new emitters without changing Slop source syntax.
//
// Supported today:
//   fn main(args: array[string]) { ... }
//   print("literal")
//   print(123)
//   let name = 123
//   let name = "literal"
//   print(name)
//
// Output target today: x86_64-linux-gnu assembly using Linux write/exit syscalls.
// Assemble/link example:
//   ./slop-native-backend native_backend_demo.slop native_backend_demo.s
//   as --64 native_backend_demo.s -o native_backend_demo.o
//   ld -o native_backend_demo native_backend_demo.o

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    char* value;
    bool is_string;
} Binding;

typedef struct {
    Binding* data;
    size_t len;
    size_t cap;
} Bindings;

typedef struct {
    char** data;
    size_t len;
    size_t cap;
} StringList;

static void die(const char* msg) {
    fprintf(stderr, "slop-native-backend: %s\n", msg);
    exit(1);
}

static char* read_all(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "slop-native-backend: failed to open %s: %s\n", path, strerror(errno));
        exit(1);
    }
    if (fseek(f, 0, SEEK_END) != 0) die("fseek failed");
    long n = ftell(f);
    if (n < 0) die("ftell failed");
    rewind(f);
    char* buf = (char*)calloc((size_t)n + 1, 1);
    if (!buf) die("out of memory");
    size_t got = fread(buf, 1, (size_t)n, f);
    if (got != (size_t)n) die("read failed");
    fclose(f);
    return buf;
}

static void* xrealloc(void* p, size_t n) {
    void* q = realloc(p, n);
    if (!q) die("out of memory");
    return q;
}

static char* xstrdup_len(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) die("out of memory");
    memcpy(out, s, n);
    out[n] = 0;
    return out;
}

static char* xstrdup0(const char* s) {
    return xstrdup_len(s, strlen(s));
}

static char* trim_in_place(char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = 0;
    return s;
}

static void strip_comment(char* s) {
    bool in_str = false;
    bool esc = false;
    for (size_t i = 0; s[i]; i++) {
        char c = s[i];
        if (esc) {
            esc = false;
            continue;
        }
        if (c == '\\' && in_str) {
            esc = true;
            continue;
        }
        if (c == '"') {
            in_str = !in_str;
            continue;
        }
        if (c == '#' && !in_str) {
            s[i] = 0;
            return;
        }
    }
}

static bool starts_with(const char* s, const char* prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void strings_push(StringList* xs, char* s) {
    if (xs->len == xs->cap) {
        xs->cap = xs->cap ? xs->cap * 2 : 16;
        xs->data = (char**)xrealloc(xs->data, xs->cap * sizeof(char*));
    }
    xs->data[xs->len++] = s;
}

static void bindings_set(Bindings* b, const char* name, const char* value, bool is_string) {
    for (size_t i = 0; i < b->len; i++) {
        if (strcmp(b->data[i].name, name) == 0) {
            free(b->data[i].value);
            b->data[i].value = xstrdup0(value);
            b->data[i].is_string = is_string;
            return;
        }
    }
    if (b->len == b->cap) {
        b->cap = b->cap ? b->cap * 2 : 16;
        b->data = (Binding*)xrealloc(b->data, b->cap * sizeof(Binding));
    }
    b->data[b->len].name = xstrdup0(name);
    b->data[b->len].value = xstrdup0(value);
    b->data[b->len].is_string = is_string;
    b->len++;
}

static Binding* bindings_get(Bindings* b, const char* name) {
    for (size_t i = 0; i < b->len; i++) {
        if (strcmp(b->data[i].name, name) == 0) return &b->data[i];
    }
    return NULL;
}

static char* parse_string_literal(const char* expr) {
    const char* p = expr;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '"') return NULL;
    p++;
    char* out = (char*)malloc(strlen(p) + 2);
    if (!out) die("out of memory");
    size_t j = 0;
    bool esc = false;
    for (; *p; p++) {
        char c = *p;
        if (esc) {
            if (c == 'n') out[j++] = '\n';
            else if (c == 't') out[j++] = '\t';
            else if (c == 'r') out[j++] = '\r';
            else if (c == '\\') out[j++] = '\\';
            else if (c == '"') out[j++] = '"';
            else out[j++] = c;
            esc = false;
            continue;
        }
        if (c == '\\') {
            esc = true;
            continue;
        }
        if (c == '"') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
            out[j] = 0;
            return out;
        }
        out[j++] = c;
    }
    free(out);
    return NULL;
}

static bool parse_int_literal(const char* expr, int64_t* out) {
    char* end = NULL;
    while (*expr && isspace((unsigned char)*expr)) expr++;
    errno = 0;
    long long v = strtoll(expr, &end, 10);
    if (expr == end || errno != 0) return false;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != 0) return false;
    *out = (int64_t)v;
    return true;
}

static char* int_to_string_alloc(int64_t v) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%lld", (long long)v);
    return xstrdup0(tmp);
}

static char* assembly_escape(const char* s) {
    size_t n = strlen(s);
    char* out = (char*)malloc(n * 4 + 1);
    if (!out) die("out of memory");
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\') { out[j++]='\\'; out[j++]='\\'; }
        else if (c == '"') { out[j++]='\\'; out[j++]='"'; }
        else if (c == '\n') { out[j++]='\\'; out[j++]='n'; }
        else if (c == '\t') { out[j++]='\\'; out[j++]='t'; }
        else if (c == '\r') { out[j++]='\\'; out[j++]='r'; }
        else if (c < 32 || c >= 127) {
            static const char hex[] = "0123456789abcdef";
            out[j++]='\\'; out[j++]='x'; out[j++]=hex[c >> 4]; out[j++]=hex[c & 15];
        } else {
            out[j++] = (char)c;
        }
    }
    out[j] = 0;
    return out;
}

static void emit_write(FILE* out, size_t label_id) {
    fprintf(out, "    mov $1, %%rax\n");
    fprintf(out, "    mov $1, %%rdi\n");
    fprintf(out, "    lea .Lmsg%zu(%%rip), %%rsi\n", label_id);
    fprintf(out, "    mov $.Lmsg%zu_len, %%rdx\n", label_id);
    fprintf(out, "    syscall\n");
}

static char* extract_call_expr(char* line, const char* name) {
    size_t name_len = strlen(name);
    if (!starts_with(line, name)) return NULL;
    char* p = line + name_len;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '(') return NULL;
    p++;
    char* start = p;
    int depth = 1;
    bool in_str = false;
    bool esc = false;
    while (*p) {
        char c = *p;
        if (esc) { esc = false; p++; continue; }
        if (c == '\\' && in_str) { esc = true; p++; continue; }
        if (c == '"') { in_str = !in_str; p++; continue; }
        if (!in_str) {
            if (c == '(') depth++;
            if (c == ')') {
                depth--;
                if (depth == 0) {
                    *p = 0;
                    return trim_in_place(start);
                }
            }
        }
        p++;
    }
    return NULL;
}

static char* parse_let_name_and_expr(char* line, char** expr_out) {
    if (!starts_with(line, "let")) return NULL;
    char* p = line + 3;
    if (*p && !isspace((unsigned char)*p)) return NULL;
    while (*p && isspace((unsigned char)*p)) p++;
    char* name_start = p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) p++;
    if (p == name_start) return NULL;
    char* name = xstrdup_len(name_start, (size_t)(p - name_start));
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ':') {
        while (*p && *p != '=') p++;
    }
    if (*p != '=') { free(name); return NULL; }
    p++;
    *expr_out = trim_in_place(p);
    return name;
}

static void compile_file(const char* in_path, const char* out_path) {
    char* src = read_all(in_path);
    FILE* out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "slop-native-backend: failed to open %s: %s\n", out_path, strerror(errno));
        exit(1);
    }

    StringList messages = {0};
    Bindings bindings = {0};

    fprintf(out, "# Generated by Slop native backend MVP: x86_64-linux syscall target\n");
    fprintf(out, ".global _start\n.section .text\n_start:\n");

    char* save = NULL;
    for (char* line = strtok_r(src, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        strip_comment(line);
        char* t = trim_in_place(line);
        if (*t == 0) continue;
        if (starts_with(t, "fn ") || strcmp(t, "{") == 0 || strcmp(t, "}") == 0) continue;

        char* expr = NULL;
        char* let_name = parse_let_name_and_expr(t, &expr);
        if (let_name) {
            char* sv = parse_string_literal(expr);
            if (sv) {
                bindings_set(&bindings, let_name, sv, true);
                free(sv);
                free(let_name);
                continue;
            }
            int64_t iv = 0;
            if (parse_int_literal(expr, &iv)) {
                char* is = int_to_string_alloc(iv);
                bindings_set(&bindings, let_name, is, false);
                free(is);
                free(let_name);
                continue;
            }
            fprintf(stderr, "slop-native-backend: unsupported let expression: %s\n", expr);
            exit(1);
        }

        expr = extract_call_expr(t, "print");
        if (expr) {
            char* sv = parse_string_literal(expr);
            if (sv) {
                size_t n = strlen(sv);
                char* msg = (char*)malloc(n + 2);
                if (!msg) die("out of memory");
                memcpy(msg, sv, n);
                msg[n] = '\n';
                msg[n + 1] = 0;
                size_t id = messages.len;
                strings_push(&messages, msg);
                emit_write(out, id);
                free(sv);
                continue;
            }
            int64_t iv = 0;
            if (parse_int_literal(expr, &iv)) {
                char* is = int_to_string_alloc(iv);
                size_t n = strlen(is);
                char* msg = (char*)malloc(n + 2);
                if (!msg) die("out of memory");
                memcpy(msg, is, n);
                msg[n] = '\n';
                msg[n + 1] = 0;
                size_t id = messages.len;
                strings_push(&messages, msg);
                emit_write(out, id);
                free(is);
                continue;
            }
            Binding* b = bindings_get(&bindings, expr);
            if (b) {
                size_t n = strlen(b->value);
                char* msg = (char*)malloc(n + 2);
                if (!msg) die("out of memory");
                memcpy(msg, b->value, n);
                msg[n] = '\n';
                msg[n + 1] = 0;
                size_t id = messages.len;
                strings_push(&messages, msg);
                emit_write(out, id);
                continue;
            }
            fprintf(stderr, "slop-native-backend: unsupported print expression: %s\n", expr);
            exit(1);
        }

        if (starts_with(t, "while") || starts_with(t, "if") || starts_with(t, "return")) {
            fprintf(stderr, "slop-native-backend: control flow is not in the MVP native subset yet: %s\n", t);
            exit(1);
        }

        fprintf(stderr, "slop-native-backend: unsupported statement: %s\n", t);
        exit(1);
    }

    fprintf(out, "    mov $60, %%rax\n");
    fprintf(out, "    xor %%rdi, %%rdi\n");
    fprintf(out, "    syscall\n\n");
    fprintf(out, ".section .rodata\n");
    for (size_t i = 0; i < messages.len; i++) {
        char* esc = assembly_escape(messages.data[i]);
        fprintf(out, ".Lmsg%zu:\n", i);
        fprintf(out, "    .ascii \"%s\"\n", esc);
        fprintf(out, ".set .Lmsg%zu_len, . - .Lmsg%zu\n", i, i);
        free(esc);
    }

    fclose(out);
    free(src);
    for (size_t i = 0; i < messages.len; i++) free(messages.data[i]);
    free(messages.data);
    for (size_t i = 0; i < bindings.len; i++) {
        free(bindings.data[i].name);
        free(bindings.data[i].value);
    }
    free(bindings.data);
}

static void usage(const char* argv0) {
    fprintf(stderr, "Usage: %s <input.slop> <output.s>\n", argv0);
    fprintf(stderr, "Target: x86_64-linux-gnu assembly, syscall-only MVP subset.\n");
}

int main(int argc, char** argv) {
    if (argc != 3) {
        usage(argv[0]);
        return 2;
    }
    compile_file(argv[1], argv[2]);
    printf("Wrote native assembly %s\n", argv[2]);
    return 0;
}
