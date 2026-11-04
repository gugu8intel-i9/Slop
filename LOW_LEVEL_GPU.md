# Low-Level GPU Control

`gpu for` and `gpu_matmul` run on the GPU for you. When you want more control,
`use "gpu"` gives you:

- buffers that live on the GPU, and copies of numbers in and out,
- kernels you write yourself, in the GPU language of each platform,
- a launch that runs a kernel over many threads and waits until it is done.

```slop
use "gpu"
```

## Backends

| Platform | Backend | Kernel source |
|---|---|---|
| Linux, Windows | Vulkan (tried first), then OpenCL | SPIR-V bytes (Vulkan), OpenCL C (OpenCL) |
| macOS on Apple Silicon | Metal | Metal Shading Language |

`gpu_backend()` says which one is in use: `"vulkan"`, `"opencl"`, `"metal"`, or
`"none"`. Setting `SLOP_GPU=vulkan` or `SLOP_GPU=opencl` picks one, and
`SLOP_GPU=none` turns the GPU off.

Nothing crashes when there is no GPU. Buffers and kernels come back as `0`, and
`gpu_run` returns `false`. Call `gpu_error()` to see why.

Numbers on the GPU are 32 bits. Floats lose a little precision, and whole numbers
are 32-bit signed.

## The common Slop code

This helper works with every backend. It takes the kernel source and its entry
name, and it expects the kernel to take four buffers: `a`, `b`, `c` and `count`
(the number of items, as one 32-bit integer). The kernel must check `i < count`,
because the GPU may start a few extra threads.

```slop
use "gpu"

fn vector_add(source: string, entry: string, xs: array[float], ys: array[float]) -> array[float] {
    let none: array[float] = []
    let n = len(xs)
    let k = gpu_kernel(source, entry)
    if k == 0 {
        eprint("no kernel: " + gpu_error())
        return none
    }
    let a = gpu_alloc(4 * n)
    let b = gpu_alloc(4 * n)
    let c = gpu_alloc(4 * n)
    let count = gpu_alloc(4)
    gpu_write(a, 0, gpu_pack_f32(xs))
    gpu_write(b, 0, gpu_pack_f32(ys))
    gpu_write(count, 0, gpu_pack_i32([n]))
    if not gpu_run(k, [a, b, c, count], n) {
        eprint("run failed: " + gpu_error())
        return none
    }
    let sums = gpu_unpack_f32(gpu_read(c, 0, 4 * n))
    gpu_free(a)
    gpu_free(b)
    gpu_free(c)
    gpu_free(count)
    return sums
}
```

## OpenCL example

The kernel is OpenCL C, given to Slop as a string:

```c
__kernel void add(__global const float* a, __global const float* b,
                  __global float* c, __global const int* count) {
    int i = get_global_id(0);
    if (i < count[0]) c[i] = a[i] + b[i];
}
```

```slop
use "gpu"

fn main() {
    let src = "__kernel void add(__global const float* a, __global const float* b, __global float* c, __global const int* count) { int i = get_global_id(0); if (i < count[0]) c[i] = a[i] + b[i]; }"
    let xs: array[float] = [1.0, 2.0, 3.0]
    let ys: array[float] = [0.5, 0.5, 0.5]
    print(vector_add(src, "add", xs, ys))
}
```

## Metal example (macOS)

The kernel is Metal Shading Language. `[[buffer(i)]]` is the buffer number
that `gpu_run` binds the i-th buffer to:

```metal
#include <metal_stdlib>
using namespace metal;
kernel void add(device const float* a [[buffer(0)]],
                device const float* b [[buffer(1)]],
                device float* c [[buffer(2)]],
                device const int* count [[buffer(3)]],
                uint i [[thread_position_in_grid]]) {
    if ((int)i < count[0]) c[i] = a[i] + b[i];
}
```

```slop
use "gpu"

fn main() {
    let src = "#include <metal_stdlib>\nusing namespace metal;\nkernel void add(device const float* a [[buffer(0)]], device const float* b [[buffer(1)]], device float* c [[buffer(2)]], device const int* count [[buffer(3)]], uint i [[thread_position_in_grid]]) { if ((int)i < count[0]) c[i] = a[i] + b[i]; }"
    let xs: array[float] = [1.0, 2.0, 3.0]
    let ys: array[float] = [0.5, 0.5, 0.5]
    print(vector_add(src, "add", xs, ys))
}
```

