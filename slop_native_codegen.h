#ifndef SLOP_NATIVE_CODEGEN_H
#define SLOP_NATIVE_CODEGEN_H

// Phase 4 native codegen infrastructure.
// Target-neutral scaffolding for instruction selection, virtual registers,
// liveness, linear-scan register allocation, spill slots, stack frames, ABI
// lowering, peephole passes, and target feature selection.
//
// This header is intentionally lightweight and deterministic: compact arrays,
// dense ids, no hidden global state. The real backends can use it directly or
// mirror the same contract in the self-hosted compiler.

#include "slop_ir.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SLOP_TARGET_X86_64_LINUX = 0,
    SLOP_TARGET_X86_64_LINUX_ELF,
    SLOP_TARGET_AARCH64_LINUX,
    SLOP_TARGET_ARMV7_LINUX,
    SLOP_TARGET_RISCV64_LINUX,
    SLOP_TARGET_WINDOWS_X64,
    SLOP_TARGET_MACOS_X86_64,
    SLOP_TARGET_MACOS_ARM64,
    SLOP_TARGET_WASM32_WASI
} SlopTargetKind;

typedef struct {
    SlopTargetKind kind;
    const char* name;
    const char* object_format;
    const char* abi;
    uint8_t pointer_size;
    uint8_t int_reg_count;
    const char* int_regs[32];
    const char* arg_regs[16];
    uint8_t arg_reg_count;
    const char* ret_reg;
    bool supports_direct_syscall;
    bool supports_direct_elf;
} SlopTargetInfo;

