#ifndef SLOP_RUNTIME_ABI_H
#define SLOP_RUNTIME_ABI_H

// Phase 6 runtime/ABI compatibility layer.
// Stable native layout descriptions used by native backends, C interop, C++
// bridge, Rust bridge, no-libc syscall mode, and future libc mode.

#include "slop_native_codegen.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char* data;
    uint64_t length;
} SlopABIString;

typedef struct {
    void** data;
    uint64_t length;
    uint64_t capacity;
} SlopABIArray;

typedef struct {
    void* head;
    void* current;
    uint64_t block_size;
} SlopABIArena;

typedef enum {
    SLOP_ABI_CLASS_VOID,
    SLOP_ABI_CLASS_INTEGER,
    SLOP_ABI_CLASS_FLOAT,
    SLOP_ABI_CLASS_MEMORY,
    SLOP_ABI_CLASS_STRING,
    SLOP_ABI_CLASS_ARRAY,
    SLOP_ABI_CLASS_STRUCT
} SlopABIClass;

typedef struct {
    SlopABIClass cls;
    uint32_t size;
    uint32_t align;
    bool pass_by_pointer;
} SlopABIType;

static inline SlopABIType slop_abi_type_for_sir(SIRType t, SlopTargetKind target) {
    SlopTargetInfo info = slop_target_info(target);
    SlopABIType out;
    out.cls = SLOP_ABI_CLASS_MEMORY;
    out.size = info.pointer_size;
    out.align = info.pointer_size;
    out.pass_by_pointer = false;
    switch (t) {
        case SIR_TYPE_VOID: out.cls = SLOP_ABI_CLASS_VOID; out.size = out.align = 0; break;
        case SIR_TYPE_BOOL: out.cls = SLOP_ABI_CLASS_INTEGER; out.size = 1; out.align = 1; break;
        case SIR_TYPE_I64: out.cls = SLOP_ABI_CLASS_INTEGER; out.size = 8; out.align = 8; break;
        case SIR_TYPE_F64: out.cls = SLOP_ABI_CLASS_FLOAT; out.size = 8; out.align = 8; break;
        case SIR_TYPE_STRING: out.cls = SLOP_ABI_CLASS_STRING; out.size = 16; out.align = 8; out.pass_by_pointer = true; break;
        case SIR_TYPE_ARRAY: out.cls = SLOP_ABI_CLASS_ARRAY; out.size = 24; out.align = 8; out.pass_by_pointer = true; break;
        case SIR_TYPE_STRUCT: out.cls = SLOP_ABI_CLASS_STRUCT; out.size = info.pointer_size; out.align = info.pointer_size; out.pass_by_pointer = true; break;
        case SIR_TYPE_PTR: out.cls = SLOP_ABI_CLASS_INTEGER; out.size = info.pointer_size; out.align = info.pointer_size; break;
        case SIR_TYPE_TENSOR: out.cls = SLOP_ABI_CLASS_MEMORY; out.size = info.pointer_size; out.align = info.pointer_size; out.pass_by_pointer = true; break;
    }
    return out;
}

typedef struct {
    uint32_t syscall_write;
    uint32_t syscall_exit;
    const char* number_register;
    const char* arg0_register;
    const char* arg1_register;
    const char* arg2_register;
} SlopSyscallABI;

static inline SlopSyscallABI slop_syscall_abi(SlopTargetKind target) {
    SlopSyscallABI s = {0, 0, "", "", "", ""};
    switch (target) {
        case SLOP_TARGET_X86_64_LINUX:
        case SLOP_TARGET_X86_64_LINUX_ELF:
            s.syscall_write = 1; s.syscall_exit = 60; s.number_register="rax"; s.arg0_register="rdi"; s.arg1_register="rsi"; s.arg2_register="rdx"; break;
        case SLOP_TARGET_AARCH64_LINUX:
            s.syscall_write = 64; s.syscall_exit = 93; s.number_register="x8"; s.arg0_register="x0"; s.arg1_register="x1"; s.arg2_register="x2"; break;
        case SLOP_TARGET_ARMV7_LINUX:
            s.syscall_write = 4; s.syscall_exit = 1; s.number_register="r7"; s.arg0_register="r0"; s.arg1_register="r1"; s.arg2_register="r2"; break;
        case SLOP_TARGET_RISCV64_LINUX:
            s.syscall_write = 64; s.syscall_exit = 93; s.number_register="a7"; s.arg0_register="a0"; s.arg1_register="a1"; s.arg2_register="a2"; break;
        default:
            break;
    }
    return s;
}

#endif // SLOP_RUNTIME_ABI_H
