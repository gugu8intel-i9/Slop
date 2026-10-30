# Slop

Slop is a small, fast programming language with syntax that is easier than Python.

It stands on its own. There is no C compiler, assembler, linker, Python or libc anywhere in the chain:

- The compiler (`compiler.slop`) is written in Slop.
- It writes x86-64 machine code and the Linux executable file directly.
- The runtime (memory, strings, printing, files, networking) is also Slop code, built into the compiler. It talks to the Linux kernel through system calls.
- `bin/slop` is the compiler, built by itself from `compiler.slop`.

```slop
names = ["Gugu", "Slop", "World"]
for name in names {
    print("Hello " + name)
}
```

```bash
bin/slop run hello.slop
```

## Get started

Slop runs on Linux x86-64. Nothing else needs to be installed.

```bash
git clone https://github.com/gugu8intel-i9/Slop
```

```bash
cd Slop
```

```bash
bin/slop run examples/hello.slop
```

To use `slop` from anywhere, copy it onto your PATH:

```bash
cp bin/slop ~/.local/bin/
```

| Command | What it does |
|---|---|
| `slop app.slop` | builds `./app` |
| `slop app.slop out` | builds `./out` |
| `slop run app.slop [args]` | builds and runs |

## The language in one page

```slop
# variables: no `let` needed (it still works), no semicolons
name = "Gugu"
age = 18
scores: array[int] = []          # a type when you want one

# functions
fn greet(who: string) -> string {
    return "Hello " + who
}

# if / else if / else, and / or / not
if age >= 18 and name != "" {
    print(greet(name))
} else if age > 12 {
    print("teen")
} else {
    print("young")
}

# loops
for i in 0..3 {
    push(scores, i * 10)
}
for s in scores {
    print(s)
}
total = 0
while total < 100 {
    total += 25
}

# lists
nums = [5, 3, 8]
push(nums, 1)
last = pop(nums)
evens = [x for x in nums if x % 2 == 0]
print(nums, len(nums))           # print takes any number of values

# structs with methods
struct Point {
    x: int,
    y: int

    fn sum() -> int {
        return this.x + this.y
    }
}
p = Point(3, 4)
print(p.sum())

# match
match name {
    "Gugu" => {
        print("hi Gugu")
    }
    else => {
        print("hi stranger")
    }
}

# pipes: x |> f(a) is f(x, a)
print(16 |> int_to_string() |> len())
```

Code outside functions runs from top to bottom, and its variables are shared with every function. `fn main(args: array[string])` is optional; if it exists it runs after the top-level code.

Types: `int`, `bool`, `string`, `array[T]` and your structs. Numbers can be written as `1_000_000` or `0xff`. Operators: `+ - * / %`, `== != < > <= >=`, `and or not`, `& | ^ << >>`, `+= -= *= /=`.

### Built-in functions

| Area | Functions |
|---|---|
| Output | `print(a, b, ...)`, `eprint(s)`, `input()` |
| Text | `len`, `int_to_string` (or `str`), `to_int`, `char_at`, `byte_at`, `chr`, `slice`, `split`, `join`, `find`, `contains`, `starts_with`, `ends_with`, `trim`, `upper`, `lower`, `replace`, `repeat`, `s[i]` |
| Lists | `len`, `push`, `pop`, `a[i]`, `for x in a` |
| Files | `read_file`, `write_file`, `make_executable` |
| Programs | `exit(code)`, `exec(path, args)`, `run(path, args)` (returns the output), `run_status()`, `clock_ns()` |
| Network | `socket_listen`, `socket_accept`, `socket_read`, `socket_write`, `socket_close` |
| Packing | `slop_pack(list)`, `slop_unpack(packed)` |
| Low level | see [LOW_LEVEL.md](LOW_LEVEL.md) |

## How it is fast and small

- **Two-bucket memory.** Each function call gets its memory from one of two buckets: calls at even depth use one, calls at odd depth use the other. When a call returns, everything it made is freed at once, and only its return value is copied to the caller's bucket. There is no garbage collector, no reference counting and no `free`, and memory grows with live data, not with how deep calls go.
- **Calls cost nothing extra when nothing is allocated.** Functions that don't allocate skip the memory bookkeeping entirely.
- **Variables in registers.** Whole-number variables and loop counters live in CPU registers.
- **Strings that don't copy.** String literals point into the program file, and `slice`, `split`, `char_at` and `for ch in s` return views of the original text. `s = s + x` grows `s` in place when it can, so building text in a loop is not quadratic.
- **Lists hold numbers directly**, 8 bytes per item, and grow in place when they're the newest thing in memory.
- **No libc.** Programs talk to the kernel directly, output is buffered, and a hello-world is about 3 KB.

Measured on this repo's test machine (Intel Xeon 2.1 GHz):

| Workload | Slop | C (gcc -O2) |
|---|---|---|
| `fib(35)` | 0.07 s, 44 KB peak memory | 0.10 s, 1.8 MB |
| Compiling the Slop compiler (4,500 lines) | 0.02 s | |

Run `bin/slop run examples/benchmark.slop` to time a few more workloads on your machine.

## Safety

- Every list index is bounds-checked; a bad index stops the program with a clear message.
- `read_file` and `write_file` refuse paths containing `..`.
- Memory freed when a call returns is wiped with zeros.

## Repository

| Path | What it is |
|---|---|
| `compiler.slop` | the compiler and runtime, in Slop |
| `bin/slop` | the compiler, built from `compiler.slop` by itself |
| `examples/` | example programs |
| `tests/` | the test suite (`bin/slop run tests/run.slop`) |
| [LEARN_SLOP_IN_10_MINUTES.md](LEARN_SLOP_IN_10_MINUTES.md) | a short tutorial |
| [SLOP_CHEATSHEET.md](SLOP_CHEATSHEET.md) | one-page reference |
| [LOW_LEVEL.md](LOW_LEVEL.md) | raw memory and CPU access |

After changing `compiler.slop`, rebuild the compiler with itself and run the tests:

```bash
bin/slop compiler.slop bin/slop
```

```bash
bin/slop run tests/run.slop
```

The tests check that the compiler reproduces itself byte for byte and that `bin/slop` matches `compiler.slop`.

## Not supported yet

Floating-point numbers, threads, and platforms other than Linux x86-64.

## License

GPL-3.0, see [LICENSE](LICENSE).
