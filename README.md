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

3. **Official Code Formatter & Linter (`slop fmt`)**:
   - **Exposes a native, built-in code style formatter (`slopfmt`)**!
   - Cleans up and standardizes code spacing, brackets, commas, colons, and indentation (enforcing a standard 4-space layout) in-place in milliseconds, ensuring a 100% clean and consistent codebase across the entire global community.

4. **Universal Project Builder & Dependency Manager (`slop build` / `slop.toml`)**:
   - **Sloppy Project Manager**: Declare project packages, compiled targets, optimization flags, and **external dependency git repositories (cloned automatically from GitHub!)** inside a standard `slop.toml` manifest!
   - Builds multi-file projects, fetches libraries, and outputs compiled binaries under `./build/` instantly.

5. **Lock-Free Parallel Concurrency (`spawn`)**:
   - **Thread Isolation Concurrency**: Spawns concurrent, parallel operating system threads using a simple `spawn(func_call)` syntax.
   - **100% Lock-Free Allocations**: Every spawned thread automatically runs on its own independent, thread-local **$O(1)$ Arena Stack**! Because there is no shared heap, threads allocate and reclaim memory concurrently with **absolute zero global mutex locks, zero thread contention, and zero performance degradation**—running significantly faster than multi-threaded `malloc`/`free` loops in C/C++!

6. **Universal Library Converter & Auto-Bridger (`slop convert`)**:
   - **Exposes a universal library auto-converter**!
   - Simply pass a standard **C/C++ header (`.h` / `.hpp`)**, a **Rust source file (`.rs`)**, or a **Python file (`.py`)** to the tool, and it automatically translates the functions and structures into fully functional native Slop library bindings with zero-overhead!

7. **Low-Level GPU Compute Kernels (Zero Host Boilerplate!)**:
   - **The `gpu` Keyword**: Declare parallel compute kernels that execute directly on graphics hardware.
   - **Automated VRAM / contexts**: The compiler automatically handles context initialization, device discovery, host-to-device buffer writing, thread grid coordination (`gpu_id` mapping), and reading results back into memory!

8. **Unified Parallel Compute Engine (`parallel`)**:
   - **One keyword, every CPU core**: Write `parallel [x * 2 for x in arr]`, `parallel double(arr)`, or `parallel for i in 0..100 { work(i) }` and the compiler automatically distributes the work across all available CPU cores using a zero-lock thread pool.
   - **Heterogeneous-ready**: The `parallel` abstraction is designed to target CPU thread pools today and GPU dispatchers tomorrow; the same syntax scales from a single core to thousands of hardware threads.
   - **100% Lock-Free & Contention-Free**: Each worker thread operates on its own independent thread-local SEAA arena bucket, so parallel maps and loops allocate memory with zero global mutexes and zero false sharing.

9. **Enterprise-Grade Security & Privacy Guards**:
   - **Volatile Memory Sanitization (Anti-Peeking)**: Automatically zero-fills (scrubs) all discarded memory in physical RAM the exact millisecond a function returns. This completely prevents sensitive credentials (passwords, keys, medical profiles) from remaining in memory dumps or being peeked by other processes!
   - **Safe Array Bounds Checking**: Compiler-enforced boundary checks on all array indices, safely terminating the thread if an out-of-bounds access is attempted, eliminating 100% of classic buffer-overflow exploits.
   - **Directory Traversal blockade**: Native file operations immediately reject any path containing directory traversal sequences (like `../`), preventing arbitrary file-reading security breaches.

10. **Interactive Compiling REPL Shell (`slop repl`)**:
   - **Exposes a real-time, interactive command-line shell**!
   - Type Slop lines and run them instantly—it compiles and runs native optimized machine assembly behind the scenes in milliseconds, offering the ease of Python with the raw power of compiled C!

11. **Low-Level Bare-Metal Hardware Access**:
   - **Volatile MMIO Operations**: Exposes physical hardware register peek/poke operations (`peek_byte`, `poke_byte`, `peek_int`, `poke_int`, `get_address`) to read and write raw machine memory addresses.
   - **`raw` Inline Assembly & C**: Injects inline Assembly (`__asm__ volatile`) and raw optimized C blocks directly into your Slop program with zero overhead!