static inline SlopTargetInfo slop_target_info(SlopTargetKind kind) {
    SlopTargetInfo t;
    memset(&t, 0, sizeof(t));
    t.kind = kind;
    t.pointer_size = 8;
    switch (kind) {
        case SLOP_TARGET_X86_64_LINUX:
        case SLOP_TARGET_X86_64_LINUX_ELF:
            t.name = kind == SLOP_TARGET_X86_64_LINUX_ELF ? "x86_64-linux-elf" : "x86_64-linux";
            t.object_format = "ELF64";
            t.abi = "SysV AMD64";
            t.int_reg_count = 10;
            t.int_regs[0]="rax"; t.int_regs[1]="rbx"; t.int_regs[2]="rcx"; t.int_regs[3]="rdx"; t.int_regs[4]="rsi"; t.int_regs[5]="rdi"; t.int_regs[6]="r8"; t.int_regs[7]="r9"; t.int_regs[8]="r10"; t.int_regs[9]="r11";
            t.arg_reg_count = 6;
            t.arg_regs[0]="rdi"; t.arg_regs[1]="rsi"; t.arg_regs[2]="rdx"; t.arg_regs[3]="rcx"; t.arg_regs[4]="r8"; t.arg_regs[5]="r9";
            t.ret_reg = "rax";
            t.supports_direct_syscall = true;
            t.supports_direct_elf = kind == SLOP_TARGET_X86_64_LINUX_ELF;
            break;
        case SLOP_TARGET_AARCH64_LINUX:
            t.name = "aarch64-linux"; t.object_format = "ELF64"; t.abi = "AAPCS64";
            t.int_reg_count = 16;
            for (uint8_t i=0;i<16;i++) t.int_regs[i] = "x";
            t.arg_reg_count = 8;
            for (uint8_t i=0;i<8;i++) t.arg_regs[i] = "x";
            t.ret_reg = "x0"; t.supports_direct_syscall = true;
            break;
        case SLOP_TARGET_ARMV7_LINUX:
            t.name = "armv7-linux"; t.object_format = "ELF32"; t.abi = "AAPCS32"; t.pointer_size = 4;
            t.int_reg_count = 10;
            t.int_regs[0]="r0"; t.int_regs[1]="r1"; t.int_regs[2]="r2"; t.int_regs[3]="r3"; t.int_regs[4]="r4"; t.int_regs[5]="r5"; t.int_regs[6]="r6"; t.int_regs[7]="r7"; t.int_regs[8]="r8"; t.int_regs[9]="r9";
            t.arg_reg_count = 4;
            t.arg_regs[0]="r0"; t.arg_regs[1]="r1"; t.arg_regs[2]="r2"; t.arg_regs[3]="r3";
            t.ret_reg = "r0"; t.supports_direct_syscall = true;
            break;
        case SLOP_TARGET_RISCV64_LINUX:
            t.name = "riscv64-linux"; t.object_format = "ELF64"; t.abi = "RISC-V ELF psABI";
            t.int_reg_count = 15;
            t.int_regs[0]="a0"; t.int_regs[1]="a1"; t.int_regs[2]="a2"; t.int_regs[3]="a3"; t.int_regs[4]="a4"; t.int_regs[5]="a5"; t.int_regs[6]="a6"; t.int_regs[7]="a7"; t.int_regs[8]="t0"; t.int_regs[9]="t1"; t.int_regs[10]="t2"; t.int_regs[11]="t3"; t.int_regs[12]="t4"; t.int_regs[13]="t5"; t.int_regs[14]="t6";
            t.arg_reg_count = 8;
            t.arg_regs[0]="a0"; t.arg_regs[1]="a1"; t.arg_regs[2]="a2"; t.arg_regs[3]="a3"; t.arg_regs[4]="a4"; t.arg_regs[5]="a5"; t.arg_regs[6]="a6"; t.arg_regs[7]="a7";
            t.ret_reg = "a0"; t.supports_direct_syscall = true;
            break;
        case SLOP_TARGET_WINDOWS_X64:
            t.name = "windows-x64"; t.object_format = "COFF"; t.abi = "Microsoft x64"; t.arg_reg_count = 4;
            t.arg_regs[0]="rcx"; t.arg_regs[1]="rdx"; t.arg_regs[2]="r8"; t.arg_regs[3]="r9"; t.ret_reg="rax";
            break;
        case SLOP_TARGET_MACOS_X86_64:
            t.name = "macos-x86_64"; t.object_format = "Mach-O 64"; t.abi = "Darwin SysV"; t.ret_reg="rax";
            break;
        case SLOP_TARGET_MACOS_ARM64:
            t.name = "macos-arm64"; t.object_format = "Mach-O 64"; t.abi = "Apple ARM64"; t.ret_reg="x0";
            break;
        case SLOP_TARGET_WASM32_WASI:
            t.name = "wasm32-wasi"; t.object_format = "WASM"; t.abi = "WASI"; t.pointer_size = 4;
            break;
    }
    return t;
}

typedef struct {
    SIRId value;
    uint32_t start;
    uint32_t end;
    int16_t phys_reg;
    int16_t spill_slot;
} SlopLiveInterval;

typedef struct {
    SlopLiveInterval* intervals;
    uint32_t len;
    uint32_t cap;
    uint32_t spill_slots;
} SlopRegAlloc;

static inline void slop_ra_push(SlopRegAlloc* ra, SlopLiveInterval iv) {
    if (ra->len == ra->cap) {
        ra->cap = ra->cap ? ra->cap * 2 : 32;
        ra->intervals = (SlopLiveInterval*)realloc(ra->intervals, (size_t)ra->cap * sizeof(SlopLiveInterval));
        if (!ra->intervals) { fprintf(stderr, "regalloc out of memory\n"); exit(1); }
    }
    ra->intervals[ra->len++] = iv;
}

static inline int slop_interval_cmp(const void* a, const void* b) {
    const SlopLiveInterval* x = (const SlopLiveInterval*)a;
    const SlopLiveInterval* y = (const SlopLiveInterval*)b;
    if (x->start != y->start) return x->start < y->start ? -1 : 1;
    return x->value < y->value ? -1 : (x->value > y->value);
}

