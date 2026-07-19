# Slop Prebuilt Stage0 Binaries

This directory ships trusted prebuilt Slop binaries so normal installation does not need Python or GCC just to get `slop-compiler`.

Currently included:

```text
linux-x86_64/slop-compiler
linux-x86_64/slop-native-backend
linux-x86_64/slop-pipeline
linux-x86_64/SHA256SUMS
```

## Why this exists

Slop is self-hosting, but a brand-new machine still needs a first compiler binary. Before this directory existed, install had to run:

```text
compiler.slop -> slop_boot.py -> compiler.c -> gcc -> slop-compiler
```

Now Linux x86_64 installs use:

```text
prebuilt slop-compiler -> ready immediately
```

Python/GCC are only fallback requirements when a platform does not have a prebuilt binary yet.

## Trust model

The prebuilt `slop-compiler` was produced from the self-hosting chain and verified with the stage1/stage2 C fixpoint before being copied here.

Checksums are stored in `SHA256SUMS`.

## Still needed later

For the stable portable C backend, users still need GCC/Clang to turn emitted C into a native executable. The experimental `slop native` path uses the direct native backend subset and assembler/linker tools.

Future goal: full native backend + object emission so ordinary Slop programs no longer need GCC/Clang.
