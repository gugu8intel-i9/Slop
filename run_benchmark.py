#!/usr/bin/env python3
import os
import shutil
import subprocess
import time

ROOT = os.path.abspath(os.path.dirname(__file__))


def run_cmd(args, cwd=ROOT):
    start_time = time.perf_counter()
    p = subprocess.Popen(args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    stdout, stderr = p.communicate()
    elapsed = time.perf_counter() - start_time
    return p.returncode, elapsed, stdout.strip(), stderr.strip()


def compile_cmd(name, args):
    print(f"Compiling {name}...")
    code, elapsed, stdout, stderr = run_cmd(args)
    if stdout:
        print(stdout)
    if stderr:
        print(stderr)
    if code != 0:
        print(f"{name}: compile failed")
        return False
    print(f"{name}: compile OK ({elapsed:.3f}s)")
    return True


def run_bench(name, exe):
    print(f"\nRunning {name} 10B dead-counter optimized benchmark...")
    code, wall, stdout, stderr = run_cmd([exe])
    if stdout:
        print(stdout)
    if stderr:
        print(stderr)
    print(f"Harness Wall Time: {wall:.6f} seconds")
    if code != 0:
        print(f"{name}: run failed with exit {code}")
    return code == 0


def main():
    print("=== [Slop/C++/Rust/Go: 10 Billion Dead Counter Loop] ===")
    print("The counter is intentionally not used after the loop. Optimized compilers may delete the loop entirely.\n")

    # C++ via GCC/G++
    if shutil.which("g++"):
        if compile_cmd("C++", ["g++", "-O3", "-std=c++17", "-ffast-math", "-flto", "-march=native", "benchmark_cpp.cpp", "-o", "benchmark_cpp"]):
            run_bench("C++", "./benchmark_cpp")
    else:
        print("Skipping C++: g++ not found")

    # Slop via native self-hosted compiler when present, falling back to one-time bootstrapper in dev checkouts.
    slop_compiler = "./slop-compiler" if os.access(os.path.join(ROOT, "slop-compiler"), os.X_OK) else None
    if slop_compiler:
        slop_to_c = [slop_compiler, "benchmark_slop.slop", "benchmark_slop.c"]
    else:
        slop_to_c = ["python3", "slop_boot.py", "benchmark_slop.slop", "benchmark_slop.c"]

    if shutil.which("gcc"):
        if compile_cmd("Slop-to-C", slop_to_c):
            if compile_cmd("Slop native", ["gcc", "-O3", "-std=gnu11", "-ffast-math", "-flto", "-march=native", "-I.", "benchmark_slop.c", "-o", "benchmark_slop"]):
                run_bench("Slop", "./benchmark_slop")
    else:
        print("Skipping Slop native build: gcc not found")

    # Rust / Go are optional in lightweight environments.
    if shutil.which("rustc"):
        if compile_cmd("Rust", ["rustc", "-C", "opt-level=3", "-C", "target-cpu=native", "benchmark_rust.rs", "-o", "benchmark_rust"]):
            run_bench("Rust", "./benchmark_rust")
    else:
        print("\nSkipping Rust: rustc not found")

    if shutil.which("go"):
        if compile_cmd("Go", ["go", "build", "-o", "benchmark_go", "benchmark_go.go"]):
            run_bench("Go", "./benchmark_go")
    else:
        print("Skipping Go: go not found")


if __name__ == "__main__":
    main()
