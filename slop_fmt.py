#!/usr/bin/env python3
# SlopFmt - The Official Slop Code Formatter
# Parses Slop source code, standardizes spacing/braces/indentation, and overwrites in-place.

import sys
import os

class Token:
    def __init__(self, type_, value):
        self.type = type_
        self.value = value
    def __repr__(self):
        return f"Token({self.type}, {repr(self.value)})"

def tokenize(source):
    # Standard Slop Lexer for Formatter
    length = len(source)
    pos = 0
    tokens = []
    
    while pos < length:
        char = source[pos]
        
        # Whitespace tracking
        if char.isspace():
            if char == '\n':
                tokens.append(Token("NEWLINE", "\n"))
            pos += 1
            continue
            
        # Comments
        if char == '#':
            comment = []
            while pos < length and source[pos] != '\n':
                comment.append(source[pos])
                pos += 1
            tokens.append(Token("COMMENT", "".join(comment)))
            continue
            
        # Multi-character symbols
        if char == '-' and pos + 1 < length and source[pos+1] == '>':
            tokens.append(Token("ARROW", "->"))
            pos += 2
            continue
        if char == '=' and pos + 1 < length and source[pos+1] == '>':
            tokens.append(Token("FAT_ARROW", "=>"))
            pos += 2
            continue
        if char == '|' and pos + 1 < length and source[pos+1] == '>':
            tokens.append(Token("PIPE", "|>"))
            pos += 2
            continue
        if char == '=' and pos + 1 < length and source[pos+1] == '=':
            tokens.append(Token("EQ", "=="))
            pos += 2
            continue
        if char == '!' and pos + 1 < length and source[pos+1] == '=':
            tokens.append(Token("NEQ", "!="))
            pos += 2
            continue
        if char == '<' and pos + 1 < length and source[pos+1] == '=':
            tokens.append(Token("LEQ", "<="))
            pos += 2
            continue
        if char == '>' and pos + 1 < length and source[pos+1] == '=':
            tokens.append(Token("GEQ", ">="))
            pos += 2
            continue
            
        # Single-character symbols
        if char in "()[]{}:,;+-%*/=<>.":
            tokens.append(Token("SYMBOL", char))
            pos += 1
            continue
            
        # String literals
        if char == '"':
            val = ['"']
            pos += 1
            while pos < length and source[pos] != '"':
                if source[pos] == '\\' and pos + 1 < length:
                    val.append(source[pos:pos+2])
                    pos += 2
                else:
                    val.append(source[pos])
                    pos += 1
            val.append('"')
            pos += 1
            tokens.append(Token("STRING", "".join(val)))
            continue
            
        # Numeric literals
        if char.isdigit():
            val = []
            while pos < length and source[pos].isdigit():
                val.append(source[pos])
                pos += 1
            tokens.append(Token("NUMBER", "".join(val)))
            continue
            
        # Identifiers and keywords
        if char.isalpha() or char == '_':
            val = []
            while pos < length and (source[pos].isalnum() or source[pos] == '_'):
                val.append(source[pos])
                pos += 1
            name = "".join(val)
            if name in ["fn", "let", "if", "else", "while", "return", "struct", "true", "false", "match", "for", "in", "raw", "gpu"]:
                tokens.append(Token("KEYWORD", name))
            else:
                tokens.append(Token("IDENTIFIER", name))
            continue
            
        # Unknown characters
        pos += 1
        
    return tokens

