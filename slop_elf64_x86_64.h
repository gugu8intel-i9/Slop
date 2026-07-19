#ifndef SLOP_ELF64_X86_64_H
#define SLOP_ELF64_X86_64_H

// Minimal direct ELF64 emitter for SIR's syscall subset.
// This is the first no-assembler/no-linker backend path:
//   Slop subset -> SIR -> ELF64 executable bytes
//
// Target: Linux x86_64, static ET_EXEC, direct write/exit syscalls.

#include "slop_ir.h"
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t* data;
    size_t len;
    size_t cap;
} SlopByteBuf;

static inline void slop_bb_reserve(SlopByteBuf* b, size_t extra) {
    if (b->len + extra <= b->cap) return;
    size_t nc = b->cap ? b->cap * 2 : 256;
    while (nc < b->len + extra) nc *= 2;
    uint8_t* p = (uint8_t*)realloc(b->data, nc);
    if (!p) { fprintf(stderr, "ELF emitter: out of memory\n"); exit(1); }
    b->data = p;
    b->cap = nc;
}

static inline void slop_bb_u8(SlopByteBuf* b, uint8_t v) { slop_bb_reserve(b, 1); b->data[b->len++] = v; }
static inline void slop_bb_i32(SlopByteBuf* b, int32_t v) { slop_bb_reserve(b, 4); memcpy(b->data + b->len, &v, 4); b->len += 4; }
static inline void slop_bb_u32(SlopByteBuf* b, uint32_t v) { slop_bb_reserve(b, 4); memcpy(b->data + b->len, &v, 4); b->len += 4; }
static inline void slop_bb_bytes(SlopByteBuf* b, const void* p, size_t n) { slop_bb_reserve(b, n); memcpy(b->data + b->len, p, n); b->len += n; }

static inline void slop_x64_mov_rax_imm32(SlopByteBuf* c, uint32_t imm) { slop_bb_u8(c,0x48); slop_bb_u8(c,0xc7); slop_bb_u8(c,0xc0); slop_bb_u32(c,imm); }
static inline void slop_x64_mov_rdi_imm32(SlopByteBuf* c, uint32_t imm) { slop_bb_u8(c,0x48); slop_bb_u8(c,0xc7); slop_bb_u8(c,0xc7); slop_bb_u32(c,imm); }
static inline void slop_x64_mov_rdx_imm32(SlopByteBuf* c, uint32_t imm) { slop_bb_u8(c,0x48); slop_bb_u8(c,0xc7); slop_bb_u8(c,0xc2); slop_bb_u32(c,imm); }
static inline size_t slop_x64_lea_rsi_rip_patch(SlopByteBuf* c) { slop_bb_u8(c,0x48); slop_bb_u8(c,0x8d); slop_bb_u8(c,0x35); size_t p=c->len; slop_bb_i32(c,0); return p; }
static inline void slop_x64_syscall(SlopByteBuf* c) { slop_bb_u8(c,0x0f); slop_bb_u8(c,0x05); }

