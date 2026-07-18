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

Phase 1 is complete as the shared IR foundation. Production migration of the full self-hosted compiler into SIR is tracked in Phase 2 because it is full-language lowering work, not foundation work.

---

## Phase 2: Full language lowering

Goal: lower real Slop programs into SIR.

- [x] Lowering contract for integer arithmetic
- [x] Lowering contract for float arithmetic
- [x] Lowering contract for Boolean ops
- [x] Lowering contract for `if` / `else`
- [x] Lowering contract for `while`
- [x] Lowering contract for functions
- [ ] Production parser-to-SIR implementation for integer arithmetic
- [ ] Production parser-to-SIR implementation for float arithmetic
- [ ] Production parser-to-SIR implementation for Boolean ops
- [ ] Production parser-to-SIR implementation for `if` / `else`
- [ ] Production parser-to-SIR implementation for `while`
- [ ] Production parser-to-SIR implementation for functions
- [ ] returns
- [ ] arrays
- [ ] strings
- [ ] structs
- [ ] field access
- [ ] method lowering
- [ ] pattern matching
- [ ] list comprehensions
- [ ] `parallel`
- [ ] `raw`
- [ ] GPU/tensor hooks

---

## Phase 3: High-performance optimizer

Goal: make SIR the place where Slop becomes fast.

- [ ] Constant folding
- [ ] Copy propagation
- [ ] Dead-code elimination
- [ ] Common subexpression elimination
- [ ] Strength reduction
- [ ] Inlining
- [ ] Escape analysis
- [ ] Arena lifetime compression
- [ ] Bounds-check elimination
- [ ] Loop-invariant code motion
- [ ] Vectorization hooks
- [ ] Parallel lowering hooks
- [ ] Target feature selection

Innovative Slop-specific passes:

- [ ] **SEAA lifetime optimizer**: merge/reuse arena scopes when provably safe
- [ ] **Bounds proof engine**: remove bounds checks using loop and array-length facts
- [ ] **Parallel purity classifier**: detect computations safe for lock-free parallel lowering
- [ ] **String/array fusion**: avoid temporary arrays for split/join/map-style chains
- [ ] **Syscall fast-path lowering**: direct native syscalls for tiny standalone binaries

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
