# The Slop Programming Language

Slop (Symbolic/Streaming Low-Overhead Programming) is an insanely high-performance, lightweight, and low learning-curve programming language. 

Slop features a novel memory management paradigm, an extremely clean and high-level syntax, and a self-hosting compiler that compiles Slop code directly to optimized C.

---

## 1. Core Architectural Innovations

### A. The "Slop Bucket" Memory Architecture (Sloppy-Escape Arena Allocation - SEAA)
Traditional languages use manual memory management (C/C++, Rust), reference counting (Swift, Python), or garbage collection (Go, Java, JS). 

Slop introduces **Sloppy-Escape Arena Allocation (SEAA)**:
- **No Garbage Collector, No Reference Counting, No Manual `free`**.
- **The Bucket Model**: Every function call receives two memory Arenas (Buckets):
  1. `caller_arena`: Used to allocate the return value of the function.
  2. `local_arena`: Used for all temporary variables, strings, and intermediate structures created during the function's execution.
- **O(1) Allocation**: Allocation is a single CPU instruction: incrementing a pointer in the local arena block.
- **O(1) Instant Reclamation**: When a function returns, the `local_arena` offset is reset to its entry state. All intermediate memory is instantly reclaimed at zero CPU cost.
- **The "Slopped Over" Return**: Any returned object (like a string or array) is automatically written into the `caller_arena`.
- **Zero-fragmentation and Insanely Low Memory Footprint**: Because Arenas use contiguous memory blocks, cache locality is perfect (100% L1/L2 cache friendly), and there is zero heap fragmentation.

### B. High-Level, Low Learning-Curve Syntax
Slop combines the simplicity of Python, the static safety of Go, and high-level syntax powerhouses like pipeline operators.

```slop
# Comments start with a hash

fn greet(name: string) -> string {
    return "Hello, " + name + "!"
}

fn process_data(nums: array[int]) -> int {
    # Pipeline operator |> passes the left side as the first argument to the right side
    let sum = nums 
        |> filter(fn(x: int) -> bool { return x % 2 == 0 })
        |> map(fn(x: int) -> int { return x * x })
        |> sum_all()
    
    return sum
}
```

### C. Self-Hosting (Slop Made in Slop)
The Slop compiler (`compiler.slop`) is written in Slop. To compile itself:
1. We write a lightweight **bootstrapper transpiler** (`slop_boot.py`) in Python that compiles Slop to native C.
2. We compile `compiler.slop` using `slop_boot.py` into native C.
3. We compile the generated C code with `gcc -O3` to create the ultra-fast, native `slop-compiler`.
4. From then on, `slop-compiler` compiles any future Slop code (including its own new versions) to native, high-performance C.

---

## 2. Language Specification

### Types
- `int`: 64-bit signed integer (`int64_t` in C).
- `float`: 64-bit floating point (`double` in C).
- `bool`: boolean (`true` / `false`).
- `string`: length-prefixed immutable byte array.
- `array[T]`: dynamic array of type `T`.
- `map[K, V]`: hash map mapping key type `K` to value type `V`.
- Custom structs.

### Functions and Variable Declarations
```slop
let name: string = "Slop"
let age = 0  # Type inference

struct Point {
    x: int,
    y: int
}

fn double_point(p: Point) -> Point {
    return Point(p.x * 2, p.y * 2)
}
```

### Built-in Standard Library Functions
- `print(val)`: Print value to standard output.
- `length(col)`: Length of string, array, or map.
- `push(arr, val)`: Push element to dynamic array.
- `read_file(path)`: Read a text file's contents into a string.
- `write_file(path, content)`: Write string contents to a file.
