# Learn Slop in 10 Minutes

Slop is designed to feel easier than Python and JavaScript while still compiling to small native programs. Slop is written in Slop: no C, Python or other language is involved.

The rule is:

> Write simple code. Slop makes it fast.

## 1. Hello world

```slop
print("Hello, Slop!")
```

Run:

```bash
bin/slop run hello.slop          # Linux (macOS: bin/slop-macos, Windows: bin\slop.exe)
```

Code outside functions just runs, top to bottom. No `main` needed.

## 2. Variables are obvious

Just write `name = value`.

```slop
name = "Gugu"
age = 18
fast = true
scores: array[int] = []
```

No `var`, `const`, `let` confusion like JavaScript. (`let name = "Gugu"` also works.)

## 3. Functions are simple

```slop
fn add(a: int, b: int) -> int {
    return a + b
}
```

Use it:

```slop
total = add(10, 5)
print(total)
```

## 4. If statements read naturally

```slop
if age >= 18 and fast {
    print("adult")
} else if age > 12 {
    print("teen")
} else {
    print("young")
}
```

No parentheses required. Use `and`, `or`, `not`.

## 5. Loops are predictable

```slop
for i in 0..5 {
    print(i)
}

for name in ["Ada", "Bob"] {
    print(name)
}

i = 0
while i < 5 {
    i += 1
}
```

`break` and `continue` work as you'd expect.

## 6. Arrays are easy

```slop
nums = [1, 2, 3]
push(nums, 4)
print(nums[0], len(nums))
print(nums)
```

Slop keeps bounds checks so unsafe indexing does not become a memory exploit.

## 7. Strings are simple

```slop
first = "Slop"
second = "rocks"
print(first + " " + second)
print(upper(first), first[0], split("a,b", ","))
```

## 8. Structs are clean

```slop
struct Player {
    name: string,
    score: int

    fn shout() -> string {
        return upper(this.name)
    }
}

p = Player("Gugu", 9001)
print(p.name, p.score, p.shout())
```

## 9. List comprehensions are Python-easy

```slop
nums = [1, 2, 3, 4]
doubled = [x * 2 for x in nums]
evens = [x for x in nums if x % 2 == 0]
```

Behind the scenes these become plain machine-code loops.

## 10. Match

```slop
match command {
    "start" => {
        print("starting")
    }
    "stop" | "quit" => {
        print("bye")
    }
    else => {
        print("unknown")
    }
}
```

## 11. The mental model

You only need to remember five things:

1. `name = value` creates variables.
2. `fn` creates functions.
3. `{ }` creates blocks.
4. `array[type]` means a list.
5. `bin/slop run file.slop` builds and runs it.

That is the beginner path.

## Why it can be easier than Python/JS

| Problem in Python/JS | Slop answer |
|---|---|
| Runtime type surprises | simple static types + inference |
| Slow hot loops | native compilation |
| JS `var`/`let`/`const` confusion | only `let` |
| Python package/runtime environment issues | one small binary with no dependencies |
| Memory safety footguns in C/C++ | bounds checks + automatic memory |

## Tiny complete program

```slop
fn greet(name: string) {
    print("Hello " + name)
}

for name in ["Gugu", "Slop", "World"] {
    greet(name)
}
```

Simple code. Native output.
