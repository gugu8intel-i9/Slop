#!/usr/bin/env python3
import os, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
COMPILER = os.path.join(ROOT, "bootstrap/prebuilt/linux-x86_64/slop-compiler")

def run(cmd):
    p = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(p.stdout, end="")
    return p.returncode

def main():
    files = sys.argv[1:] or ["easy_start.slop", "low_level_demo.slop", "native_backend_demo.slop"]
    compiler = COMPILER if os.path.exists(COMPILER) else os.path.join(ROOT, "slop-compiler")
    failed = 0
    with tempfile.TemporaryDirectory() as td:
        for f in files:
            src = f if os.path.isabs(f) else os.path.join(ROOT, f)
            base = os.path.join(td, os.path.basename(f).replace('.slop',''))
            print(f"== test {f} ==")
            if run([compiler, src, base + ".c"]): failed += 1; continue
            if run(["gcc", "-O3", "-std=gnu11", "-I", ROOT, base + ".c", "-o", base]): failed += 1; continue
            if run([base]): failed += 1
    return failed

if __name__ == "__main__":
    raise SystemExit(main())
