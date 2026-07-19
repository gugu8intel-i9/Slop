# Optional Unsafe Low-Level Slop

Slop's default mode stays safe and easy. Low-level control is opt-in and explicit.

The rule is:

> If it can touch raw memory, devices, CPU timing, GPU/device fences, or components, its name must make the danger obvious.

That is why these APIs use names like `unsafe_load64`, `unsafe_store64`, `mmio_write32`, `cpu_cycles`, and `gpu_fence`.

## What this is for

Use this only for:

- kernels and hobby OS work
- driver experiments
- embedded/MMIO projects
- allocators and runtime development
- GPU/runtime synchronization
- high-performance instrumentation
- hardware labs

Normal app code should keep using normal Slop arrays, strings, structs, functions, and bounds checks.

## Example

```slop
fn main(args: array[string]) {
    let cell = 0
    let addr = addr_of(cell)

    unsafe_store64(addr, 1337)
    print(unsafe_load64(addr))

    cpu_fence()
    cpu_prefetch(addr)

    ram_zero(addr, 8)
    print(unsafe_load64(addr))
}
```

## APIs

### Addressing

```slop
addr_of(x) -> int
component_base(addr: int) -> int
```

`addr_of` returns the address of a Slop local/value in generated native C.

`component_base` is a zero-cost marker for hardware/component base addresses. It makes driver-style code read clearly:

```slop
let uart = component_base(0x10000000)
```

### Unsafe RAM loads/stores

```slop
unsafe_load8(addr: int) -> int
unsafe_load16(addr: int) -> int
unsafe_load32(addr: int) -> int
unsafe_load64(addr: int) -> int

unsafe_store8(addr: int, value: int)
unsafe_store16(addr: int, value: int)
unsafe_store32(addr: int, value: int)
unsafe_store64(addr: int, value: int)
```

These are volatile raw memory operations. They can crash your program or corrupt memory if the address is wrong.

### MMIO device registers

```slop
mmio_read32(addr: int) -> int
mmio_write32(addr: int, value: int)
```

MMIO operations include full fences so device register access is not reordered across the operation.

### CPU performance/control

```slop
cpu_cycles() -> int
cpu_fence()
cpu_relax()
cpu_prefetch(addr: int)
```

- `cpu_cycles`: low-level cycle counter where supported.
- `cpu_fence`: full memory fence.
- `cpu_relax`: spin-loop hint (`pause`, `yield`, or equivalent).
- `cpu_prefetch`: prefetches a memory address for high-performance loops.

### RAM bulk operations

```slop
ram_copy(dst: int, src: int, bytes: int)
ram_zero(dst: int, bytes: int)
```

Zero-cost wrappers around native memory copy/zero operations.

### Device/GPU synchronization

```slop
device_fence()
gpu_fence()
```

These are explicit synchronization points for code that talks to devices or GPU/runtime buffers.

## Why this is novel

Slop keeps low-level power separate from normal code:

```text
normal Slop: safe, easy, bounds-checked
unsafe Slop: explicit hardware control
```

The compiler can keep normal code beginner-friendly while still allowing systems programmers to drop down to raw memory, CPU, GPU/device synchronization, and MMIO when needed.

Future SIR/native backend work can recognize these operations as first-class effects:

```text
unsafe.load
unsafe.store
mmio.read
mmio.write
cpu.fence
gpu.fence
prefetch
```

That lets Slop optimize around hardware operations without incorrectly deleting or reordering them.
