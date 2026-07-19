#ifndef SLOP_OBJECT_LINK_H
#define SLOP_OBJECT_LINK_H

// Phase 5 object/linking infrastructure.
// Defines common symbol/relocation/link-plan metadata shared by future ELF,
// COFF, Mach-O, and WASM emitters. The current direct ELF executable backend is
// wired through slop_elf64_x86_64.h; relocatable objects build on these records.

#include "slop_native_codegen.h"
#include "slop_elf64_x86_64.h"

typedef enum {
    SLOP_OBJ_ELF64,
    SLOP_OBJ_ELF32,
    SLOP_OBJ_COFF,
    SLOP_OBJ_MACHO64,
    SLOP_OBJ_WASM
} SlopObjectFormat;

typedef enum {
    SLOP_RELOC_ABS64,
    SLOP_RELOC_ABS32,
    SLOP_RELOC_PC32,
    SLOP_RELOC_CALL26,
    SLOP_RELOC_RISCV_HI20,
    SLOP_RELOC_RISCV_LO12
} SlopRelocKind;

typedef struct {
    const char* name;
    uint64_t value;
    uint32_t section;
    bool global;
} SlopSymbol;

typedef struct {
    SlopRelocKind kind;
    uint32_t section;
    uint64_t offset;
    uint32_t symbol;
    int64_t addend;
} SlopReloc;

typedef struct {
    SlopObjectFormat format;
    SlopTargetKind target;
    SlopSymbol* symbols;
    uint32_t symbol_count;
    SlopReloc* relocs;
    uint32_t reloc_count;
    const char* entry_symbol;
} SlopObjectPlan;

static inline const char* slop_object_format_name(SlopObjectFormat f) {
    switch (f) {
        case SLOP_OBJ_ELF64: return "ELF64";
        case SLOP_OBJ_ELF32: return "ELF32";
        case SLOP_OBJ_COFF: return "COFF";
        case SLOP_OBJ_MACHO64: return "Mach-O 64";
        case SLOP_OBJ_WASM: return "WASM";
    }
    return "unknown";
}

static inline SlopObjectFormat slop_default_object_format(SlopTargetKind target) {
    switch (target) {
        case SLOP_TARGET_ARMV7_LINUX: return SLOP_OBJ_ELF32;
        case SLOP_TARGET_WINDOWS_X64: return SLOP_OBJ_COFF;
        case SLOP_TARGET_MACOS_X86_64:
        case SLOP_TARGET_MACOS_ARM64: return SLOP_OBJ_MACHO64;
        case SLOP_TARGET_WASM32_WASI: return SLOP_OBJ_WASM;
        default: return SLOP_OBJ_ELF64;
    }
}

static inline void slop_write_link_plan(FILE* out, SlopTargetKind target) {
    SlopTargetInfo info = slop_target_info(target);
    fprintf(out, "slop.link.plan {\n");
    fprintf(out, "  target = %s\n", info.name);
    fprintf(out, "  abi = %s\n", info.abi);
    fprintf(out, "  object = %s\n", slop_object_format_name(slop_default_object_format(target)));
    fprintf(out, "  entry = _start\n");
    fprintf(out, "}\n");
}

static inline int slop_emit_direct_executable(FILE* out, const SIRModule* m, SlopTargetKind target) {
    if (target == SLOP_TARGET_X86_64_LINUX_ELF) return sir_emit_elf64_x86_64(out, m);
    fprintf(stderr, "direct executable emitter for target is not implemented yet\n");
    return 1;
}

#endif // SLOP_OBJECT_LINK_H
