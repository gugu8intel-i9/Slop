# Slop One-Page Cheat Sheet

Slop is meant to be easier to start than Python or JavaScript.

## The whole beginner language

```slop
name = "Gugu"
age = 18

if age >= 18 and name != "" {
    print("adult")
} else if age > 12 {
    print("teen")
} else {
    print("young")
}

nums = [1, 2, 3]
push(nums, 4)

for n in nums {
    print(n)
}

total = 0
for i in 0..10 {
    total += i
}
print(total)
```

## Five rules

1. `x = 1` creates a variable (`let x = 1` also works).
2. `fn` creates a function. Code outside functions just runs; no `main` needed.
3. `{ }` means a block.
4. `for x in list { }` and `for i in 0..n { }` loop.
5. `print(x)` shows a value.

Extras: `and`, `or`, `not`, `+=`, `-=`, `len(list)`, and `x |> f()` (same as `f(x)`).

## Types

```slop
let n: int = 42
let f: float = 3.14
let ok: bool = true
let s: string = "hello"
let xs: array[int] = [1, 2, 3]
```

Most of the time, Slop can infer the type:

```slop
let n = 42
let s = "hello"
```

## Functions

```slop
fn add(a: int, b: int) -> int {
    return a + b
}
```

## Structs

```slop
struct Player {
    name: string,
    score: int
}

let p = Player("Gugu", 9001)
print(p.name)
```

## Arrays

```slop
let names: array[string] = ["Gugu", "Slop"]
push(names, "World")
print(names[0])
```

## Comprehensions

```slop
let doubled = [x * 2 for x in nums]
```

## Parallel

```slop
let doubled = parallel [x * 2 for x in nums]
```

## Mental model

Write code like a simple scripting language. Slop lowers it to SIR, optimizes it, then emits native/C backend code.

```text
simple Slop -> SIR -> optimizer -> backend
```