static inline SlopRegAlloc slop_build_liveness(const SIRModule* m) {
    SlopRegAlloc ra;
    memset(&ra, 0, sizeof(ra));
    if (m->next_value == 0) return ra;
    uint32_t* first = (uint32_t*)malloc((size_t)m->next_value * sizeof(uint32_t));
    uint32_t* last = (uint32_t*)calloc(m->next_value, sizeof(uint32_t));
    if (!first || !last) { free(first); free(last); return ra; }
    for (uint32_t i=0;i<m->next_value;i++) first[i] = UINT32_MAX;
    for (uint32_t i=0;i<m->inst_len;i++) {
        const SIRInst* in = &m->insts[i];
        if (in->dst && in->dst < m->next_value && first[in->dst] == UINT32_MAX) first[in->dst] = i;
        SIRId ops[3] = { in->a, in->b, in->c };
        for (uint32_t k=0;k<3;k++) if (ops[k] && ops[k] < m->next_value) {
            if (first[ops[k]] == UINT32_MAX) first[ops[k]] = i;
            if (last[ops[k]] < i) last[ops[k]] = i;
        }
    }
    for (SIRId v=1; v<m->next_value; v++) if (first[v] != UINT32_MAX) {
        SlopLiveInterval iv = { v, first[v], last[v] < first[v] ? first[v] : last[v], -1, -1 };
        slop_ra_push(&ra, iv);
    }
    qsort(ra.intervals, ra.len, sizeof(SlopLiveInterval), slop_interval_cmp);
    free(first); free(last);
    return ra;
}

static inline void slop_linear_scan_allocate(SlopRegAlloc* ra, uint8_t phys_reg_count) {
    if (phys_reg_count == 0) phys_reg_count = 1;
    uint32_t* active = (uint32_t*)malloc((size_t)ra->len * sizeof(uint32_t));
    bool used[64] = {0};
    uint32_t active_len = 0;
    for (uint32_t i=0;i<ra->len;i++) {
        for (uint32_t j=0;j<active_len;) {
            SlopLiveInterval* a = &ra->intervals[active[j]];
            if (a->end < ra->intervals[i].start) {
                if (a->phys_reg >= 0 && a->phys_reg < 64) used[a->phys_reg] = false;
                active[j] = active[--active_len];
            } else j++;
        }
        int reg = -1;
        for (uint8_t r=0;r<phys_reg_count && r<64;r++) if (!used[r]) { reg = r; break; }
        if (reg >= 0) {
            ra->intervals[i].phys_reg = (int16_t)reg;
            used[reg] = true;
            active[active_len++] = i;
        } else {
            ra->intervals[i].spill_slot = (int16_t)ra->spill_slots++;
        }
    }
    free(active);
}

static inline void slop_regalloc_free(SlopRegAlloc* ra) {
    free(ra->intervals);
    memset(ra, 0, sizeof(*ra));
}

typedef struct {
    uint32_t stack_size;
    uint32_t spill_slots;
    uint32_t callee_saved_mask;
} SlopStackFrame;

static inline SlopStackFrame slop_compute_stack_frame(const SlopRegAlloc* ra, const SlopTargetInfo* target) {
    SlopStackFrame f;
    memset(&f, 0, sizeof(f));
    f.spill_slots = ra->spill_slots;
    uint32_t slot = target->pointer_size ? target->pointer_size : 8;
    f.stack_size = ((ra->spill_slots * slot) + 15u) & ~15u;
    return f;
}

static inline void slop_dump_regalloc(FILE* out, const SlopRegAlloc* ra, const SlopTargetInfo* target) {
    fprintf(out, "regalloc target=%s intervals=%u spills=%u\n", target->name, ra->len, ra->spill_slots);
    for (uint32_t i=0;i<ra->len;i++) {
        const SlopLiveInterval* iv = &ra->intervals[i];
        fprintf(out, "  v%u [%u,%u] ", iv->value, iv->start, iv->end);
        if (iv->phys_reg >= 0 && (uint8_t)iv->phys_reg < target->int_reg_count) fprintf(out, "reg=%s\n", target->int_regs[iv->phys_reg]);
        else fprintf(out, "spill=%d\n", iv->spill_slot);
    }
}

#endif // SLOP_NATIVE_CODEGEN_H
