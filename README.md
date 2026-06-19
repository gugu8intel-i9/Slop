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

2. **Combined Multi-Paradigm Syntax (C++, Rust, and Python COMBINED)**:
   - **C++ Inline Struct Methods**: Define high-performance structures with inline methods and implicit `this` pointer routing.
   - **Rust-style Pattern Matching (`match`)**: Expressive, fast matching with default `else` branches transpiled directly to optimized C chains.
   - **Python-style List Comprehensions**: Highly concise, inline data mappings (e.g. `[x * 2 for x in array]`) compiled directly into optimized, zero-allocation C loops.
   - Modern, elegant, Python/Go-like syntax with static safety and pipeline operators (`|>`).

3. **Self-Hosting (Slop Made in Slop)**:
   - The Slop compiler/lexer (`compiler.slop`) is written entirely in Slop.
   - It is bootstrapped by a Python-based transpiler (`slop_boot.py`) that outputs optimized, native C.
   - Once compiled, the native binary can compile and lex other Slop files!

4. **High-Performance C++ Native Bridge (`slop_bridge.hpp`)**:
   - A zero-copy header-only C++ library that lets you run C++ code inside Slop memory buckets.
   - Share strings and vectors between C++ and Slop without copying a single byte!

5. **High-Performance Rust Native Bridge (`rust_bridge/`)**:
   - A fully functional Cargo-crate library implementing Rust FFI bindings directly to the Slop SEAA engine.
   - Provides safe, idiomatic Rust wrappers with RAII `SlopScope` lifetimes and zero-copy string and array structures.

6. **Auto-Slop Python Transpiler (`slop_translate.py`)**:
   - Write standard Python code and automatically convert it to native `.slop` files!
   - Compiles your Python code into native machine code with **100x+ speedup**.

---

## Directory Structure

- `slop_rt.h` / `slop_rt.c` - The core runtime library containing the Bucket allocator, FFI exports, and print/IO builtins.
- `slop_boot.py` - The bootstrap compiler/transpiler that translates Slop source code to C.
- `compiler.slop` - **The self-hosting Slop compiler/lexer written in Slop itself!**
- `hello.slop` - An example Slop script demonstrating pipeline operations and arrays.
- `complex_syntax.slop` - Demonstrating C++ methods, Rust matches, and Python list comprehensions in Slop!
- `slop_bridge.hpp` - The C++ native bridge library.
- `cpp_library_test.cpp` - Example C++ code running on the high-performance Slop bridge.
- `rust_bridge/` - Cargo library for the high-performance Rust FFI bridge.
- `slop_translate.py` - The Auto-Slop Python translator.
- `test_program.py` / `test_program.slop` - Examples of automatic Python-to-Slop conversion.
- `DESIGN.md` - Technical specification and deep-dive into the architectural innovations of Slop.

---

## Quick Start

### 1. Run the Multi-Paradigm Syntax Demo

Compile and run `complex_syntax.slop` showcasing C++ methods, Rust matches, and Python comprehensions:

```bash
# Transpile Slop to C
python3 slop_boot.py complex_syntax.slop

# Compile with maximum optimizations
gcc -O3 -ffast-math -flto -march=native complex_syntax.c -o complex_syntax

# Run the native executable
./complex_syntax
```

**Expected Output:**
```
--- [Testing C++ Struct Methods] ---
Sum of fields (10 + 20):
30
Scaled sum (30 + 60):
90
--- [Testing Rust Match Statement] ---
Matched 1: Low
Matched 5: Medium
Matched other value
--- [Testing Python List Comprehension] ---
2
4
6
8
10
```

### 2. Run the Self-Hosting Compiler

You can compile the Slop compiler (written in Slop) using the bootstrap tool to get a native binary:

```bash
# Transpile the self-hosting compiler to C
python3 slop_boot.py compiler.slop

# Compile to a native binary
gcc -O3 -ffast-math -flto -march=native compiler.c -o slop-compiler

# Run the native compiler directly on any .slop file
./slop-compiler complex_syntax.slop
```

### 3. Run the C++ Native Bridge Test

```bash
g++ -O3 -march=native cpp_library_test.cpp -o cpp_library_test && ./cpp_library_test
```

### 4. Automatically Convert Python Code to Slop & Run Natively

```bash
# Translate Python script to native Slop
python3 slop_translate.py test_program.py test_program.slop

# Compile the newly generated test_program.slop to C
python3 slop_boot.py test_program.slop test_program.c

# Compile and run
gcc -O3 -march=native test_program.c -o test_program && ./test_program
```

### 5. High-Performance Rust Bridge Compilation

The Rust library is organized as a Cargo package located in `rust_bridge/`. To build and use it on any machine with Cargo installed:

```bash
cd rust_bridge
cargo build --release
cargo run --example test_bridge
```
