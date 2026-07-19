# SIR Optimizer

Phase 3 adds the shared high-performance optimizer layer for Slop IR.

The optimizer is intentionally backend-independent:

```text
Slop -> SIR -> optimizer -> C/native/WASM/object backend
```

## Implemented passes

`slop_sir_optimizer.h` implements:

- linear-time constant folding with value facts
- algebraic simplification / strength reduction
- copy propagation
- hash-table common subexpression elimination, including commutative expressions
- string literal fusion (`"a" + "b" -> "ab"`)
- branch folding when conditions are compile-time constants
- side-effect-safe dead pure value cleanup
- NOP compaction
- duplicate bounds-check elimination
- proven constant bounds-check elimination
- empty SEAA arena scope compression
- fixed-point optimization rounds
- jump-to-next-block cleanup
- unreachable pure-code cleanup after terminators
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
proven_bounds_removed
branches_folded
strings_fused
arena_scopes_compressed
fixed_point_rounds
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

Additional Phase-3 completion tests:

```text
folded=1 strings=1 branches=1 proven_bounds=1 rounds=2
fold=3 cse=1 str=1 br=1 proven=1 jump=0 unreach=1 compact=8 rounds=2
```
```

## Next optimizer expansions

- deeper loop-invariant code motion on CFG-backed SIR
- function inlining once full function bodies are emitted as SIR
- stronger bounds proof engine using loop facts and array length facts
- string/array fusion across comprehension pipelines
- target vectorization hints
- target-specific cost models

## Performance model

The optimizer now avoids obvious quadratic behavior in hot passes:

- constant folding uses a value-id fact table instead of rescanning the whole module for every operand
- CSE uses an open-addressed hash table instead of pairwise comparison for every instruction
- DCE uses dense bitsets keyed by compact SIR value IDs
- compaction is one linear write pass

This keeps optimization fast while preserving deterministic output and small implementation size.
