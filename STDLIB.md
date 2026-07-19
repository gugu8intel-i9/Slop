# Slop Standard Library Plan

Slop's standard library should stay tiny, obvious, and native-fast.

## Packages

```text
std/io       print, files, streams
std/fs       paths, directories, metadata
std/net      sockets, HTTP helpers
std/math     integers, floats, vectors
std/array    array helpers, map/filter/fold
std/string   split, join, replace, parse
std/sync     atomics, spawn, channels
std/hw       optional unsafe hardware APIs
std/gpu      GPU/device buffers and fences
std/simd     explicit SIMD/vector hooks
std/test     tests and benchmarks
```

## Design rule

Beginners should only need:

```slop
print("hello")
let xs = [1, 2, 3]
```

Advanced users can opt into packages like:

```slop
std/hw
std/gpu
std/simd
```

## Performance rule

Stdlib abstractions must lower to SIR, not opaque runtime calls, whenever possible. That allows:

- inlining
- fusion
- bounds-check elimination
- arena lifetime compression
- native backend lowering