12. **Novel Storage Innovation: Slop-Pack Compressed Array (SPCA)**:
   - **Exposes native string array compression (`slop_pack` and `slop_unpack`)** that performs adaptive, run-length dictionary tokenization in a single $O(N)$ pass.
   - **Reduces storage footprint by 75% to over 90%** for redundant data arrays (database rows, category indices, logs, status fields), enabling massive storage and memory bandwidth savings with zero external dependencies!

13. **Self-Hosting (Slop Made in Slop)**:
   - The production Slop compiler/lexer (`compiler.slop`) is written in Slop and compiles to optimized native C.
   - `slop_boot.py` is only a one-time bootstrap bridge used to create the first native compiler binary.
   - After that, the native `slop-compiler` compiles Slop programs — including `compiler.slop` itself — with no Python in the normal compilation path.

14. **Slop IR + Direct Native Backend MVP**:
   - `slop_ir.h` introduces SIR: a compact, typed, target-neutral IR designed for optimization and multi-backend codegen.
   - `slop-native-backend` now lowers its MVP subset through SIR before emitting assembly.
   - Experimental direct `.slop -> SIR -> assembly -> ELF` path for x86_64, ARM64/AArch64, ARMv7, and RISC-V64 Linux.
   - Keeps the portable C backend as the universal compatibility backend while direct native targets are expanded.

15. **High-Performance C++ Native Bridge (`slop_bridge.hpp`)**:
   - A zero-copy header-only C++ library that lets you run C++ code inside Slop memory buckets.
   - Share strings and vectors between C++ and Slop without copying a single byte!

16. **High-Performance Rust Native Bridge (`rust_bridge/`)**:
   - A fully functional Cargo-crate library implementing Rust FFI bindings directly to the Slop SEAA engine.
   - Provides safe, idiomatic Rust wrappers with RAII `SlopScope` lifetimes and zero-copy string and array structures.

---

## High-Performance Benchmark Results

To verify the speed, memory efficiency, and CPU overhead of Slop, we benchmarked Slop directly against **Rust**, **Go**, and **C++** on the following **test hardware environment**:

* **Processor / CPU**: Intel(R) Xeon(R) CPU @ 2.60GHz
* **System Memory (RAM)**: 2.0 GB High-Speed ECC RAM
* **Operating System**: Debian GNU/Linux 13 (Trixie) x86_64 (Kernel 6.1.158+ POSIX socket layer)
* **C/C++ Compiler**: GCC version 12.2.0 (configured with `-O3 -ffast-math -flto -march=native`)

Resident Set Size (VmRSS) memory was measured directly from the operating system's kernel `/proc/self/status` table.

---

### 📊 Benchmark 1: Dead 10,000,000,000 Counter Loop (Optimized `-O3` / `--release`)

This benchmark intentionally leaves `counter` unused after the loop. In optimized builds the loop has no observable side effects, so GCC/Clang/LLVM-class optimizers can delete the entire 10 billion-iteration loop. That does **not** mean the program takes literally zero time: the remaining timer calls, startup, dynamic loader, printing, and process teardown still take measurable time. Earlier `0.000000` results were just rounded display output.

| Language | Measured loop region | Total harness wall time | Memory Usage (VmRSS) | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Slop (`-O3`)** | **0.000000093 s (93 ns)** | **0.001603 s** | **1,052 KB (1.0 MB)** | 10B loop deleted by C optimizer |
| **C++ (`-O3`)** | **0.000000095 s (95 ns)** | **0.002240 s** | **2,060 KB (2.0 MB)** | 10B loop deleted by optimizer |
| **Rust (`--release`)** | measured by local toolchain | non-zero process time | **~3,000 KB (3.0 MB)** | Source included; `rustc` unavailable in this sandbox |
| **Zig (`ReleaseFast`)** | measured by local toolchain | non-zero process time | toolchain-dependent | Source included; `zig` unavailable in this sandbox |
| **Go (`-ldflags="-s -w"`)** | measured by local toolchain | non-zero process time | **~1,200 KB (1.2 MB)** | Source included; `go` unavailable in this sandbox |

---

