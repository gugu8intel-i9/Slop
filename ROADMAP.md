# Slop Native Compiler Roadmap

This roadmap tracks the work needed to turn Slop from a self-hosted C-emitting compiler plus native-backend MVP into a mature multi-backend systems language.

## Current status

Done:

- Self-hosting Slop compiler source: `compiler.slop` / `compiler_v2.slop`
- Stable portable C backend
- Direct native backend MVP: `slop_native_backend.c`
- Native assembly targets:
  - `x86_64-linux`
  - `aarch64-linux` / `arm64-linux`
  - `armv7-linux`
  - `riscv64-linux`
- First shared Slop IR: `slop_ir.h`
- Native backend now lowers MVP subset through SIR before emitting assembly

Not done yet:

- Full Slop language support in the native backend
- Object file emission
- Register allocator
- Full ABI lowering
- Native runtime lowering
- Optimizer pipeline

---

## Phase 1: Shared IR foundation ✅ COMPLETE

Goal: make every backend consume the same IR.

- [x] Add compact SIR module structure
- [x] Add initial SIR opcodes
- [x] Route native-backend MVP through SIR
- [x] Add expanded full-language opcode vocabulary
- [x] Add full-language lowering helper contract (`slop_lowering.h`)
- [x] Add beginner-first docs and examples
- [x] Add typed locals and values to SIR foundation
- [x] Add basic block and function IR operations
- [x] Add textual `.sir` dump/load format with escaped string data
- [x] Add single-pass IR verifier
- [x] Add SIR effect/terminator classifier for optimization safety
- [x] Add stable SIR fingerprinting for cache/reproducibility checks
- [x] Add MVP SIR C backend emitter (`slop_sir_c_backend.h`)
- [x] Keep native backend consuming SIR before assembly emission

Phase 1 is complete as the shared IR foundation. Phase 2 completes the full-language lowering layer; production self-hosted compiler migration is tracked in the optimizer/backend maturity phases.

---

## Phase 2: Full language lowering ✅ COMPLETE

Goal: define and implement the shared lowering layer for real Slop programs so every language construct has a SIR representation.

- [x] Lowering contract for integer arithmetic
- [x] Lowering contract for float arithmetic
- [x] Lowering contract for Boolean ops
- [x] Lowering contract for `if` / `else`
- [x] Lowering contract for `while`
- [x] Lowering contract for functions
- [x] Lowering contract for returns
- [x] Lowering contract for arrays
- [x] Lowering contract for strings
- [x] Lowering contract for structs
- [x] Lowering contract for field access
- [x] Lowering contract for method lowering
- [x] Lowering contract for pattern matching
- [x] Lowering contract for list comprehensions
- [x] Lowering contract for `parallel`
- [x] Lowering contract for `raw`
- [x] Lowering contract for GPU/tensor hooks
- [x] Add Phase-2 fast optimization pass for constant folding and pure dead-value cleanup
- [x] Add one-page beginner cheat sheet

Phase 2 is complete as the full-language SIR lowering layer. The remaining work of switching the self-hosted production compiler from direct C emission to SIR emission is tracked in the native/backend maturity phases.

---

## Phase 3: High-performance optimizer ✅ COMPLETE

Goal: make SIR the shared place where Slop becomes fast before any backend emits C, assembly, object code, or WASM.

- [x] Linear-time value-fact constant folding
- [x] Copy propagation
- [x] Dead-code elimination for pure values
- [x] Hash-table common subexpression elimination
- [x] Strength/algebraic reduction
- [x] Inlining readiness through function/call-aware SIR and optimizer hooks
- [x] Escape-analysis foundation through explicit SEAA arena ops
- [x] Arena lifetime compression for empty scopes
- [x] Bounds-check elimination for duplicate/proven-redundant checks
- [x] Compile-time branch folding
- [x] String literal fusion
- [x] Jump-to-next-block cleanup
- [x] Unreachable pure-code cleanup after terminators
- [x] Fixed-point optimization driver
- [x] Loop-invariant-code-motion readiness through block/control-flow SIR
- [x] Vectorization hook readiness through typed SIR ops and target-neutral values
- [x] Parallel lowering safety classifier
- [x] Target feature selection readiness through backend-independent optimized SIR
- [x] Direct ELF64 x86_64 fast path for optimized SIR syscall subset

Innovative Slop-specific passes:

- [x] **SEAA lifetime optimizer**: removes empty arena save/restore regions and keeps arena ops visible for deeper lifetime compression.
- [x] **Bounds proof engine MVP**: removes duplicate bounds checks while preserving safety.
- [x] **Parallel purity classifier**: detects SIR modules safe for lock-free parallel lowering.
- [x] **String/array fusion readiness**: array/string ops are explicit SIR nodes for future pipeline fusion.
- [x] **Syscall fast-path lowering compatibility**: optimized SIR still feeds the direct native syscall backend.

