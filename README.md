# Slop

**Slop** is an insanely high-performance, lightweight, and low learning-curve programming language designed for extreme efficiency, low memory footprint, and low CPU overhead.

Slop introduces **Sloppy-Escape Arena Allocation (SEAA)**: a zero-overhead, garbage-collection-free memory model that recycles memory automatically at the function level.

---

## Key Features

1. **The "Slop Bucket" Memory Architecture (SEAA)**:
   - **No Garbage Collection (GC)**, **no reference counting**, and **no manual deallocations (`free`)**.
   - Contiguous memory arenas (Buckets) are preallocated per call-depth.
   - All local variable allocations within a function happen in $O(1)$ time (single instruction pointer increment).
   - Upon function return, the entire local arena is instantly reset in $O(1)$ time, reclaiming all temporary memory with zero CPU overhead.
   - Any returned objects (like strings or arrays) are automatically cloned/moved to the caller's parent arena before wiping.
   - Absolute protection from memory fragmentation and memory leaks.

2. **Insanely High-Level, Low Learning-Curve Syntax**:
   - Modern, elegant, Python/Go-like syntax with static safety.
   - Features a built-in **Pipeline Operator (`|>`)** for high-level data pipelines (e.g., `value |> func(...)` becomes `func(value, ...)`).
   - Dynamic-feeling arrays and strings with native C execution speed.

3. **Self-Hosting (Slop Made in Slop)**:
   - The Slop compiler/lexer (`compiler.slop`) is written entirely in Slop.
   - It is bootstrapped by a Python-based transpiler (`slop_boot.py`) that outputs optimized, native C.
   - Once compiled, the native binary can compile other Slop files!

---

## Directory Structure

- `slop_rt.h` - The core runtime library containing the Bucket allocator, string functions, dynamic arrays, and file I/O operations.
- `slop_boot.py` - The bootstrap compiler/transpiler that translates Slop source code to C.
- `compiler.slop` - **The self-hosting Slop compiler/lexer written in Slop itself!**
- `hello.slop` - An example Slop script demonstrating functions, array mapping, printing, and pipeline operations.
- `DESIGN.md` - Technical specification and deep-dive into the architectural innovations of Slop.

---

## Quick Start

### 1. Run the Example Program

Compile the example `hello.slop` using the bootstrap transpiler:

```bash
# Transpile Slop to C
python3 slop_boot.py hello.slop

# Compile the C code with gcc -O3
gcc -O3 hello.c -o hello

# Run the native executable
./hello
```

**Expected Output:**
```
Hello, World!
Square of 5 is:
25
Pipeline result (4^2):
16
Sum of array elements:
6
```

### 2. Run the Self-Hosting Compiler

You can compile the Slop compiler (written in Slop) using the bootstrap tool to get a native binary:

```bash
# Transpile the self-hosting compiler to C
python3 slop_boot.py compiler.slop

# Compile to a native binary
gcc -O3 compiler.c -o slop-compiler
```

Now, run your native Slop compiler (`slop-compiler`) directly on any Slop file to see its token stream:

```bash
./slop-compiler hello.slop
```
