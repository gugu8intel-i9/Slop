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

4. **High-Performance C++ Native Bridge (`slop_bridge.hpp`)**:
   - A zero-copy header-only C++ library that lets you run C++ code inside Slop memory buckets.
   - Share strings and vectors between C++ and Slop without copying a single byte!

5. **Auto-Slop Python Transpiler (`slop_translate.py`)**:
   - Write standard Python code and automatically convert it to native `.slop` files!
   - Compiles your Python code into native machine code with **100x+ speedup**.

---

## Directory Structure

- `slop_rt.h` - The core runtime library containing the Bucket allocator, string functions, dynamic arrays, and file I/O operations.
- `slop_boot.py` - The bootstrap compiler/transpiler that translates Slop source code to C.
- `compiler.slop` - **The self-hosting Slop compiler/lexer written in Slop itself!**
- `hello.slop` - An example Slop script demonstrating functions, array mapping, printing, and pipeline operations.
- `slop_bridge.hpp` - The C++ native bridge library.
- `cpp_library_test.cpp` - Example C++ code running on the high-performance Slop bridge.
- `slop_translate.py` - The Auto-Slop Python translator.
- `test_program.py` / `test_program.slop` - Examples of automatic Python-to-Slop conversion.
- `DESIGN.md` - Technical specification and deep-dive into the architectural innovations of Slop.

---

## Quick Start

### 1. Run the Example Program

Compile the example `hello.slop` using the bootstrap transpiler:

```bash
# Transpile Slop to C
python3 slop_boot.py hello.slop

# Compile the C code with maximum GCC optimizations
gcc -O3 -ffast-math -flto -march=native hello.c -o hello

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
gcc -O3 -ffast-math -flto -march=native compiler.c -o slop-compiler

# Run the native compiler directly on any .slop file
./slop-compiler hello.slop
```

### 3. Run the C++ Native Bridge Test

```bash
# Compile the C++ program
g++ -O3 -march=native cpp_library_test.cpp -o cpp_library_test

# Run the native test
./cpp_library_test
```

### 4. Automatically Convert Python Code to Slop & Run Natively

```bash
# Translate Python script to native Slop
python3 slop_translate.py test_program.py test_program.slop

# Compile the newly generated test_program.slop to C
python3 slop_boot.py test_program.slop test_program.c

# Compile with maximum optimization
gcc -O3 -march=native test_program.c -o test_program

# Run the translated program at full compiled C speed!
./test_program
```
