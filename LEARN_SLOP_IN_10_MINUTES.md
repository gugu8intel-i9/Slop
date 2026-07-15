# Learn Slop in 10 Minutes

Slop is designed to feel easier than Python and JavaScript while still compiling to native code.

The rule is:

> Write simple code. Slop makes it fast.

## 1. Hello world

```slop
fn main(args: array[string]) {
    print("Hello, Slop!")
}
```

Run:

```bash
slop run hello.slop
```

## 2. Variables are obvious

Use `let`.

```slop
let name = "Gugu"
let age = 18
let fast = true
```

No `var`, `const`, `let` confusion like JavaScript. Just `let`.

## 3. Functions are simple

```slop
fn add(a: int, b: int) -> int {
    return a + b
}
```

Use it:

```slop
let total = add(10, 5)
print(total)
```

## 4. If statements read naturally

```slop
if age >= 18 {
    print("adult")
} else {
    print("young")
}
```

No parentheses required.

## 5. Loops are predictable

```slop
let i = 0
while i < 5 {
    print(i)
    i = i + 1
}
```

## 6. Arrays are easy

```slop
let nums = [1, 2, 3]
push(nums, 4)
print(nums[0])
```

Slop keeps bounds checks so unsafe indexing does not become a memory exploit.

## 7. Strings are simple

```slop
let first = "Slop"
let second = "rocks"
print(first + " " + second)
```

## 8. Structs are clean

```slop
struct Player {
    name: string,
    score: int
}

fn main(args: array[string]) {
    let p = Player("Gugu", 9001)
    print(p.name)
    print(p.score)
}
```

## 9. List comprehensions are Python-easy

```slop
let nums = [1, 2, 3, 4]
let doubled = [x * 2 for x in nums]
```

Behind the scenes Slop lowers this to fast native loops.

## 10. Parallel is one word

```slop
let doubled = parallel [x * 2 for x in nums]
```

The compiler/runtime handles workers and arena isolation.

## 11. The mental model

You only need to remember five things:

1. `let` creates variables.
2. `fn` creates functions.
3. `{ }` creates blocks.
4. `array[type]` means a list.
5. Slop compiles to native code.

That is the beginner path.

## Why it can be easier than Python/JS

| Problem in Python/JS | Slop answer |
|---|---|
| Runtime type surprises | simple static types + inference |
| Slow hot loops | native compilation |
| JS `var`/`let`/`const` confusion | only `let` |
| Python package/runtime environment issues | compile to native binary |
| Memory safety footguns in C/C++ | bounds checks + SEAA arenas |
| Manual thread boilerplate | `parallel` |

## Tiny complete program

```slop
fn greet(name: string) {
    print("Hello " + name)
}

fn main(args: array[string]) {
    let names = ["Gugu", "Slop", "World"]
    let i = 0
    while i < length(names) {
        greet(names[i])
        i = i + 1
    }
}
```

Simple code. Native output.