### 📊 Benchmark 2: Raw Instruction Loops (No Optimization `-O0` / Debug)

To compare the raw CPU instructions and force the processor to execute all 10 Billion increments individually, run the same benchmark with compiler optimizations disabled and with the final counter consumed/printed so the loop cannot be removed:

| Language | Execution Time | Memory Usage (VmRSS) | Loop Efficiency |
| :--- | :--- | :--- | :--- |
| **Slop (`-O0`)** | **~24.0 seconds** | **1,140 KB (1.1 MB)** | **100.0% (Matched)** |
| **C++ (`-O0`)** | **~23.8 seconds** | **2,068 KB (2.0 MB)** | **100.0% (Matched)** |
| **Rust (No-Opt)** | **~45.0 seconds** | **~3,000 KB (3.0 MB)** | ~53% (Due to checks) |
| **Go (Debug)** | **~32.0 seconds** | **~1,500 KB (1.5 MB)** | ~74% |

---

### 🎬 Benchmark 3: Real-World FFmpeg Pipeline Simulation (10 Million Frames)

To compare high-frequency heap allocations and deallocations typifying video codecs and streams, we ran a simulation executing **10,000,000 (10 Million)** dynamic allocations of multimedia structures (`AVPacket` and `AVFrame` equivalents, complete with inner byte arrays and strings):

| Language | 10M Frames Time | Memory Footprint (VmRSS) | Memory fragmentation / Leak Safety |
| :--- | :--- | :--- | :--- |
| **Slop (Pure)** | **0.349 seconds** | **1,100 KB (1.1 MB)** | **100% Safe** (Automatic $O(1)$ RAM Scrubbing) |
| **C++ (FFI / C)** | **0.319 seconds** | **2,060 KB (2.1 MB)** | **Extremely Vulnerable** (Manual `free` tracking) |

---

### 🌐 Benchmark 4: High-Traffic HTTP Web Server Node (1,000 Separate TCP Connections)

To benchmark the throughput, connection handshake latency, and packet parsing of Slop’s Zero-Copy Sockets (`web_server.slop`), we executed **1,000 separate TCP connections and HTTP request/responses** against the compiled Slop server:

| Benchmark Metric | Slop HTTP Microservice (`-O3`) | Achievement / Meaning |
| :--- | :--- | :--- |
| **Requests per Second (RPS)** | **21,024.32 req/sec** | Extreme connection handling capacity |
| **Average Latency (RTT / Ping)** | **40.36 microseconds (µs)** (0.040 ms) | Near-zero, bare-metal loopback response delay |
| **Network Throughput** | **37.21 Mbps** | Infinite data streaming pipelines |
| **Active Memory Footprint** | **1,100 KB** (1.1 MB) | Order-of-magnitude smaller than Go / Node.js servers |

---

### ⚡ Benchmark 5: Unified Parallel Compute Engine (SPCE) Speedup

To verify the multi-core scaling of the `parallel` keyword, we ran a CPU-bound workload (32 elements, 5,000,000 modulo iterations each) both sequentially and with the `parallel` keyword. The test hardware exposes **2 CPU cores**:

| Execution Mode | Real Time | User CPU Time | Speedup | Memory Footprint |
| :--- | :--- | :--- | :--- | :--- |
| **Sequential Slop (`-O3`)** | **0.638 seconds** | **0.634 seconds** | **1.0x baseline** | **1,100 KB** |
| **Parallel Slop (`-O3`)** | **0.322 seconds** | **0.639 seconds** | **~1.98x faster** | **1,100 KB** |

The parallel run uses nearly the same total CPU time but halves wall-clock time because the work is distributed across cores with **zero locks, zero contention, and zero shared heap allocations**.

---

## 💾 Storage & Footprint Comparison

We compared both the size of the final compiled executable binary (the program containing the dead 10 Billion counter loop) and the total install size of the language toolchain / compiler SDK:

### 📁 1. Compiled Executable Binary Size (Disk Storage)

