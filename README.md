# Slop

Slop is a small, fast programming language with syntax that is easier than Python.

It stands on its own. There is no C compiler, assembler, linker, Python or libc anywhere in the chain:

- The compiler (`compiler.slop`) is written in Slop.
- It writes machine code (x86-64 or ARM64) and the executable file directly: ELF for Linux, Mach-O for macOS, PE (`.exe`) for Windows.
- The runtime (`runtime/`: memory, strings, numbers, math, threads, printing, files, processes, networking) is also Slop code, built into every program. It talks to the operating system directly: Linux system calls, macOS's libSystem, Windows's kernel32.
- `bin/` holds the compiler for each system, built by itself from `compiler.slop`.

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

Slop runs on Linux, macOS and Windows, on both Intel/AMD and ARM64 machines. Nothing else needs to be installed.

```bash
git clone https://github.com/gugu8intel-i9/Slop
```

```bash
cd Slop
```

| System | Compiler | Try it |
|---|---|---|
| Linux (Intel/AMD) | `bin/slop` | `bin/slop run examples/hello.slop` |
| Linux (ARM64, e.g. Raspberry Pi 4/5) | `bin/slop-linux-arm64` | `bin/slop-linux-arm64 run examples/hello.slop` |
| macOS (Apple Silicon) | `bin/slop-macos-arm64` | `bin/slop-macos-arm64 run examples/hello.slop` |
| macOS (Intel) | `bin/slop-macos` | `bin/slop-macos run examples/hello.slop` |
| Windows (Intel/AMD) | `bin\slop.exe` | `bin\slop.exe run examples/hello.slop` |
| Windows (ARM64, e.g. Surface, Snapdragon laptops) | `bin\slop-windows-arm64.exe` | `bin\slop-windows-arm64.exe run examples/hello.slop` |

To use `slop` from anywhere, let it install itself (it tells you if a folder needs adding to your PATH):

```bash
bin/slop install
```

### The `slop` command

| Command | What it does |
|---|---|
| `slop run app.slop [args]` | builds and runs a program |
| `slop build app.slop` | builds `./app` (`app.exe` on Windows); `-o name` picks the name |
| `slop check app.slop` | looks for mistakes without building |
| `slop new hello` | starts a new program in `hello.slop` |
| `slop repl` | type Slop code line by line and see results right away |
| `slop install` | copies `slop` into a folder for programs on your computer |
| `slop add NAME` / `slop remove NAME` | adds or removes a package (see below) |
| `slop list` / `slop search [word]` | shows your packages / finds packages |
| `slop std [NAME]` | lists the standard library / shows one module |
| `slop cert [host]` | makes `cert.pem` and `key.pem` for trying https locally |
| `slop convert hello.rs` | turns code in another language into Slop (see below) |
| `slop sync` / `slop update` | gets missing packages / gets the newest versions |
| `slop version` / `slop help` | shows the version / all commands |
| `--target SYSTEM` | builds for another system from any system: `linux`, `linux-arm64`, `macos`, `macos-arm64`, `windows`, `windows-arm64` |

`slop app.slop` (with no command) still builds `./app`.

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

# floats: whole numbers turn into floats when mixed with them
r = 2.5
print(3.14159 * r * r, 7 / 2, 7 / 2.0)   # 19.6349375 3 3.5
print(sqrt(2), round(2.5), int(9.99))   # 1.4142135623730951 3.0 9

