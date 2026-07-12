#!/usr/bin/env python3
# Sloppy - The Official Slop Package Manager & Project Builder
# Parses slop.toml manifests, fetches dependencies, and compiles optimized targets.

import sys
import os
import subprocess
import shutil

BLUE = '\033[0;34m'
GREEN = '\033[0;32m'
RED = '\033[0;31m'
NC = '\033[0m'

def parse_toml(content):
    # A simple, zero-dependency TOML parser for slop.toml
    config = {}
    current_section = None
    
    for line in content.split("\n"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1].strip()
            config[current_section] = {}
        elif "=" in line and current_section:
            key, val = line.split("=", 1)
            key = key.strip()
            val = val.strip().strip('"').strip("'")
            # Parse arrays
            if val.startswith("[") and val.endswith("]"):
                val = [item.strip().strip('"').strip("'") for item in val[1:-1].split(",") if item.strip()]
            config[current_section][key] = val
            
    return config

def init_project(project_name):
    print(f"{BLUE}Creating new Slop project: '{project_name}'...{NC}")
    if os.path.exists(project_name):
        print(f"{RED}Error: Directory '{project_name}' already exists!{NC}", file=sys.stderr)
        return
        
    os.makedirs(os.path.join(project_name, "src"))
    os.makedirs(os.path.join(project_name, "build"))
    
    # 1. Generate slop.toml manifest
    toml_content = f"""# slop.toml - Project Manifest for Slop
[package]
name = "{project_name}"
version = "0.1.0"
authors = ["everybody"]
description = "A high-performance Slop application"

[build]
target = "src/main.slop"
flags = ["-O3", "-ffast-math", "-flto", "-march=native"]

[dependencies]
# Add external Slop libraries here (e.g. from GitHub!)
# my_library = "https://github.com/user/my_library.git"
"""
    with open(os.path.join(project_name, "slop.toml"), "w") as f:
        f.write(toml_content)
        
    # 2. Generate boilerplate main.slop
    main_content = """# main.slop - Entry point for the application

fn main(args: array[string]) {
    print("Hello from your new Slop project!")
}
"""
    with open(os.path.join(project_name, "src", "main.slop"), "w") as f:
        f.write(main_content)
        
    # 3. Generate a local .gitignore
    gitignore = """# Slop build artifacts
build/
*.c
"""
    with open(os.path.join(project_name, ".gitignore"), "w") as f:
        f.write(gitignore)

    print(f"{GREEN}Successfully initialized Slop project!{NC}")
    print(f"Run it with: {BLUE}cd {project_name} && slop run{NC}")

def build_project():
    if not os.path.exists("slop.toml"):
        print(f"{RED}Error: Could not find slop.toml in the current directory!{NC}", file=sys.stderr)
        return False
        
    with open("slop.toml", "r") as f:
        content = f.read()
        
    config = parse_toml(content)
    pkg_name = config.get("package", {}).get("name", "app")
    target_file = config.get("build", {}).get("target", "src/main.slop")
    flags = config.get("build", {}).get("flags", ["-O3"])
    deps = config.get("dependencies", {})
    
    # Resolve global Slop installation directory
    slop_dir = os.path.expanduser("~/.slop")
    slop_bin = os.path.join(slop_dir, "bin")
    slop_include = os.path.join(slop_dir, "include")
    
    if not os.path.exists(slop_include):
        slop_include = os.path.abspath(os.path.dirname(__file__))

    print(f"{BLUE}=== Building Slop project: '{pkg_name}' ==={NC}")
    
    # 1. Fetch dependencies (simulating git cloning into ~/.slop/pkg)
    for dep_name, dep_url in deps.items():
        dep_dir = os.path.join(slop_dir, "pkg", dep_name)
        if not os.path.exists(dep_dir):
            print(f"Fetching dependency '{dep_name}' from {dep_url}...")
            os.makedirs(os.path.dirname(dep_dir), exist_ok=True)
            subprocess.run(["git", "clone", "--depth", "1", dep_url, dep_dir], check=True)
            
    # 2. Transpile to C
    os.makedirs("build", exist_ok=True)
    c_output = f"build/{pkg_name}.c"
    print(f"Compiling {target_file} to C with the native self-hosted compiler...")
    native_compiler = os.path.join(slop_bin, "slop-compiler")
    local_compiler = os.path.abspath("slop-compiler")
    if os.path.exists(native_compiler):
        compile_cmd = [native_compiler, target_file, c_output]
    elif os.path.exists(local_compiler):
        compile_cmd = [local_compiler, target_file, c_output]
    else:
        # Development fallback only: the production install uses native slop-compiler.
        compile_cmd = [sys.executable, os.path.join(slop_bin if os.path.exists(slop_bin) else slop_include, "slop_boot.py"), target_file, c_output]
    try:
        subprocess.run(compile_cmd, check=True)
    except subprocess.CalledProcessError:
        print(f"{RED}Error: Slop-to-C compilation failed!{NC}", file=sys.stderr)
        return False
        
    # 3. Compile native optimized binary
    binary_output = f"build/{pkg_name}"
    print(f"Compiling highly optimized native binary {binary_output}...")
    comp_cmd = ["gcc", "-std=gnu11"] + flags + [f"-I{slop_include}", c_output, "-o", binary_output]
    # Check if we need to link math or threads libraries
    with open(target_file, "r") as f:
        target_src = f.read()
        if "spawn" in target_src or "parallel" in target_src:
            comp_cmd.append("-lpthread")
        if "tensor" in target_src:
            comp_cmd.append("-lm")
            
    try:
        subprocess.run(comp_cmd, check=True)
    except subprocess.CalledProcessError:
        print(f"{RED}Error: Compilation failed!{NC}", file=sys.stderr)
        return False
        
    print(f"{GREEN}Build Successful! Native binary compiled at: {binary_output}{NC}")
    return True

def run_project():
    if build_project():
        with open("slop.toml", "r") as f:
            content = f.read()
        config = parse_toml(content)
        pkg_name = config.get("package", {}).get("name", "app")
        binary_output = f"./build/{pkg_name}"
        
        print(f"{BLUE}Executing {binary_output}...{NC}")
        print()
        subprocess.run([binary_output])

def main():
    if len(sys.argv) < 2:
        print("Sloppy - Slop Project Builder & Dependency Manager")
        print("Usage: sloppy <command> [arguments]")
        print("")
        print("Commands:")
        print("  new <project_name>   Initialize a new high-performance Slop project")
        print("  build                Compile current project using slop.toml manifest")
        print("  run                  Build and execute current project instantly")
        sys.exit(0)
        
    cmd = sys.argv[1]
    
    if cmd == "new":
        if len(sys.argv) < 3:
            print(f"{RED}Error: Please specify a project name!{NC}", file=sys.stderr)
            sys.exit(1)
        init_project(sys.argv[2])
    elif cmd == "build":
        build_project()
    elif cmd == "run":
        run_project()
    else:
        print(f"{RED}Error: Unknown Command '{cmd}'{NC}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