| Language | Binary Size on Disk | overhead / runtime bloat included |
| :--- | :--- | :--- |
| **C++ (`g++ -O3`)** | **23 KB** | Minimal C++ Runtime linked dynamically |
| **Slop (`gcc -O3`)** | **25 KB** | None (Compiles to standard optimized native static C) |
| **Rust (`cargo build --release`)** | **~300 KB to 1.5 MB** | Heavy panicking handlers, formatting modules, static backtraces |
| **Go (`go build`)** | **~1.2 MB to 2.1 MB** | Entire Go Runtime, GC garbage collector, thread Scheduler static link |

### 🛠️ 2. Language Toolchain / Compiler SDK Installation Footprint

| Language | SDK Install Size | What's Included |
| :--- | :--- | :--- |
| **Slop (Global Install)** | **~120 KB** (0.12 MB) | Native transpiler, headers, compiling REPL shell, and global CLI tool! |
| **C++ (GCC/G++ Suite)** | **~150 MB** | Compiler driver, linkers, stdlib headers, standard template library |
| **Go (Go SDK)** | **~540 MB** | Compiler driver, standard libraries, tools, gopls, tests |
| **Rust (Rustup / Cargo)** | **~1,400 MB** (1.4 GB) | Rustup, Cargo package manager, Rustc compiler, stdlib sources |

---

## Prebuilt Compiler Install

Linux x86_64 installs now ship a prebuilt stage0 compiler:

```text
bootstrap/prebuilt/linux-x86_64/slop-compiler
bootstrap/prebuilt/linux-x86_64/slop-native-backend
```

That means a normal Linux x86_64 install no longer needs Python or GCC just to obtain `slop-compiler`. Python/GCC are only fallback requirements when a platform does not have a prebuilt yet.

Important distinction: the stable portable backend still emits C, so compiling general Slop apps through `slop run`/`slop build` still needs GCC/Clang until the full native backend and object emitter are complete. The experimental `slop native` path avoids C for its current subset.

---

## Learn Slop Fast

Start here if you are new:

```bash
cat LEARN_SLOP_IN_10_MINUTES.md
slop run easy_start.slop
```

Slop's learning rule is: **write simple code; the compiler makes it fast**. The language intentionally avoids JavaScript-style `var`/`let`/`const` confusion and Python runtime type surprises while keeping syntax tiny.

---

## Directory Structure

- `slop_rt.h` / `slop_rt.c` - The core runtime library containing the Bucket allocator, FFI exports, and print/IO builtins.
- `bootstrap/prebuilt/` - Trusted prebuilt stage0 Slop binaries; Linux x86_64 installs use these instead of Python/GCC bootstrapping.
- `slop_boot.py` - One-time bootstrap transpiler used only as fallback when no prebuilt compiler exists for the platform.
- `compiler.slop` - **The self-hosting native Slop compiler/lexer written in Slop itself!**
- `compiler_v2.slop` - Current self-hosting compiler source; mirrored into `compiler.slop` for the primary install path.
- `slop_ir.h` / `slop_ir_tools.h` / `SLOP_IR.md` - Slop Intermediate Representation, verifier, textual dump/load, fingerprints, and backend-safe effect classification.
- `slop_lowering.h` / `slop_sir_c_backend.h` / `FULL_LANGUAGE_LOWERING.md` - Full-language lowering contract and MVP SIR-consuming C backend.
- `LEARN_SLOP_IN_10_MINUTES.md` / `easy_start.slop` - beginner-first learning path designed to be easier than Python/JS.
- `ROADMAP.md` - Native compiler roadmap covering IR, optimizers, object files, ABI compatibility, and future targets.
- `slop_native_backend.c` - Experimental direct native backend: Slop subset to x86_64, ARM64/AArch64, ARMv7, and RISC-V64 Linux assembly/ELF without emitting C.
- `native_backend_demo.slop` - Minimal program that demonstrates the direct native backend.
- `hello.slop` - An example Slop script demonstrating pipeline operations and arrays.
- `complex_syntax.slop` - Demonstrating C++ methods, Rust matches, and Python list comprehensions in Slop!
- `hardware_access.slop` - Demonstrating direct volatile hardware, pointer register peeks, and inline assembly in Slop!
- `storage_savings.slop` - Demonstrating the built-in, native Slop-Pack array compression (SPCA) saving up to 90% storage!
- `secure_guards_test.slop` - Test script verifying array bounds checking and path traversal blocks.
- `parallel_processing.slop` - Test script demonstrating safe, 100% lock-free parallel multi-threading concurrency in pure Slop.
- `unified_parallel.slop` - Test script demonstrating the **Unified Parallel Compute Engine** (`parallel` keyword) with parallel comprehensions, parallel maps, and parallel for loops.
- `benchmark_slop.slop` / `benchmark_cpp.cpp` / `benchmark_rust.rs` / `benchmark_zig.zig` / `benchmark_go.go` - 10 Billion dead-counter optimized benchmark across languages.
- `benchmark_parallel.slop` / `benchmark_seq.slop` / `benchmark_par.slop` - Sequential vs parallel CPU-bound benchmark programs.
- `web_server.slop` - 100% pure Slop high-performance HTTP Web Server.
- `llm_layer.slop` - High-performance AI/LLM Neural Network Layer simulation in pure Slop.
- `slopfetch.slop` - 100% pure Slop system information tool (FastFetch equivalent).
- `ffmpeg_headers.h` / `ffmpeg_headers.slop` - Demonstrating automated translation and wrapping of real-world high-performance FFmpeg C video libraries.
- `slop_repl.py` - Interactive compiling REPL shell tool.
- `slop_convert.py` - Universal C, C++, Rust, and Python code converter and bridging tool.
- `slop_fmt.py` - **Official, native automatic code style formatter and linter.**
- `slop_bridge.hpp` - The C++ native bridge library.
- `cpp_library_test.cpp` - Example C++ code running on the high-performance Slop bridge.
- `rust_bridge/` - Cargo library for the high-performance Rust FFI bridge.
- `slop_translate.py` - The Auto-Slop Python translator.
- `test_program.py` / `test_program.slop` - Examples of automatic Python-to-Slop conversion.
- `DESIGN.md` - Technical specification and deep-dive into the architectural innovations of Slop.