# threads: spawn runs a call on another CPU core, join waits for its result
fn work(n: int) -> int {
    return n * n
}
jobs = [spawn work(i) for i in 0..4]
for j in jobs {
    print(join(j))
}
```

Code outside functions runs from top to bottom, and its variables are shared with every function. `fn main(args: array[string])` is optional; if it exists it runs after the top-level code.

Types: `int`, `float`, `bool`, `string`, `array[T]`, `thread[T]` and your structs. Numbers can be written as `1_000_000`, `0xff`, `2.5` or `6.02e23`. Floats print the shortest text that reads back as the same number, like Python. Operators: `+ - * / %`, `== != < > <= >=`, `and or not`, `& | ^ << >>`, `+= -= *= /=`.

### Built-in functions

| Area | Functions |
|---|---|
| Output | `print(a, b, ...)`, `eprint(s)`, `input()` |
| Text | `len`, `int_to_string` (or `str`), `to_int`, `char_at`, `byte_at`, `chr`, `slice`, `split`, `join`, `find`, `contains`, `starts_with`, `ends_with`, `trim`, `upper`, `lower`, `replace`, `repeat`, `s[i]` |
| Numbers | `float(x)`, `int(x)` (cuts off the fraction), `abs`, `sqrt`, `floor`, `ceil`, `round`, `pow`, `exp`, `log`, `sin`, `cos`, `tan`, `atan`, `float_bits`, `float_from_bits` |
| Lists | `len`, `push`, `pop`, `a[i]`, `for x in a` |
| Threads | `spawn f(args)`, `join(t)`, `lock()`, `unlock()`, `cpu_count()` |
| Files | `read_file`, `write_file`, `remove_file`, `make_dir`, `make_executable` |
| Programs | `exit(code)`, `exec(path, args)`, `run(path, args)` (returns the output), `run_status()`, `clock_ns()`, `env(name)`, `program_path()`, `input_ended()` |
| Network | `socket_listen`, `socket_accept`, `socket_connect(ip, port)`, `socket_read`, `socket_write`, `socket_close` (or `use "web"`) |
| Packing | `slop_pack(list)`, `slop_unpack(packed)` |
| Low level | see [LOW_LEVEL.md](LOW_LEVEL.md) |

## How it is fast and small

- **Two-bucket memory.** Each function call gets its memory from one of two buckets: calls at even depth use one, calls at odd depth use the other. When a call returns, everything it made is freed at once, and only its return value is copied to the caller's bucket. There is no garbage collector, no reference counting and no `free`, and memory grows with live data, not with how deep calls go.
- **Calls cost nothing extra when nothing is allocated.** Functions that don't allocate skip the memory bookkeeping entirely.
- **Variables in registers.** Whole-number variables and loop counters live in CPU registers.
- **Strings that don't copy.** String literals point into the program file, and `slice`, `split`, `char_at` and `for ch in s` return views of the original text. `s = s + x` grows `s` in place when it can, so building text in a loop is not quadratic.
- **Lists hold numbers directly**, 8 bytes per item, and grow in place when they're the newest thing in memory.
- **Threads without locks on memory.** Every thread has its own memory buckets, so threads never wait on each other to get memory. `print` keeps each line whole.
- **No C library.** Programs talk to the operating system directly, output is buffered, and a hello-world is about 3 KB on Linux.

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
| `compiler.slop` | the compiler, in Slop |
| `runtime/` | the runtime built into every program: `core.slop` plus one file per system |
| `bin/` | the compiler for each system (`slop`, `slop-linux-arm64`, `slop-macos`, `slop-macos-arm64`, `slop.exe`, `slop-windows-arm64.exe`) |
| `std/` | the standard library, in Slop |
| `tools/convert.slop` | `slop convert`, in Slop |
| `packages/` | the package list and the packages that come with Slop (`stats`, `text`, `colors`) |
| `examples/` | example programs |
| `tests/` | the test suite (`bin/slop run tests/run.slop`) |
| [LEARN_SLOP_IN_10_MINUTES.md](LEARN_SLOP_IN_10_MINUTES.md) | a short tutorial |
| [SLOP_CHEATSHEET.md](SLOP_CHEATSHEET.md) | one-page reference |
| [LOW_LEVEL.md](LOW_LEVEL.md) | raw memory and CPU access |

After changing `compiler.slop` or `runtime/`, rebuild the compilers (from any system) and run the tests:

```bash
bin/slop compiler.slop bin/slop
```

```bash
bin/slop --target linux-arm64 compiler.slop bin/slop-linux-arm64
```

```bash
bin/slop --target macos compiler.slop bin/slop-macos
```

```bash
bin/slop --target macos-arm64 compiler.slop bin/slop-macos-arm64
```

```bash
bin/slop --target windows compiler.slop bin/slop.exe
```

```bash
bin/slop --target windows-arm64 compiler.slop bin/slop-windows-arm64.exe
```

```bash
bin/slop run tests/run.slop
```

The tests check that the compiler reproduces itself byte for byte and that all six compilers in `bin/` match the source. GitHub Actions runs them on Linux, macOS and Windows, on both Intel/AMD and ARM64, for every push.

## Websites

`use "web"` turns a program into a website. Write a `handle` function that answers each request, then call `serve`:

```slop
use "web"

