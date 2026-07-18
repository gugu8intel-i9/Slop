# Slop IR (SIR)

Slop IR (SIR) is the new shared intermediate representation for Slop's backend pipeline.

The goal is to stop every backend from becoming its own compiler. The long-term pipeline is:

```text
.slop source
  -> lexer/parser
  -> typed AST
  -> SIR: Slop Intermediate Representation
  -> optimizers
  -> backend:
       C
       x86_64 Linux
       AArch64 / ARM64 Linux
       ARMv7 Linux
       RISC-V64 Linux
       Windows COFF
       macOS Mach-O
       WebAssembly/WASI
```

## Phase 1 completion

Phase 1 now includes:

- expanded full-language opcode vocabulary
- typed values through instruction result types and lowering helpers
- function/block IR operations
- single-pass verifier (`sir_verify_module`)
- side-effect and terminator classification
- stable FNV-1a module fingerprinting
- textual `.sir` writer/loader with escaped string data
- MVP SIR -> C backend emitter
- native backend validation before assembly emission

This gives Slop a real backend contract: C/native/WASM/object emitters can consume SIR instead of duplicating frontend logic.

## What exists now

`slop_ir.h` provides the first compact IR module:

- linear instruction stream
- string pool
- 32-bit ids for compact value/string references
- backend-neutral opcodes
- typed operation slots
- module-level lifetime management
- IR dump helper for debugging

`slop_native_backend.c` now lowers the native-backend MVP subset into SIR first, then emits target assembly from SIR. That makes the native backend a true two-step path:

```text
Slop subset -> SIR -> target assembly
```

Current SIR-backed native subset:

```slop
print("literal")
print(123)
let n = 123
let s = "literal"
print(n)
print(s)
```

## Performance design

SIR is designed for high-performance compilation and high-performance output:

1. **Dense linear instructions**
   - instructions are fixed-size C structs
   - cache-friendly sequential traversal
   - easy to batch optimize and emit

2. **Small ids**
   - values, blocks, and string constants use `uint32_t` ids
   - smaller memory footprint than pointer-heavy ASTs

3. **Arena-friendly lifecycle**
   - build module
   - run optimization passes
   - emit target code
   - release module as a unit

4. **Explicit side effects**
   - print, calls, arena save/restore, bounds checks, and returns are explicit ops
   - enables dead-code elimination without deleting required effects

5. **Backend-neutral lowering**
   - no x86, ARM, or RISC-V register names in SIR
   - target-specific work happens after IR selection/lowering

6. **Optimization-ready**
   - constant folding
   - dead code elimination
   - bounds-check elimination
   - inlining
   - escape analysis
   - arena lifetime compression
   - target-specific instruction selection

## Near-term IR expansion

The next SIR milestones are:

- typed values and locals
- integer/floating arithmetic
- comparisons
- labels and basic blocks
- conditional branches
- while/if lowering
- function call/return lowering
- stack frame description
- ABI argument/return classification
- SEAA arena operations
- array/string runtime operations
- bounds-check operations
- struct field load/store
- C ABI interop calls

## Long-term vision

SIR should become Slop's universal backend contract:

```text
Frontend correctness once.
Optimizations once.
Many backends.
```

The C backend remains the compatibility backend. Native backends become the high-performance direct targets.
