#!/usr/bin/env python3
import sys
import os
import subprocess
import tempfile

BLUE = '\033[0;34m'
GREEN = '\033[0;32m'
RED = '\033[0;31m'
NC = '\033[0m'

def main():
    print(f"{BLUE}=== Slop Programming Language Interactive REPL Shell ==={NC}")
    print("Type your Slop code line-by-line. Press Enter on an empty line to execute.")
    print("Type 'exit' to quit.")
    print()

    history = []
    
    # Locate runtime include
    slop_dir = os.path.expanduser("~/.slop")
    slop_include = os.path.join(slop_dir, "include")
    
    # If not globally installed, use local directory
    if not os.path.exists(slop_include):
        slop_include = os.path.abspath(os.path.dirname(__file__))

    while True:
        try:
            line = input("slop> ")
        except (KeyboardInterrupt, EOFError):
            print()
            break

        if line.strip() == "exit":
            break
        if line.strip() == "":
            continue

        # If it's a simple expression, wrap it in a print statement so they see results
        code_to_compile = line
        is_expr = False
        # If it doesn't contain '=' or 'let' or 'fn' or 'print' or 'while' or 'if', assume it's an expression
        if not any(k in line for k in ["=", "let ", "fn ", "print", "while", "if", "struct", "match", "raw"]):
            code_to_compile = f"print({line})"
            is_expr = True

        # Build full temporary Slop program
        slop_program = f"""
fn main(args: array[string]) {{
    {code_to_compile}
}}
"""
        # Create temp files to compile and run
        with tempfile.TemporaryDirectory() as tmpdir:
            slop_file = os.path.join(tmpdir, "repl.slop")
            c_file = os.path.join(tmpdir, "repl.c")
            binary = os.path.join(tmpdir, "repl")

            with open(slop_file, "w") as f:
                f.write(slop_program)

            # 1. Transpile to C
            try:
                # We can import slop_boot dynamically or run as subprocess
                # Subprocess is simpler and avoids import path issues
                subprocess.run(
                    [sys.executable, os.path.join(slop_include, "slop_boot.py"), slop_file, c_file],
                    check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
                )
            except subprocess.CalledProcessError as e:
                print(f"{RED}Transpiler Error:{NC}")
                print(e.stderr.decode().strip())
                continue

            # 2. Compile to binary
            try:
                subprocess.run(
                    ["gcc", "-O3", "-ffast-math", f"-I{slop_include}", c_file, "-o", binary],
                    check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
                )
            except subprocess.CalledProcessError as e:
                print(f"{RED}Compilation Error (Type Mismatch or Syntax Error):{NC}")
                # Filter out raw C compiler noise to keep it extremely user-friendly
                err_lines = e.stderr.decode().strip().split("\n")
                for err_line in err_lines:
                    if "error:" in err_line or "warning:" in err_line:
                        print(err_line)
                continue

            # 3. Run and print output
            try:
                res = subprocess.run([binary], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                output = res.stdout.decode().strip()
                if output:
                    print(f"{GREEN}{output}{NC}")
            except subprocess.CalledProcessError as e:
                print(f"{RED}Runtime Error:{NC}")
                print(e.stderr.decode().strip())

if __name__ == "__main__":
    main()
