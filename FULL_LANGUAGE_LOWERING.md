# Full Slop Language Lowering to SIR

This document defines how the full Slop language lowers into **SIR** (Slop Intermediate Representation).

The goal is simple:

```text
Slop syntax stays easy.
Compiler lowering becomes shared.
Every backend gets the same optimized IR.
```

## Pipeline

```text
.slop source
  -> tokens
  -> parsed AST
  -> typed AST
  -> SIR lowering
  -> SIR optimization pipeline
  -> backend emission
       C backend
       x86_64 backend
       ARM64 backend
       ARMv7 backend
       RISC-V64 backend
       WASM/object backends
```

## Current implementation status

Implemented now:

- `slop_ir.h`: expanded SIR op vocabulary for the whole language family
- `slop_lowering.h`: first full-language lowering helper API
- `slop_native_backend.c`: MVP native backend lowers through SIR before target assembly
- `sir` dump mode in the native backend

Still being wired into the self-hosted compiler:

- replacing direct C-string emission with SIR emission
- making C backend consume SIR
- making native backends consume the full SIR set

## Lowering map

| Slop construct | SIR lowering |
|---|---|
| `let x = expr` | `expr`, then `local.declare` |
| `x = expr` | `expr`, then `local.set` |
| integer literal | `const.i64` |
| float literal | `const.f64` |
| bool literal | `const.bool` |
| string literal | `const.string` + string pool |
| `a + b` | `add.i64`, `add.f64`, or `string.concat` by type |
| `a - b` | `sub.i64` / `sub.f64` |
| `a * b` | `mul.i64` / `mul.f64` |
| `a / b` | `div.i64` / `div.f64` |
| `a % b` | `mod.i64` |
| comparisons | `cmp.*` |
| `if cond { ... } else { ... }` | `branch`, blocks, joins, optional `phi` |
| `while cond { ... }` | condition block, body block, exit block |
| function declaration | `function.begin`, param locals, body, `function.end` |
| function call | `call` |
| `return expr` | `return` |
| arrays | `array.new`, `array.push`, `array.get`, `array.set`, `array.len` |
| array bounds | `bounds.check` before indexed access |
| strings | `string.concat`, `string.len`, `char.at` |
| structs | `struct.new`, `field.get`, `field.set` |
| methods | regular functions with lowered `this` parameter |
| pattern matching | branch chain or jump table depending on cases |
| list comprehensions | loop lowering with optional preallocation/fusion |
| `parallel` | `parallel.for` / runtime worker lowering |
| SEAA arena scope | `arena.save`, `arena.alloc`, `arena.restore` |
| file IO | `read.file`, `write.file` effects |
| tensors/GPU | `tensor.op` / backend runtime hook |

## High-performance lowering principles

1. **Typed lowering first**
   - Type decisions happen before backend emission.
   - Backends do not guess whether `+` means integer add or string concat.

2. **Explicit effects**
   - File IO, printing, calls, bounds checks, arena operations, and exits are explicit.
   - Optimizers can safely delete dead pure work without deleting side effects.

3. **Arena-aware IR**
   - SEAA operations are first-class.
   - This enables Slop-specific lifetime optimizations that C/LLVM-style IRs do not naturally expose.

4. **Bounds proof engine**
   - Bounds checks are explicit and can be proven redundant.
   - Example: `while i < length(arr)` allows removing repeated `arr[i]` checks inside the loop.

5. **Fusion-friendly arrays and strings**
   - `split(...).map(...).join(...)` style pipelines can be fused into fewer allocations.
   - List comprehensions can become tight loops.

6. **Parallel purity classifier**
   - Pure operations can be lowered into `parallel.for` without locks.
   - Impure operations remain sequential unless explicitly isolated.

## Easier-than-Python surface rule

The language should remain easy even if the IR is powerful:

```slop
let name = "Gugu"
print("Hello " + name)

let numbers = [1, 2, 3]
let doubled = [x * 2 for x in numbers]
print(doubled[0])
```

Behind the scenes this becomes typed SIR with explicit allocations, bounds checks, and optimized loops.

Users learn simple rules. The compiler handles the hard parts.
