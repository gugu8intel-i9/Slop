// Slop SIR Pipeline CLI
// Real SIR-first backend driver:
//   .sir -> verify -> optimize -> backend output
// Backends today: C, x86_64-linux-elf direct executable.

#define _GNU_SOURCE
#include "slop_pipeline.h"
#include "slop_sir_frontend.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void usage(const char* argv0) {
    fprintf(stderr, "Usage: %s <input.sir> <output> <backend> [target]\n", argv0);
    fprintf(stderr, "Backends: c, elf\n");
    fprintf(stderr, "Targets : x86_64-linux-elf (for elf)\n");
}

static int parse_backend(const char* s, SlopBackendKind* out) {
    if (strcmp(s, "c") == 0 || strcmp(s, "C") == 0) { *out = SLOP_BACKEND_C; return 1; }
    if (strcmp(s, "elf") == 0 || strcmp(s, "x86_64-linux-elf") == 0) { *out = SLOP_BACKEND_DIRECT_ELF; return 1; }
    return 0;
}

static int parse_pipeline_target(const char* s, SlopTargetKind* out) {
    if (!s || strcmp(s, "x86_64-linux-elf") == 0 || strcmp(s, "elf64-x86_64") == 0) { *out = SLOP_TARGET_X86_64_LINUX_ELF; return 1; }
    if (strcmp(s, "x86_64-linux") == 0) { *out = SLOP_TARGET_X86_64_LINUX; return 1; }
    if (strcmp(s, "aarch64-linux") == 0 || strcmp(s, "arm64-linux") == 0) { *out = SLOP_TARGET_AARCH64_LINUX; return 1; }
    if (strcmp(s, "armv7-linux") == 0) { *out = SLOP_TARGET_ARMV7_LINUX; return 1; }
    if (strcmp(s, "riscv64-linux") == 0) { *out = SLOP_TARGET_RISCV64_LINUX; return 1; }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        usage(argv[0]);
        return 2;
    }

    SIRModule module;
    const char* ext = strrchr(argv[1], '.');
    if (ext && strcmp(ext, ".slop") == 0) {
        FILE* sf = fopen(argv[1], "rb");
        if (!sf) { fprintf(stderr, "failed to open %s: %s\n", argv[1], strerror(errno)); return 1; }
        fseek(sf, 0, SEEK_END);
        long n = ftell(sf);
        rewind(sf);
        char* src = (char*)calloc((size_t)n + 1, 1);
        if (!src) return 1;
        fread(src, 1, (size_t)n, sf);
        fclose(sf);
        if (!slop_sir_lower_source(argv[1], src, &module, stderr)) { free(src); return 1; }
        free(src);
    } else {
        FILE* in = fopen(argv[1], "rb");
        if (!in) {
            fprintf(stderr, "failed to open %s: %s\n", argv[1], strerror(errno));
            return 1;
        }
        if (!sir_load_text(in, &module, stderr)) {
            fclose(in);
            return 1;
        }
        fclose(in);
    }

    SlopPipelineOptions opt = slop_pipeline_default();
    if (!parse_backend(argv[3], &opt.backend)) {
        fprintf(stderr, "unknown backend: %s\n", argv[3]);
        sir_module_free(&module);
        usage(argv[0]);
        return 2;
    }
    if (!parse_pipeline_target(argc == 5 ? argv[4] : NULL, &opt.target)) {
        fprintf(stderr, "unknown target: %s\n", argv[4]);
        sir_module_free(&module);
        usage(argv[0]);
        return 2;
    }
    if (opt.backend == SLOP_BACKEND_C) {
        opt.target = SLOP_TARGET_X86_64_LINUX;
    }

    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "failed to open %s: %s\n", argv[2], strerror(errno));
        sir_module_free(&module);
        return 1;
    }

    int rc = slop_pipeline_emit(out, &module, opt, stderr);
    fclose(out);
    if (rc == 0 && opt.backend == SLOP_BACKEND_DIRECT_ELF) {
        chmod(argv[2], 0755);
    }
    if (rc == 0) {
        fprintf(stdout, "Wrote %s via SIR pipeline\n", argv[2]);
    }
    sir_module_free(&module);
    return rc;
}
