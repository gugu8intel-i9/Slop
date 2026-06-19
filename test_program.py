def fib(n: int) -> int:
    if n <= 1:
        return n
    return (fib((n - 1)) + fib((n - 2)))

def main(args: list):
    let_val = 10
    result = fib(let_val)
    print("Fibonacci of 10 is:")
    print(result)
