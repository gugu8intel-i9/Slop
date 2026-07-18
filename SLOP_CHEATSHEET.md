# Slop One-Page Cheat Sheet

Slop is meant to be easier to start than Python or JavaScript.

## The whole beginner language

```slop
fn main(args: array[string]) {
    let name = "Gugu"
    let age = 18

    if age >= 18 {
        print("adult")
    } else {
        print("young")
    }

    let nums = [1, 2, 3]
    push(nums, 4)

    let i = 0
    while i < length(nums) {
        print(nums[i])
        i = i + 1
    }
}
```

## Five rules

1. `let` creates a variable.
2. `fn` creates a function.
3. `{ }` means a block.
4. `array[T]` means a list of `T`.
5. `print(x)` shows a value.

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