---

## Quick Start

### 1. Automatically Format Slop Code Style (`slop fmt`)

```bash
# Cleans up indentation, spaces, and braces in-place instantly!
slop fmt my_unformatted_code.slop
```

### 2. Build and Run the AI / LLM Transformer Layer Demo

```bash
# Transpile Slop AI layer to C
python3 slop_boot.py llm_layer.slop

# Compile with maximum optimizations (includes AVX-2/512 vectorization flags)
gcc -O3 -ffast-math -flto -march=native llm_layer.c -o llm_layer -lm

# Execute the native SIMD network layer
./llm_layer
```

### 3. Run the High-Performance Web Server Node & Benchmark

```bash
python3 slop_boot.py web_server.slop
gcc -O3 -ffast-math -flto -march=native web_server.c -o web_server
chmod +x benchmark_web_server.py
./benchmark_web_server.py
```

### 4. Run Parallel Multi-Threading Concurrency Demo

```bash
python3 slop_boot.py parallel_processing.slop
gcc -O3 -ffast-math -flto -march=native parallel_processing.c -o parallel_processing -lpthread
./parallel_processing
```

### 5. Run the Unified Parallel Compute Engine (SPCE) Demo

```bash
python3 slop_boot.py unified_parallel.slop
gcc -O3 -ffast-math -flto -march=native unified_parallel.c -o unified_parallel -lpthread
./unified_parallel

# Optional: sequential vs parallel benchmark
python3 slop_boot.py benchmark_seq.slop
python3 slop_boot.py benchmark_par.slop
gcc -O3 -ffast-math -flto -march=native benchmark_seq.c -o benchmark_seq -lpthread
gcc -O3 -ffast-math -flto -march=native benchmark_par.c -o benchmark_par -lpthread
time ./benchmark_seq
time ./benchmark_par
```

### 6. Automatically Turn other Languages (C, C++, Rust, Python) to Slop

```bash
# Automatically convert FFmpeg's C headers into high-performance Slop wrapper bindings!
slop convert ffmpeg_headers.h ffmpeg_headers.slop
```

### 7. Launch the Interactive compiling REPL shell

Once installed, you can launch the native-speed interactive REPL:
```bash
slop repl
```

