# Learning Slop: The Ultimate Programming Guide

Welcome to **Slop** (Symbolic/Streaming Low-Overhead Programming)! 

Slop is an insanely high-performance, lightweight, and low learning-curve language designed for systems programmers and high-level developers alike. It combines the clean scripting feel of **Python**, the match operations of **Rust**, and the inline object capabilities of **C++**, executing at raw machine speed with zero garbage-collection pauses.

---

## Table of Contents
1. [Installation](#1-installation)
2. [Hello, World!](#2-hello-world)
3. [Variables & Types](#3-variables--types)
4. [Functions & The Pipeline Operator](#4-functions--the-pipeline-operator)
5. [C++ Inline Struct Methods](#5-c-inline-struct-methods)
6. [Rust Pattern Matching (`match`)](#6-rust-pattern-matching-match)
7. [Python List Comprehensions](#7-python-list-comprehensions)
8. [Bare-Metal Hardware Access (Pointers & Assembly)](#8-bare-metal-hardware-access-pointers--assembly)
9. [Low-Level GPU Compute Kernels](#9-low-level-gpu-compute-kernels)
10. [Universal Library Auto-Converter](#10-universal-library-auto-converter)
11. [High-Performance Array Compression (SPCA)](#11-high-performance-array-compression-spca)
12. [Bridging with C++ & Rust](#12-bridging-with-c--rust)

---

## 1. Installation

You can install Slop onto any Linux or macOS system with a single, simple terminal command (similar to installing Rust or Go):

```bash
curl -fsSL https://raw.githubusercontent.com/gugu8intel-i9/Slop/main/install.sh | bash
```

Once installed, append Slop to your `PATH` (e.g., in `~/.bashrc` or `~/.zshrc`):
```bash
export PATH="$HOME/.slop/bin:$PATH"
```

Now, the global `slop` command tool is available. You can transpile, compile, and execute scripts instantly!

### The Interactive REPL Shell
You can also launch the high-performance interactive compiling REPL shell directly by running:
```bash
slop repl
```
This lets you type Slop expressions (e.g. `x = [1, 2, 3]`, `x[1] * 5`, `"hello" + " world"`) and see immediate results executed in optimized, native machine assembly in milliseconds!

---

## 2. Hello, World!

Let's write your first Slop program. Create a file named `hello_world.slop`:

```slop
fn main(args: array[string]) {
    print("Hello, World!")
}
```

Run it instantly with:
```bash
slop run hello_world.slop
```

---

## 3. Variables & Types

Slop is statically typed but feels like a dynamic scripting language because of local type inference.

### Core Types
* `int`: 64-bit signed integer.
* `float`: 64-bit floating point.
* `bool`: Boolean value (`true` or `false`).
* `string`: Length-prefixed immutable text.
* `array[T]`: High-performance dynamic list of type `T`.

### Variable Declaration
Variables are declared using the `let` keyword:
```slop
# Type inference
let age = 42                 # Infers int
let pi = 3.14                # Infers float
let greeting = "Hello!"      # Infers string

# Explicit typing
let count: int = 100
let names: array[string] = ["Alice", "Bob", "Charlie"]
```

---

## 4. Functions & The Pipeline Operator

Functions are declared with the `fn` keyword. If a function returns a value, declare its return type using the arrow symbol `->`.

```slop
fn square(x: int) -> int {
    return x * x
}
```

### The Pipeline Operator (`|>`)
To make data pipelines highly readable, Slop includes a pipeline operator (`|>`). It passes the expression on its left as the *first parameter* to the function call on its right:

```slop
fn add_five(x: int) -> int {
    return x + 5
}

fn main(args: array[string]) {
    # 10 is passed as the parameter to add_five, and the result is passed to square!
    let result = 10 |> add_five() |> square()
    print(result) # Prints 225 ((10 + 5) ^ 2)
}
```

---

## 5. C++ Inline Struct Methods

Slop supports object-oriented structures with inline methods. Methods have access to their containing struct via the `this` keyword:

```slop
struct Rectangle {
    width: int,
    height: int

    fn area() -> int {
        return this.width * this.height
    }

    fn scale(factor: int) -> int {
        return (this.width * factor) * (this.height * factor)
    }
}

fn main(args: array[string]) {
    let r = Rectangle(5, 10)
    print(r.area())      # Prints 50
    print(r.scale(2))    # Prints 200
}
```

---

## 6. Rust Pattern Matching (`match`)

Slop includes Rust-style match blocks for clean, high-performance conditional dispatching:

```slop
fn process_code(code: int) {
    match code {
        100 => { print("Continue") },
        200 => { print("OK") },
        404 => { print("Not Found") },
        else => { print("Unknown Code") }
    }
}
```

---

## 7. Python List Comprehensions

Slop includes Python-style list comprehensions. Unlike Python, Slop compiles these directly into native loops with a pre-sized array buffer, making them execute at bare-metal speed:

```slop
fn main(args: array[string]) {
    let numbers = [1, 2, 3, 4, 5]
    
    # Square all numbers in a single, high-performance line
    let squares = [x * x for x in numbers]
    
    let i = 0
    while i < length(squares) {
        print(squares[i])
        i = i + 1
    }
}
```

---

## 8. Bare-Metal Hardware Access (Pointers & Assembly)

For operating system developers, game engines, and embedded programmers, Slop offers direct pointer-level access to machine memory and registers.

### Volatile Peek & Poke
Exposes volatile Memory-Mapped IO (MMIO) read and writes:
```slop
fn main(args: array[string]) {
    let x = 42
    
    # Get the raw physical pointer address of x
    let addr = get_address(x)
    
    # Directly write (poke) a new value into that hardware memory slot!
    poke_int(addr, 999)
    
    # Read (peek) the value back from the hardware memory slot
    let val = peek_int(addr)
    print(val) # Prints 999
}
```

### Raw Inline Blocks
Inject inline assembly (`__asm__ volatile`) or native C directly into your compiled pipeline:
```slop
fn main(args: array[string]) {
    print("Injected native instructions:")
    raw {
        "__asm__ volatile (\"nop\\n\\t\");"
        "printf(\"-> Hello from raw CPU register state!\\n\");"
    }
}
```

---

## 9. Low-Level GPU Compute Kernels

Slop includes the `gpu` keyword, allowing you to write highly parallelized compute kernels that execute directly on graphics processing hardware with **absolute zero host-side boilerplate**:

```slop
gpu scale_vectors(a: array[int], factor: int) -> array[int] {
    # 'gpu_id' represents the active GPU thread index across thousands of cores
    let id = gpu_id
    return a[id] * factor
}

fn main(args: array[string]) {
    let nums = [10, 20, 30, 40, 50]
    
    # Launches parallel compute grid on GPU natively!
    let doubled = scale_vectors(nums, 2)
}
```

The compiler automatically generates OpenCL C shader code, context creation, device mapping, host-to-device memory copy queues, grid launching, and data gathering on your behalf!

---

## 10. Universal Library Auto-Converter

Slop features a built-in auto-converter that lets you **instantly convert and bridge existing libraries** written in C, C++, Rust, or Python into Slop!

Type this single command to translate any library definitions:
```bash
slop convert my_library.h my_library.slop
```

It parses the signatures of Python files (`.py`), C/C++ headers (`.h` / `.hpp`), or Rust sources (`.rs`), translates the data types, and automatically generates high-performance Slop wrapper functions, declaring the necessary FFI linkage structures under-the-hood so that compiling and linking is 100% automated!

---

## 11. High-Performance Array Compression (SPCA)

To save memory and disk storage when processing thousands of items, Slop includes native, zero-dependency **Slop-Pack Compressed Array (SPCA)** compression:

```slop
fn main(args: array[string]) {
    let status_logs = ["active", "active", "pending", "inactive", "active", "pending"]
    
    # Compress the redundant array down to a highly packed byte format!
    # Saves up to 90% of storage on large datasets
    let packed = slop_pack(status_logs)
    print(length(packed)) # Showcases massive space reduction
    
    # Unpack it on-the-fly back to a standard, accessible array
    let restored = slop_unpack(packed)
    print(restored[0]) # Prints "active"
}
```

---

## 12. Bridging with C++ & Rust

Slop compiles directly to optimized C, which means you can link Slop modules directly with **C++** and **Rust** with **zero FFI conversion penalty**!

### C++ Bridge (`slop_bridge.hpp`)
```cpp
#include "slop_bridge.hpp"

void run_calculation() {
    slop::run_native([](SlopArena* arena) {
        // Map std::string to SlopString zero-copy!
        SlopString s = slop::copy_to_slop_string(arena, "C++ direct integration!");
        slop_print_string(s);
    }); // Memory is instantly reclaimed here!
}
```

### Rust Bridge (`rust_bridge/`)
```rust
use slop_bridge::{run_native, slop_print_string};

fn run_calculation() {
    run_native(|scope| {
        // Scope-bound allocation automatically dropped!
        let s = scope.create_string("Rust safe FFI!");
        unsafe { slop_print_string(s); }
    });
}
```

Happy coding in Slop! Let's write some insanely high-performance, low-memory code!