def format_slop(source):
    tokens = tokenize(source)
    output = []
    indent_level = 0
    
    i = 0
    num_tokens = len(tokens)
    
    while i < num_tokens:
        tok = tokens[i]
        
        if tok.type == "NEWLINE":
            # Avoid writing duplicate trailing newlines
            if output and output[-1] != "\n":
                output.append("\n")
            i += 1
            continue
            
        if tok.type == "COMMENT":
            # Apply standard indent before comment
            if not output or output[-1] == "\n":
                output.append("    " * indent_level)
            output.append(tok.value + "\n")
            i += 1
            continue
            
        # If we are starting a new line, apply standard 4-space indentation
        if not output or output[-1] == "\n":
            # Decrement indentation if closing brace is next
            if tok.type == "SYMBOL" and tok.value == "}":
                indent_level = max(0, indent_level - 1)
            output.append("    " * indent_level)
            
        # Format operators and symbols with uniform spacing rules
        if tok.type in ["ARROW", "FAT_ARROW", "PIPE", "EQ", "NEQ", "LEQ", "GEQ"]:
            # Surround operators with standard spaces
            if output and output[-1] != " ":
                output.append(" ")
            output.append(tok.value + " ")
            
        elif tok.type == "SYMBOL":
            if tok.value in "+-*/%":
                if output and output[-1] != " ":
                    output.append(" ")
                output.append(tok.value + " ")
            elif tok.value == "=":
                if output and output[-1] != " ":
                    output.append(" ")
                output.append("= ")
            elif tok.value == ":":
                output.append(": ")
            elif tok.value == ",":
                output.append(", ")
            elif tok.value == "{":
                # Increment indentation level
                if output and output[-1] != " ":
                    output.append(" ")
                output.append("{\n")
                indent_level += 1
            elif tok.value == "}":
                output.append("}\n")
            else:
                output.append(tok.value)
                
        elif tok.type == "KEYWORD":
            # Ensure keywords are spaced out nicely
            output.append(tok.value + " ")
            
        else:
            # Identifiers, numbers, and strings are printed as-is
            output.append(tok.value)
            
        i += 1
        
    # Clean up and normalize spacing
    res = "".join(output)
    # Fix trailing spaces and double spaces
    res = re_normalize(res)
    return res

def re_normalize(code):
    # Simple regex cleaning to polish commas, colons, and braces spacing
    code = re_replace(code, r' +', ' ')
    code = re_replace(code, r' \n', '\n')
    code = re_replace(code, r'\n\n+', '\n\n')
    code = re_replace(code, r'\s*,\s*', ', ')
    code = re_replace(code, r'\s*:\s*', ': ')
    code = re_replace(code, r'\(\s*', '(')
    code = re_replace(code, r'\s*\)', ')')
    code = re_replace(code, r'\[\s*', '[')
    code = re_replace(code, r'\s*\]', ']')
    code = re_replace(code, r'fn\s+', 'fn ')
    code = re_replace(code, r'let\s+', 'let ')
    code = re_replace(code, r'if\s+', 'if ')
    code = re_replace(code, r'while\s+', 'while ')
    code = re_replace(code, r'return\s+', 'return ')
    code = re_replace(code, r'struct\s+', 'struct ')
    code = re_replace(code, r'match\s+', 'match ')
    code = re_replace(code, r'raw\s*', 'raw ')
    code = re_replace(code, r'gpu\s+', 'gpu ')
    return code.strip() + "\n"

def re_replace(text, pattern, replacement):
    import re
    return re.sub(pattern, replacement, text)

def main():
    if len(sys.argv) < 2:
        print("SlopFmt - Official Code Formatter for Slop")
        print("Usage: slopfmt <file.slop> [--check]")
        sys.exit(0)
        
    filepath = sys.argv[1]
    
    if not os.path.exists(filepath):
        print(f"Error: File not found '{filepath}'")
        sys.exit(1)
        
    with open(filepath, "r") as f:
        original = f.read()
        
    formatted = format_slop(original)
    
    if len(sys.argv) > 2 and sys.argv[2] == "--check":
        if original == formatted:
            print(f"Pristine! '{filepath}' is formatted correctly.")
            sys.exit(0)
        else:
            print(f"Mismatch! '{filepath}' needs formatting.")
            sys.exit(1)
            
    # Overwrite the file in place with beautifully standard formatted code
    with open(filepath, "w") as f:
        f.write(formatted)
    print(f"Formatted: '{filepath}' successfully.")

if __name__ == "__main__":
    main()