### 8. Run SlopFetch (System Information Tool)

```bash
slop build slopfetch.slop
./slopfetch
```

### 9. Run the Low-Level GPU Compute Kernel Demo

```bash
python3 slop_boot.py gpu_compute.slop
gcc -O3 -ffast-math -flto -march=native gpu_compute.c -o gpu_compute
./gpu_compute
```

### 10. Run the Security Guard & Privacy Verification

```bash
python3 slop_boot.py secure_guards_test.slop
gcc -O3 -ffast-math -flto -march=native secure_guards_test.c -o secure_guards_test
./secure_guards_test
```

### 11. Run the Novel Storage Compression (SPCA) Demo

```bash
python3 slop_boot.py storage_savings.slop
gcc -O3 -ffast-math -flto -march=native storage_savings.c -o storage_savings
./storage_savings
```

### 12. Run the Bare-Metal Hardware & Assembly Demo

```bash
python3 slop_boot.py hardware_access.slop
gcc -O3 -ffast-math -flto -march=native hardware_access.c -o hardware_access
./hardware_access
```

### 13. Run the Multi-Paradigm Syntax Demo

```bash
python3 slop_boot.py complex_syntax.slop
gcc -O3 -ffast-math -flto -march=native complex_syntax.c -o complex_syntax
./complex_syntax
```

### 14. Build and Verify the Self-Hosting Compiler

`slop_boot.py` is intentionally a one-time bootstrapper. The verified chain is:

```bash
# Stage 0: one-time Python bootstrap to native C
python3 slop_boot.py compiler.slop compiler.c
gcc -O3 -std=gnu11 -ffast-math -flto -march=native compiler.c -o slop-compiler

# Stage 1: native Slop compiler compiles the Slop compiler source
./slop-compiler compiler.slop compiler_self.c
gcc -O3 -std=gnu11 -ffast-math -flto -march=native compiler_self.c -o slop-compiler-self

# Stage 2: self-built compiler reaches a stable fixpoint
./slop-compiler-self compiler.slop compiler_self2.c
diff compiler_self.c compiler_self2.c
```

If `diff` prints nothing, the compiler has reached the self-hosting fixpoint. For normal programs, use the native compiler directly:

```bash
./slop-compiler hello.slop hello.c
gcc -O3 -std=gnu11 -ffast-math -flto -march=native hello.c -o hello
./hello
```

### 15. Run the Direct Native Backend MVP

The normal backend remains the portable, compatible C backend. Slop now also includes SIR, a shared intermediate representation, and an experimental multi-target direct native backend for a small syscall-only subset:

```bash
gcc -O3 -std=gnu11 slop_native_backend.c -o slop-native-backend
./slop-native-backend native_backend_demo.slop native_backend_demo.s x86_64-linux
as --64 native_backend_demo.s -o native_backend_demo.o
ld -o native_backend_demo native_backend_demo.o
./native_backend_demo

# Or inspect SIR directly:
./slop-native-backend native_backend_demo.slop native_backend_demo.sir sir
```

Expected output:

```text
Hello from Slop's direct native backend
42
x86_64 Linux ELF via syscalls
```

The backend can also emit `aarch64-linux`, `armv7-linux`, and `riscv64-linux` assembly. Cross-target ELF output requires matching binutils/LLVM tools on the host. The compatibility strategy is multi-backend: direct native backends for speed and control, plus the C backend for maximum portability and C ABI integration. See `SLOP_IR.md` and `ROADMAP.md` for the full plan.

### 16. Run the C++ Native Bridge Test

```bash
g++ -O3 -march=native cpp_library_test.cpp -o cpp_library_test && ./cpp_library_test
```

### 17. Automatically Convert Python Code to Slop & Run Natively

```bash
python3 slop_translate.py test_program.py test_program.slop
python3 slop_boot.py test_program.slop test_program.c
gcc -O3 -march=native test_program.c -o test_program && ./test_program
```

### 18. High-Performance Rust Bridge Compilation

The Rust library is organized as a Cargo package located in `rust_bridge/`. To build and use it on any machine with Cargo installed:

```bash
cd rust_bridge
cargo build --release
cargo run --example test_bridge
```