fn handle(req: Request) -> Response {
    if route(req, "GET", "/") {
        return html(page("Home", "<h1>Hello!</h1>"))
    }
    if route(req, "GET", "/hello/:name") {
        return text("Hi " + param("name"))
    }
    if route(req, "POST", "/signup") {
        return html(page("Thanks", render("<p>Welcome, {{who}}!</p>", ["who", form(req, "name")])))
    }
    return not_found()
}

serve(8080)    # open http://localhost:8080
```

| To | Use |
|---|---|
| answer | `html(text)`, `text(text)`, `json(text)`, `redirect(url)`, `not_found()`, `send_file(path)`, `send_dir(req, "/static/", "public")`, `with_status(r, 201)`, `with_header(r, name, value)`, `set_cookie(r, name, value)` |
| read the request | `route(req, method, "/path/:part")`, `param(name)`, `query(req, key)`, `form(req, key)`, `header(req, name)`, `cookie(req, name)`, `req.method`, `req.path`, `req.body` |
| make pages | `page(title, body)` (a full page with a clean look), `render(template, ["key", value, ...])` (`{{key}}` is made safe for HTML, `{{{key}}}` is put in as is), `render_file(path, pairs)`, `escape_html` |
| JSON | `json_string(s)`, `json_object(["key", json_text, ...])`, `json_list(items)` |
| ask other servers | `http_get(url)`, `http_post(url, form_data)`, `http_status()`, `http_error()` (http:// and https://) |
| https | `serve_https(port, cert, key)`, `make_cert(host, cert, key)`, `trust_cert(cert)` |

- `slop new mysite --web` starts a website you can build on; `examples/web_server.slop` is a small notes app.
- Requests are answered one at a time; `serve_count(port, n)` stops after `n` requests, and `quiet()` turns off the request log.

### https

Slop speaks https itself (TLS 1.3 and 1.2, written in Slop like everything else):

```slop
serve_https(443, "cert.pem", "key.pem")      # instead of serve(8080)
page = http_get("https://example.com/")      # checks the site's certificate
```

- `slop cert` (or `make_cert("localhost", "cert.pem", "key.pem")`) makes a self-signed certificate for trying https on your computer. Browsers warn about self-signed certificates; `trust_cert("cert.pem")` makes `http_get` accept yours.
- For a real site, use a certificate from a certificate authority such as Let's Encrypt. RSA keys (2048 to 4096 bits) and ECDSA keys (P-256, P-384) both work; ECDSA makes each new connection faster. Pass the `fullchain.pem` and `privkey.pem` files.
- `http_get` checks certificates against the ones your system trusts (ECDSA and RSA, with expiry dates and the host name) and refuses the connection if anything is wrong; `http_error()` says why.
- Slop uses TLS 1.3 whenever the other side can, and TLS 1.2 for older browsers and servers (with the extended master secret, and the check that stops an attacker from pushing a 1.3 connection down to 1.2). TLS 1.1 and older are not supported: they are no longer safe.
- Ciphers: ChaCha20-Poly1305 (preferred) and AES-128-GCM. Key exchange: X25519 (preferred) and P-256.

## Bring code from other languages

`slop convert` reads a program written in Rust, C, C++, Java, C#, JavaScript, TypeScript, Go, Kotlin, Swift or Python and writes the Slop version next to it:

```rust
fn main() {
    println!("Hello");
}
```

```bash
slop convert hello.rs
```

```slop
# converted from hello.rs by slop convert
fn main() {
    print("Hello")
}
```

- It translates functions, variables and types, printing (`println!`, `printf`, `cout`, `System.out.println`, `console.log`, `fmt.Println`, f-strings, `${}` and `{}` formats), if/else, loops (including C-style `for`), `switch`/`match`, `a ? b : c`, structs (and Rust `impl` and Go methods), and common string and list methods.
- Anything it cannot translate (classes, pointers, exceptions, closures, ...) is kept as a `# TODO:` comment, so nothing is lost silently.
- It then checks the result and says whether it builds. Python and JavaScript have no types, so it guesses them from how the code uses its values; the line says so when it guessed.
- The language comes from the file's extension; `--from python` (or `rust`, `c`, `cpp`, `java`, `csharp`, `js`, `go`, `kotlin`, `swift`) sets it by hand. `-o name.slop` picks the output file.

## Standard library

Modules that come with Slop, built into `slop` itself (nothing to download). Put `use "NAME"` at the top of a program:

| Module | What it has |
|---|---|
| `web` | websites: `serve`, `serve_https`, `route`, `html`, `page`, `render`, `form`, `query`, `json_object`, `http_get` (see Websites) |
| `crypto` | `sha256`, `sha384`, `sha512`, `hmac`, `hkdf_extract`/`hkdf_expand`, `chacha20_poly1305_seal`/`open`, `aes128_gcm_seal`/`open`, `x25519`, `p256_shared`, `ecdsa_sign`/`ecdsa_verify` (P-256, P-384), `rsa_sign_pss`/`rsa_sign_pkcs1`, `rsa_verify_pss`/`rsa_verify_pkcs1`, `random_bytes`, `to_hex`, `to_bytes` |
| `tls` | TLS 1.3 connections, certificates and keys (used by `web`): `tls_connect`, `tls_accept`, `parse_cert`, `pem_blocks`, `base64_encode`/`decode` |
| `math` | `PI`, `E`, `gcd`, `lcm`, `is_prime`, `factorial`, `ipow`, `clamp`, `lerp`, `hypot`, `radians`, `min_int`/`max_float`, ... |
| `lists` | `sum_ints`, `sort_ints`, `sort_floats`, `sort_strings`, `reverse_ints`, `unique_ints`, `index_of_int`, `range_list`, ... |
| `strings` | `pad_left`, `center`, `title_case`, `capitalize`, `words`, `lines`, `count`, `is_digit`, `trim_left`, `strip_suffix`, ... |
| `random` | `random_int`, `random_float`, `coin_flip`, `pick_string`, `shuffle_ints`, `seed` |
| `time` | `sleep(ms)`, `now_ms`, `elapsed_ms`, `format_duration` |
| `files` | `read_lines`, `write_lines`, `append_file`, `join_path`, `file_name`, `extension`, `exists` |
| `csv` | `read_csv`, `parse_csv`, `write_csv`, `to_csv_line` |
| `testing` | `check`, `check_int`, `check_float`, `check_string`, `test_summary` |

```slop
use "random"
use "lists"

roll = random_int(1, 6)
print("you rolled", roll)
print(sort_ints([5, 3, 9, 1]))
```

`slop std` lists the modules; `slop std NAME` shows everything in one.

A function's variables stay its own: a plain `x = 1` inside a function changes a global `x` only when that global is in the same file. So a library can never change your program's variables by accident (it can still read them, like `PI`).

## Packages

Put `use` lines at the top of a program to use code from other files:

```slop
use "stats"          # a package
use "helpers.slop"   # a file next to this one

print(median([3.0, 1.0, 2.0]))
```

```bash
slop search          # see the package list
slop add stats       # download a package into slop_packages/
slop run app.slop
```

- `slop add` takes a name from the package list, `github:user/repo` (or `github:user/repo@v1.0`) for any repository, or a link to a `.slop` file. A package needs `NAME.slop` or `lib.slop` at its top.
- `slop add` writes the packages your program uses into `slop.pkg`. Commit that file; anyone can then run `slop sync` to get the same packages.
- Packages a package uses are added for it.
- To share a package, put it on GitHub. To list it in `slop search`, add a line to [packages/index.txt](packages/index.txt).
- Downloads use `curl` and `tar`, which come with Linux, macOS and Windows 10 and later.

## Using every CPU core

`parallel for` shares a loop out over all the CPU cores:

```slop
results: array[float] = []
for i in 0..1_000_000 {
    push(results, 0.0)
}
parallel for i in 0..1_000_000 {
    results[i] = sqrt(float(i)) * 2.5
}
```

- The range is cut into small blocks that are dealt out to one thread per core, so loops whose steps take different times stay balanced. The loop ends when every block is done.
- The body can read local variables (it gets copies), write into lists (`results[i] = ...`), and change globals inside `lock()` / `unlock()`. Changing a local variable is an error, since the change would be lost; so are `break` and `return`.
- Each `parallel for` starts its threads fresh, so use it for loops that do real work (thousands of steps or more).
- On a 4-core machine, counting primes below 3 million takes 316 ms instead of 1047 ms.

## Threads in short

- `t = spawn f(a, b)` starts `f(a, b)` on a new thread. `f` must return `int`, `float`, `bool` or nothing.
- `join(t)` waits for it and gives its result. Join each thread once.
- Global variables are shared. Wrap changes to shared values in `lock()` / `unlock()`.
- Join a thread before the function that started it returns if you passed it strings or lists.


## License

GPL-3.0, see [LICENSE](LICENSE).