## Vulkan example (Linux and Windows)

Vulkan takes SPIR-V, which is a compiled shader. Write the shader in GLSL, then
compile it with `glslangValidator`:

```glsl
#version 450
layout(local_size_x = 64) in;
layout(std430, set = 0, binding = 0) buffer A { float a[]; };
layout(std430, set = 0, binding = 1) buffer B { float b[]; };
layout(std430, set = 0, binding = 2) buffer C { float c[]; };
layout(std430, set = 0, binding = 3) buffer N { int count[]; };
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (int(i) < count[0]) {
        c[i] = a[i] + b[i];
    }
}
```

```sh
glslangValidator -V add.comp -o add.spv
```

`local_size_x` is the number of threads in one group. `gpu_run` starts enough
groups to cover all the threads you ask for, so the `count` check is still needed.
Vulkan's entry point is named `main`.

```slop
use "gpu"

fn main() {
    let xs: array[float] = [1.0, 2.0, 3.0]
    let ys: array[float] = [0.5, 0.5, 0.5]
    print(vector_add(read_file("add.spv"), "main", xs, ys))
}
```

On Vulkan the source is the file's bytes, so `read_file` gives the right
string. `gpu_kernel` checks the SPIR-V header and returns `0` with an error if it is
not SPIR-V.

## Functions

| Function | What it does |
|---|---|
| `gpu_backend() -> string` | `"vulkan"`, `"opencl"`, `"metal"` or `"none"` |
| `gpu_device() -> string` | The GPU's name, or `""` |
| `gpu_alloc(bytes: int) -> int` | A zeroed buffer of that many bytes. Handle above 0, or 0 |
| `gpu_free(buf: int)` | Frees a buffer |
| `gpu_write(buf: int, offset: int, data: string) -> bool` | Copies the bytes of `data` into the buffer at `offset` |
| `gpu_read(buf: int, offset: int, bytes: int) -> string` | Copies `bytes` bytes out of the buffer, from `offset` |
| `gpu_pack_f32(xs: array[float]) -> string` | Numbers as 32-bit little-endian floats |
| `gpu_unpack_f32(s: string) -> array[float]` | The floats in a byte string |
| `gpu_pack_i32(xs: array[int]) -> string` | Whole numbers as 32-bit little-endian integers |
| `gpu_unpack_i32(s: string) -> array[int]` | The integers in a byte string |
| `gpu_kernel(source: string, entry: string) -> int` | Compiles a kernel. Handle above 0, or 0 with `gpu_error()` saying why |
| `gpu_run(kernel: int, buffers: array[int], threads: int) -> bool` | Runs the kernel with the buffers as arguments `0, 1, ...` on `threads` threads, and waits |
| `gpu_error() -> string` | Why the last `gpu_` call failed, or `""` after a call that worked |

## Limits

- Up to 1024 live buffers, and up to 256 kernels in one run of the program.
- A buffer is at most 1 GiB. A `gpu_run` uses from 1 to 16 buffers.
- On Vulkan, one `gpu_run` is at most 65535 groups of the kernel's local size.
- `gpu_run` waits for the GPU to finish, so there is no overlap with CPU work.
- `gpu for` loops and `gpu_matmul` do not use these buffers. Copy the data
  yourself if you want to share it.

## Testing without a GPU

On Linux, the CPU drivers can stand in for a GPU. `SLOP_OPENCL_CPU=1` allows an
OpenCL CPU device, and `SLOP_VULKAN_CPU=1` allows a Vulkan CPU device:

```sh
SLOP_GPU=vulkan SLOP_VULKAN_CPU=1 slop run tests/gpu.slop
SLOP_GPU=opencl SLOP_OPENCL_CPU=1 slop run tests/gpu.slop
```

`tests/gpu.slop` checks the buffers, the kernel and a run of 100,000 numbers on
whichever backend is there. It prints the same lines on every machine.
