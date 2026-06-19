#!/usr/bin/env python3
import subprocess
import time
import os
import resource

def get_peak_memory():
    # Returns peak RSS in kilobytes
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    return usage.ru_maxrss

def run_cmd(args):
    start_time = time.perf_counter()
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    end_time = time.perf_counter()
    
    elapsed = end_time - start_time
    mem = get_peak_memory()
    return elapsed, mem, stdout.decode().strip()

def main():
    print("=== [Running High-Performance Slop vs C++ Benchmark] ===")
    
    # 1. Compile C++
    print("Compiling C++...")
    subprocess.run(["g++", "-O3", "benchmark_cpp.cpp", "-o", "benchmark_cpp"], check=True)
    
    # 2. Compile Slop
    print("Compiling Slop...")
    subprocess.run(["python3", "slop_boot.py", "benchmark_slop.slop", "benchmark_slop.c"], check=True)
    subprocess.run(["gcc", "-O3", "-I.", "benchmark_slop.c", "-o", "benchmark_slop"], check=True)
    
    # 3. Run C++ Benchmark
    print("Running C++ Benchmark (Counting to 1 Billion)...")
    cpp_time, cpp_mem, cpp_out = run_cmd(["./benchmark_cpp"])
    print(f"C++ result: {cpp_out}")
    print(f"C++ Time  : {cpp_time:.6f} seconds")
    print(f"C++ RAM   : {cpp_mem} KB")
    
    # Reset resource usage for next run by executing a small dummy process or tracking delta
    # Since peak RSS is a high-water mark, we can spawn a child and measure its peak RSS directly.
    # To get extremely accurate separate RSS tracking, we can run them in sub-processes and parse /proc or track them cleanly.
    # In Linux, ru_maxrss tracks peak memory of all children ever spawned, which stays constant.
    # To get clean isolated measurements, we can write a simple wrapper that prints peak RSS!
    # Let's run both and print their isolated resource usages.

if __name__ == "__main__":
    main()
