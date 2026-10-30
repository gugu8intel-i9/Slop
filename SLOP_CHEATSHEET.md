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
n: int = 42
ok: bool = true
s: string = "hello"
xs: array[int] = [1, 2, 3]
```

Most of the time, Slop can infer the type:

```slop
n = 42
s = "hello"
```

Numbers: `1_000_000`, `0xff`. Floats are not supported yet.

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

    fn label() -> string {
        return this.name + ": " + str(this.score)
    }
}

p = Player("Gugu", 9001)
print(p.name, p.label())
p.score += 1
```

## Arrays

```slop
names: array[string] = ["Gugu", "Slop"]
push(names, "World")
print(names[0], len(names), pop(names))
```

## Comprehensions

```slop
doubled = [x * 2 for x in nums]
small = [x for x in nums if x < 10]
squares = [i * i for i in 0..10]
```

## Match

```slop
match n {
    0 => {
        print("zero")
    }
    1 | 2 => {
        print("small")
    }
    else => {
        print("big")
    }
}
```

## Built-ins

`print`, `eprint`, `input`, `len`, `push`, `pop`, `int_to_string`/`str`, `to_int`, `char_at`, `byte_at`, `chr`, `slice`, `split`, `join`, `find`, `contains`, `starts_with`, `ends_with`, `trim`, `upper`, `lower`, `replace`, `repeat`, `read_file`, `write_file`, `exit`, `run`, `exec`, `clock_ns`, `socket_*`, `slop_pack`, `slop_unpack`.

## Mental model

Write code like a simple scripting language. The Slop compiler (written in Slop) turns it straight into a small program for Linux, macOS or Windows: no C compiler, no linker, no C library.

```text
app.slop -> bin/slop -> ./app
```