static inline int sir_emit_elf64_x86_64(FILE* out, const SIRModule* m) {
    const uint64_t base = 0x400000;
    const size_t header_size = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    SlopByteBuf code = {0};
    SlopByteBuf data = {0};

    // Build data pool and remember offsets.
    uint32_t* str_off = (uint32_t*)calloc(m->string_len ? m->string_len : 1, sizeof(uint32_t));
    if (!str_off) return 1;
    for (uint32_t i = 0; i < m->string_len; i++) {
        str_off[i] = (uint32_t)data.len;
        slop_bb_bytes(&data, m->strings[i].data, m->strings[i].len);
    }

    typedef struct { size_t disp_pos; uint32_t string_id; size_t next_ip; } Patch;
    Patch* patches = NULL;
    size_t patch_len = 0, patch_cap = 0;
#define ADD_PATCH(pos, sid, next) do { \
    if (patch_len == patch_cap) { patch_cap = patch_cap ? patch_cap * 2 : 16; patches = (Patch*)realloc(patches, patch_cap * sizeof(Patch)); if (!patches) exit(1); } \
    patches[patch_len++] = (Patch){ (pos), (sid), (next) }; \
} while (0)

    for (uint32_t i = 0; i < m->inst_len; i++) {
        const SIRInst* inst = &m->insts[i];
        if (inst->op == SIR_OP_PRINT_STRING) {
            SIRId sid = inst->a;
            for (uint32_t j = 0; j < m->inst_len; j++) {
                if (m->insts[j].dst == inst->a && m->insts[j].op == SIR_OP_CONST_STRING) { sid = m->insts[j].a; break; }
            }
            if (sid < m->string_len) {
                slop_x64_mov_rax_imm32(&code, 1);       // write
                slop_x64_mov_rdi_imm32(&code, 1);       // stdout
                size_t disp = slop_x64_lea_rsi_rip_patch(&code);
                ADD_PATCH(disp, sid, code.len);
                slop_x64_mov_rdx_imm32(&code, m->strings[sid].len);
                slop_x64_syscall(&code);
            }
        } else if (inst->op == SIR_OP_EXIT) {
            slop_x64_mov_rax_imm32(&code, 60);      // exit
            slop_bb_u8(&code, 0x48); slop_bb_u8(&code, 0x31); slop_bb_u8(&code, 0xff); // xor rdi,rdi
            slop_x64_syscall(&code);
        }
    }
    if (code.len == 0 || code.data[code.len - 1] != 0x05) {
        slop_x64_mov_rax_imm32(&code, 60);
        slop_bb_u8(&code, 0x48); slop_bb_u8(&code, 0x31); slop_bb_u8(&code, 0xff);
        slop_x64_syscall(&code);
    }

    size_t data_file_off = header_size + code.len;
    for (size_t i = 0; i < patch_len; i++) {
        uint64_t str_va = base + data_file_off + str_off[patches[i].string_id];
        uint64_t next_va = base + header_size + patches[i].next_ip;
        int64_t delta = (int64_t)str_va - (int64_t)next_va;
        int32_t disp = (int32_t)delta;
        memcpy(code.data + patches[i].disp_pos, &disp, 4);
    }

    Elf64_Ehdr eh;
    memset(&eh, 0, sizeof(eh));
    eh.e_ident[EI_MAG0] = ELFMAG0;
    eh.e_ident[EI_MAG1] = ELFMAG1;
    eh.e_ident[EI_MAG2] = ELFMAG2;
    eh.e_ident[EI_MAG3] = ELFMAG3;
    eh.e_ident[EI_CLASS] = ELFCLASS64;
    eh.e_ident[EI_DATA] = ELFDATA2LSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_ident[EI_OSABI] = ELFOSABI_SYSV;
    eh.e_type = ET_EXEC;
    eh.e_machine = EM_X86_64;
    eh.e_version = EV_CURRENT;
    eh.e_entry = base + header_size;
    eh.e_phoff = sizeof(Elf64_Ehdr);
    eh.e_ehsize = sizeof(Elf64_Ehdr);
    eh.e_phentsize = sizeof(Elf64_Phdr);
    eh.e_phnum = 1;

    Elf64_Phdr ph;
    memset(&ph, 0, sizeof(ph));
    ph.p_type = PT_LOAD;
    ph.p_flags = PF_R | PF_X;
    ph.p_offset = 0;
    ph.p_vaddr = base;
    ph.p_paddr = base;
    ph.p_filesz = header_size + code.len + data.len;
    ph.p_memsz = ph.p_filesz;
    ph.p_align = 0x1000;

    fwrite(&eh, 1, sizeof(eh), out);
    fwrite(&ph, 1, sizeof(ph), out);
    fwrite(code.data, 1, code.len, out);
    fwrite(data.data, 1, data.len, out);

    free(str_off);
    free(patches);
    free(code.data);
    free(data.data);
    return ferror(out) ? 1 : 0;
#undef ADD_PATCH
}

#endif // SLOP_ELF64_X86_64_H
