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

## Phase 4: Native backend maturity

Goal: move from assembly MVP to serious native codegen.

- [ ] Instruction selection per target
- [ ] Virtual registers
- [ ] Liveness analysis
- [ ] Register allocation
- [ ] Spill/reload
- [ ] Stack frame layout
- [ ] Calling convention lowering
- [ ] Debug-friendly assembly comments
- [ ] Peephole optimizers
- [ ] Target-specific fast paths

Targets:

- [x] x86_64 Linux assembly MVP
- [x] ARM64/AArch64 Linux assembly MVP
- [x] ARMv7 Linux assembly MVP
- [x] RISC-V64 Linux assembly MVP
- [ ] x86_64 Linux full ABI
- [ ] ARM64 Linux full ABI
- [ ] ARMv7 Linux full ABI
- [ ] RISC-V64 Linux full ABI
- [ ] Windows x64 COFF
- [ ] macOS x86_64 Mach-O
- [ ] macOS ARM64 Mach-O
- [ ] WebAssembly/WASI

---

## Phase 5: Object files and linking

Goal: emit linkable artifacts directly.

- [ ] ELF relocatable `.o`
- [ ] COFF `.obj`
- [ ] Mach-O `.o`
- [ ] WASM modules
- [ ] symbol tables
- [ ] relocations
- [ ] static data sections
- [ ] debug sections
- [ ] use `lld`/system linker initially
- [ ] optional Slop linker later

---

## Phase 6: Runtime and ABI compatibility

Goal: make native Slop compatible with real-world libraries and OSes.

- [ ] Native SEAA runtime ABI
- [ ] string ABI
- [ ] array ABI
- [ ] struct layout ABI
- [ ] C calling convention interop
- [ ] C++ bridge compatibility
- [ ] Rust bridge compatibility
- [ ] OS syscall wrappers
- [ ] libc interop mode
- [ ] no-libc syscall-only mode

---

## Phase 7: Tooling and tests

Goal: make it reliable.

- [ ] `slop build --target ...`
- [ ] `slop emit-ir file.slop`
- [ ] `slop emit-asm --target ...`
- [ ] backend test snapshots
- [ ] IR verifier tests
- [ ] C backend vs native backend output comparison
- [ ] QEMU target smoke tests where available
- [ ] CI matrix for Linux/macOS/Windows
- [ ] benchmark suite for C backend vs native backend

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
