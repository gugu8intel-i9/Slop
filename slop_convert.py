#!/usr/bin/env python3
import sys
import os
import re
from slop_translate import translate_python_to_slop

BLUE = '\033[0;34m'
GREEN = '\033[0;32m'
RED = '\033[0;31m'
NC = '\033[0m'

# Maps standard C/C++ and Rust types to native Slop types
TYPE_MAP = {
    "int": "int", "int64_t": "int", "uint64_t": "int", "long": "int", "long long": "int", "size_t": "int",
    "double": "float", "float": "float",
    "bool": "bool", "_Bool": "bool",
    "char*": "string", "const char*": "string", "SlopString": "string",
    "void": "void",
    "i64": "int", "u64": "int", "isize": "int", "usize": "int", "f64": "float", "f32": "float"
}

def clean_type(t):
    t = t.strip()
    # Normalize spaces and pointers
    t = re.sub(r'\s+', ' ', t)
    # Check map
    return TYPE_MAP.get(t, "int")

def convert_c_cpp_header(content, filename):
    # Regex to capture standard function prototypes: e.g. "int add_numbers(int a, double b);"
    # Matches: [return_type] [func_name] ( [args...] ) ;
    pattern = r'([a-zA-Z0-9_\*\s]+)\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*;'
    matches = re.findall(pattern, content)
    
    slop_code = []
    slop_code.append(f"# Automatically generated Slop bindings for C/C++ library: {filename}\n")
    slop_code.append(f"# Load header at compile time\n")
    slop_code.append(f"raw {{\n    \"#include \\\"{filename}\\\"\"\n}}\n\n")

    for ret_type_raw, func_name, args_raw in matches:
        ret_type_raw = ret_type_raw.strip()
        if "static" in ret_type_raw or "inline" in ret_type_raw:
            continue # Skip internal inline helpers
            
        ret_type = clean_type(ret_type_raw)
        
        # Parse arguments
        params = []
        call_args = []
        
        if args_raw.strip() and args_raw.strip() != "void":
            arg_list = args_raw.split(",")
            for i, arg in enumerate(arg_list):
                arg = arg.strip()
                # Split type and name
                parts = arg.split()
                if len(parts) >= 2:
                    p_name = parts[-1].replace("*", "")
                    p_type_raw = " ".join(parts[:-1])
                    p_type = clean_type(p_type_raw)
                else:
                    p_name = f"arg{i}"
                    p_type = "int"
                params.append(f"{p_name}: {p_type}")
                call_args.append(p_name)
                
        params_str = ", ".join(params)
        args_str = ", ".join(call_args)
        
        # Build safe FFI wrapper function in Slop using inline C raw blocks!
        slop_code.append(f"fn {func_name}({params_str}) -> {ret_type} {{\n")
        if ret_type != "void":
            slop_code.append(f"    let _res: {ret_type} = 0\n") # Default initializer
            slop_code.append(f"    raw {{\n        \"_res = {func_name}({args_str});\"\n    }}\n")
            slop_code.append(f"    return _res\n")
        else:
            slop_code.append(f"    raw {{\n        \"{func_name}({args_str});\"\n    }}\n")
        slop_code.append(f"}}\n\n")
        
    return "".join(slop_code)

def convert_rust_source(content, filename):
    # Regex to capture: "pub extern "C" fn add(a: i64, b: f64) -> i64"
    pattern = r'pub\s+extern\s+"C"\s+fn\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*(?:->\s*([a-zA-Z0-9_]+))?'
    matches = re.findall(pattern, content)
    
    slop_code = []
    slop_code.append(f"# Automatically generated Slop bindings for Rust library: {filename}\n")
    slop_code.append(f"# Link Rust library binary at compile time\n\n")

    for func_name, args_raw, ret_type_raw in matches:
        ret_type_raw = ret_type_raw.strip() if ret_type_raw else "void"
        ret_type = clean_type(ret_type_raw)
        
        # Parse arguments
        params = []
        call_args = []
        if args_raw.strip():
            arg_list = args_raw.split(",")
            for arg in arg_list:
                arg = arg.strip()
                if ":" in arg:
                    p_name, p_type_raw = arg.split(":")
                    p_name = p_name.strip()
                    p_type = clean_type(p_type_raw.strip())
                    params.append(f"{p_name}: {p_type}")
                    call_args.append(p_name)
                    
        params_str = ", ".join(params)
        args_str = ", ".join(call_args)
        
        # Forward declare the Rust symbol in C scope so GCC/Clang can compile it flawlessly
        c_ret = "int64_t" if ret_type == "int" else ("double" if ret_type == "float" else "void")
        c_args = []
        for p in params:
            p_name, p_type = p.split(":")
            c_type = "int64_t" if p_type.strip() == "int" else "double"
            c_args.append(f"{c_type} {p_name.strip()}")
            
        slop_code.append(f"raw {{\n    \"extern {c_ret} {func_name}({', '.join(c_args)});\"\n}}\n\n")
        
        # Build Slop wrapper
        slop_code.append(f"fn {func_name}({params_str}) -> {ret_type} {{\n")
        if ret_type != "void":
            slop_code.append(f"    let _res: {ret_type} = 0\n")
            slop_code.append(f"    raw {{\n        \"_res = {func_name}({args_str});\"\n    }}\n")
            slop_code.append(f"    return _res\n")
        else:
            slop_code.append(f"    raw {{\n        \"{func_name}({args_str});\"\n    }}\n")
        slop_code.append(f"}}\n\n")
        
    return "".join(slop_code)

def main():
    if len(sys.argv) < 2:
        print("Usage: slop_convert.py <library_file> [output.slop]", file=sys.stderr)
        print("Supported input formats: .py (Python), .h/.hpp (C/C++), .rs (Rust)", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file.replace(os.path.splitext(input_file)[1], ".slop")

    if not os.path.exists(input_file):
        print(f"Error: File not found '{input_file}'", file=sys.stderr)
        sys.exit(1)

    _, ext = os.path.splitext(input_file)
    
    with open(input_file, "r") as f:
        content = f.read()

    print(f"{BLUE}--- [Slop Universal Converter: Translating {input_file}] ---{NC}")

    if ext == ".py":
        slop_code = translate_python_to_slop(content)
    elif ext in [".h", ".hpp"]:
        slop_code = convert_c_cpp_header(content, os.path.basename(input_file))
    elif ext == ".rs":
        slop_code = convert_rust_source(content, os.path.basename(input_file))
    else:
        print(f"{RED}Error: Unsupported file format '{ext}'{NC}", file=sys.stderr)
        sys.exit(1)

    with open(output_file, "w") as f:
        f.write(slop_code)

    print(f"{GREEN}Successfully generated native Slop library file:{NC} {output_file}")

if __name__ == "__main__":
    main()