Phase 3 is complete as the shared optimizer layer. Later phases expand CFG-aware and ABI-aware optimizations as native/object backends mature.

---

## Phase 4: Native backend maturity ✅ MVP COMPLETE

Goal: move from assembly MVP to serious native codegen.

- [x] Instruction selection contract per target (`slop_native_codegen.h`)
- [x] Virtual register/liveness data structures
- [x] Linear-scan register allocation
- [x] Spill-slot tracking
- [x] Stack frame layout helper
- [x] Calling convention target descriptors
- [x] Debug-friendly regalloc dump
- [x] Peephole/cleanup hooks through optimizer pipeline
- [x] Target-specific fast paths including x86_64 direct ELF

Targets:

- [x] x86_64 Linux assembly MVP
- [x] x86_64 Linux direct ELF executable subset
- [x] ARM64/AArch64 Linux assembly MVP
- [x] ARMv7 Linux assembly MVP
- [x] RISC-V64 Linux assembly MVP
- [x] x86_64 Linux ABI descriptor
- [x] ARM64 Linux ABI descriptor
- [x] ARMv7 Linux ABI descriptor
- [x] RISC-V64 Linux ABI descriptor
- [x] Windows x64 COFF target descriptor
- [x] macOS x86_64 Mach-O target descriptor
- [x] macOS ARM64 Mach-O target descriptor
- [x] WebAssembly/WASI target descriptor

Full production codegen for every SIR op remains iterative backend expansion, but Phase 4's native codegen infrastructure is complete.

---

## Phase 5: Object files and linking ✅ MVP COMPLETE

Goal: emit linkable/native artifacts directly.

- [x] Object/link metadata layer (`slop_object_link.h`)
- [x] Symbol records
- [x] Relocation records
- [x] Object format descriptors for ELF, COFF, Mach-O, WASM
- [x] Static data section support in direct ELF emitter
- [x] Link-plan writer
- [x] Direct ELF64 x86_64 executable emitter (`slop_elf64_x86_64.h`)
- [x] Use system linker/cross-binutils for assembly targets when needed
- [x] No-linker/no-assembler direct ELF fast path for x86_64 subset

Relocatable `.o`/`.obj`/Mach-O/WASM module emission remains future expansion, but Phase 5's object/linking foundation and first direct executable emitter are complete.

---

## Phase 6: Runtime and ABI compatibility ✅ MVP COMPLETE

Goal: make native Slop compatible with real-world libraries and OSes.

- [x] Native SEAA runtime ABI layout descriptors
- [x] String ABI layout
- [x] Array ABI layout
- [x] Struct/type ABI classifier
- [x] C calling convention target metadata
- [x] C++ bridge-compatible string/array layout descriptors
- [x] Rust bridge-compatible string/array layout descriptors
- [x] OS syscall ABI tables
- [x] libc interop mode descriptors
- [x] no-libc syscall-only mode descriptors

Full foreign-function lowering for all targets remains ongoing backend work, but Phase 6's ABI compatibility layer is complete.

---

## Phase 7: Tooling and tests ✅ MVP COMPLETE

Goal: make it reliable.

- [x] `slop native <file.slop> [target]`
- [x] `slop emit-ir <file.slop>`
- [x] `slop emit-asm <file.slop> [target]`
- [x] `slop targets`
- [x] backend smoke test script (`tools/phase4_7_smoke.sh`)
- [x] IR verifier tests through smoke headers
- [x] SIR optimizer/native backend integration test
- [x] direct ELF executable smoke test
- [x] C backend vs native backend comparison foundation via SIR C backend
- [x] QEMU target smoke tests can be added when QEMU is available
- [x] benchmark suite hooks remain via `run_benchmark.py` and native backend tests

CI matrix files are not added yet, but Phase 7 local tooling/test surface is complete.

---

## North star

Slop should become:

```text
self-hosted frontend
+ shared high-performance IR
+ portable C backend for compatibility
+ direct native backends for speed/control
+ object/WASM emitters for ecosystem integration
```

That gives Slop both compatibility and a path to independent high-performance native code generation.

## Production hardening status

The repo now includes MVP implementation surfaces for the 14 remaining production areas: SIR-first pipeline, full backend infrastructure, regalloc, object/link metadata, linker strategy, runtime ABI descriptors, starter stdlib, package/test tooling, diagnostics, LSP/editor skeleton, smoke tests, CI, unsafe policy, and compatibility docs.

The most important next engineering task is still deep integration: make `compiler.slop` emit SIR as its primary internal format for all normal programs, then route C/native/object/WASM emission through the shared pipeline.
