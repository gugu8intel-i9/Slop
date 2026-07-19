# Slop Unsafe / Hardware Control Policy

Slop is safe and easy by default. Unsafe low-level control is opt-in.

## Policy

1. Dangerous functions must be visually obvious:
   - `unsafe_*`
   - `mmio_*`
   - `cpu_*`
   - `gpu_*`
   - `device_*`
   - `ram_*`

2. Optimizers must treat low-level operations as side effects unless proven otherwise.

3. Unsafe APIs should be documented with:
   - what hardware/resource is touched
   - whether memory ordering/fences are implied
   - failure modes
   - safe alternative if one exists

4. Future sandboxed builds may reject unsafe APIs unless explicitly enabled:

```bash
slop build --allow-unsafe-hardware
```

5. Future syntax may add explicit unsafe blocks:

```slop
unsafe {
    mmio_write32(uart + 0x00, byte)
}
```

For now, the explicit function names act as the safety boundary.

## Optimizer contract

These SIR ops are anchored effects:

```text
unsafe.load
unsafe.store
mmio.read
mmio.write
cpu.fence
cpu.relax
cpu.prefetch
cpu.cycles
device.fence
gpu.fence
ram.copy
ram.zero
```

They must not be deleted or reordered across fences/device operations.
