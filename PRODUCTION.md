# Slop Production Surface

This file defines what is currently treated as production-gated, not aspirational.

## Production-gated now

The gate is:

```bash
tools/production_gate.sh
```

It verifies:

1. prebuilt compiler checksums
2. self-hosting without Python
3. stage1/stage2 self-host fixpoint
4. SIR production pipeline to C and direct ELF
5. portable C backend examples
5. unsafe low-level demo
6. direct x86_64 ELF executable backend for the supported native subset
7. SIR-first `.slop -> C/ELF` pipeline subset plus `.slop -> SIR -> C` core control-flow subset
8. Phase 4-7 smoke test: SIR, optimizer, regalloc, ABI, direct ELF

## Supported production surface today

- Linux x86_64 prebuilt `slop-compiler`
- self-hosting compiler source `compiler.slop`
- portable C backend for normal programs
- direct x86_64 ELF backend for the current syscall/native subset
- SIR verification/optimization infrastructure
- optional unsafe low-level API through explicit `unsafe_*`, `mmio_*`, `cpu_*`, `gpu_*`, `ram_*` calls
- standard library starter packages
- production gate in CI

## Not falsely claimed

These are still engineering work, not fake-complete claims:

- full native backend for every SIR op
- full object emission for every platform
- complete LSP/editor server
- full package registry
- full Slop-written replacement for every helper script
- own linker for every object format

The rule is: if it is not covered by the production gate, it is not claimed as production-ready.
