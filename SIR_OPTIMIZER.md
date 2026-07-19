# SIR Optimizer

Phase 3 adds the shared high-performance optimizer layer for Slop IR.

The optimizer is intentionally backend-independent:

```text
Slop -> SIR -> optimizer -> C/native/WASM/object backend
```

## Implemented passes

`slop_sir_optimizer.h` implements:

- constant folding
- algebraic simplification
- copy propagation
- common subexpression elimination
- side-effect-safe dead pure value cleanup
- NOP compaction
- duplicate bounds-check elimination
- empty SEAA arena scope compression
- parallel-safety classification

## Why this is novel for Slop

Most optimizers treat memory as generic heap state. Slop keeps its unique SEAA arena model explicit in IR:

```text
arena.save
arena.alloc
arena.restore
```

That lets Slop optimize lifetimes directly instead of guessing through malloc/free-like calls.

Bounds checks are also explicit:

```text
bounds.check index length
```

So Slop can keep safety by default but remove repeated/proven checks when safe.

## Safety rule

The optimizer may remove or rewrite pure value code, but it must keep side-effect operations anchored:

- print
- read/write file
- calls
- raw emit
- bounds checks unless proven duplicate/redundant
- arena operations unless proven empty
- returns/exits
- array/field mutation
- parallel/tensor hooks

## Main API

```c
SIROptStats stats = sir_opt_phase3_high_performance(&module);
```

Stats include:

```c
constants_folded
algebraic_simplified
cse_eliminated
copies_propagated
pure_nops_inserted
nops_compacted
bounds_checks_removed
arena_scopes_compressed
```

## Example

Input pattern:

```text
40 + 2
2 + 40
x + 0
duplicate bounds.check
dead pure multiply
empty arena save/restore
```

Optimized result:

```text
const.i64 42
one surviving bounds.check
side effects preserved
empty arena scope removed
dead pure value removed
```

Verified optimizer stats from the Phase-3 test:

```text
folded=4 algebra=0 cse=3 copies=0 dce=3 compact=9 bounds=1 arena=1 parallel_safe=1
```

## Next optimizer expansions

- loop-invariant code motion on CFG-backed SIR
- function inlining once full function bodies are emitted as SIR
- stronger bounds proof engine using loop facts
- string/array fusion across comprehension pipelines
- target vectorization hints
- target-specific cost models
