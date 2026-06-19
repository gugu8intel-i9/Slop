#!/usr/bin/env python3
import sys
import os

class Token:
    def __init__(self, type_, value, line, col):
        self.type = type_
        self.value = value
        self.line = line
        self.col = col

    def __repr__(self):
        return f"Token({self.type}, {repr(self.value)}, {self.line}:{self.col})"

class Lexer:
    def __init__(self, source):
        self.source = source
        self.length = len(source)
        self.pos = 0
        self.line = 1
        self.col = 1

    def peek(self, offset=0):
        if self.pos + offset >= self.length:
            return ""
        return self.source[self.pos + offset]

    def advance(self):
        if self.pos >= self.length:
            return ""
        char = self.source[self.pos]
        self.pos += 1
        if char == '\n':
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return char

    def tokenize(self):
        tokens = []
        while self.pos < self.length:
            char = self.peek()

            # Skip whitespace
            if char.isspace():
                self.advance()
                continue

            # Skip comments
            if char == '#':
                while self.peek() and self.peek() != '\n':
                    self.advance()
                continue

            # Multi-character symbols
            if char == '-' and self.peek(1) == '>':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("ARROW", "->", l, c))
                continue

            if char == '|' and self.peek(1) == '>':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("PIPE", "|>", l, c))
                continue

            if char == '=' and self.peek(1) == '=':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("EQ", "==", l, c))
                continue

            if char == '!' and self.peek(1) == '=':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("NEQ", "!=", l, c))
                continue

            if char == '<' and self.peek(1) == '=':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("LEQ", "<=", l, c))
                continue

            if char == '>' and self.peek(1) == '=':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("GEQ", ">=", l, c))
                continue

            # Single-character symbols
            if char in "()[]{}:,;+-%*/=<>":
                l, c = self.line, self.col
                self.advance()
                tokens.append(Token("SYMBOL", char, l, c))
                continue

            # String literals
            if char == '"':
                l, c = self.line, self.col
                self.advance()  # Consume open quote
                val = []
                while self.peek() and self.peek() != '"':
                    if self.peek() == '\\':
                        self.advance()
                        escaped = self.advance()
                        if escaped == 'n':
                            val.append('\n')
                        elif escaped == 't':
                            val.append('\t')
                        elif escaped == '"':
                            val.append('"')
                        elif escaped == '\\':
                            val.append('\\')
                        else:
                            val.append('\\' + escaped)
                    else:
                        val.append(self.advance())
                if self.peek() != '"':
                    print(f"Error: Unterminated string literal at {l}:{c}", file=sys.stderr)
                    sys.exit(1)
                self.advance()  # Consume close quote
                tokens.append(Token("STRING", "".join(val), l, c))
                continue

            # Numeric literals
            if char.isdigit():
                l, c = self.line, self.col
                val = []
                while self.peek().isdigit():
                    val.append(self.advance())
                tokens.append(Token("NUMBER", "".join(val), l, c))
                continue

            # Identifiers and keywords
            if char.isalpha() or char == '_':
                l, c = self.line, self.col
                val = []
                while self.peek().isalnum() or self.peek() == '_':
                    val.append(self.advance())
                name = "".join(val)
                if name in ["fn", "let", "if", "else", "while", "return", "struct", "true", "false"]:
                    tokens.append(Token("KEYWORD", name, l, c))
                else:
                    tokens.append(Token("IDENTIFIER", name, l, c))
                continue

            # Unknown character error
            l, c = self.line, self.col
            print(f"Error: Unknown character '{char}' at {l}:{c}", file=sys.stderr)
            sys.exit(1)

        tokens.append(Token("EOF", "", self.line, self.col))
        return tokens

# Parser AST Nodes
class ASTNode:
    pass

class ProgramNode(ASTNode):
    def __init__(self, declarations):
        self.declarations = declarations

class StructNode(ASTNode):
    def __init__(self, name, fields):
        self.name = name
        self.fields = fields # list of (name, type)

class FuncNode(ASTNode):
    def __init__(self, name, params, return_type, body):
        self.name = name
        self.params = params # list of (name, type)
        self.return_type = return_type
        self.body = body # list of statements

class VarDeclNode(ASTNode):
    def __init__(self, name, type_, expr):
        self.name = name
        self.type = type_
        self.expr = expr

class AssignNode(ASTNode):
    def __init__(self, name, expr):
        self.name = name
        self.expr = expr

class ReturnNode(ASTNode):
    def __init__(self, expr):
        self.expr = expr

class IfNode(ASTNode):
    def __init__(self, cond, then_branch, else_branch):
        self.cond = cond
        self.then_branch = then_branch
        self.else_branch = else_branch

class WhileNode(ASTNode):
    def __init__(self, cond, body):
        self.cond = cond
        self.body = body

class ExprNode(ASTNode):
    pass

class LiteralNode(ExprNode):
    def __init__(self, type_, value):
        self.type = type_ # "int", "string", "bool"
        self.value = value

class IdentifierNode(ExprNode):
    def __init__(self, name):
        self.name = name

class BinaryOpNode(ExprNode):
    def __init__(self, op, left, right):
        self.op = op
        self.left = left
        self.right = right

class CallNode(ExprNode):
    def __init__(self, name, args):
        self.name = name
        self.args = args

class ArrayIndexNode(ExprNode):
    def __init__(self, array_expr, index_expr):
        self.array_expr = array_expr
        self.index_expr = index_expr

class ArrayLiteralNode(ExprNode):
    def __init__(self, elements, element_type=None):
        self.elements = elements
        self.element_type = element_type # can be None, meaning empty/inferred

class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self, offset=0):
        if self.pos + offset >= len(self.tokens):
            return self.tokens[-1]
        return self.tokens[self.pos + offset]

    def advance(self):
        tok = self.peek()
        if self.pos < len(self.tokens):
            self.pos += 1
        return tok

    def match(self, type_, value=None):
        tok = self.peek()
        if tok.type == type_ and (value is None or tok.value == value):
            self.advance()
            return tok
        return None

    def consume(self, type_, value=None, msg=None):
        tok = self.match(type_, value)
        if not tok:
            actual = self.peek()
            err_msg = msg or f"Expected {type_} (value: {value}), got {actual.type} (value: {actual.value})"
            print(f"Error at {actual.line}:{actual.col} - {err_msg}", file=sys.stderr)
            sys.exit(1)
        return tok

    def parse_type(self):
        # Parses types like "int", "string", "bool", "array[int]", "array[string]"
        base = self.consume("IDENTIFIER", msg="Expected type identifier").value
        if base == "array" and self.match("SYMBOL", "["):
            elem_type = self.parse_type()
            self.consume("SYMBOL", "]", "Expected closing ']' for array type")
            return f"array[{elem_type}]"
        return base

    def parse(self):
        declarations = []
        while self.peek().type != "EOF":
            if self.match("KEYWORD", "struct"):
                declarations.append(self.parse_struct())
            elif self.match("KEYWORD", "fn"):
                declarations.append(self.parse_function())
            else:
                tok = self.peek()
                print(f"Error at {tok.line}:{tok.col} - Unexpected top-level declaration starting with {tok.type} {tok.value}", file=sys.stderr)
                sys.exit(1)
        return ProgramNode(declarations)

    def parse_struct(self):
        name = self.consume("IDENTIFIER", msg="Expected struct name").value
        self.consume("SYMBOL", "{", "Expected '{' after struct name")
        fields = []
        while not self.match("SYMBOL", "}"):
            field_name = self.consume("IDENTIFIER", msg="Expected field name").value
            self.consume("SYMBOL", ":", "Expected ':' after field name")
            field_type = self.parse_type()
            fields.append((field_name, field_type))
            if self.match("SYMBOL", ","):
                continue
            if self.peek().value == "}":
                continue
        return StructNode(name, fields)

    def parse_function(self):
        name = self.consume("IDENTIFIER", msg="Expected function name").value
        self.consume("SYMBOL", "(", "Expected '(' after function name")
        params = []
        while not self.match("SYMBOL", ")"):
            param_name = self.consume("IDENTIFIER", msg="Expected parameter name").value
            self.consume("SYMBOL", ":", "Expected ':' after parameter name")
            param_type = self.parse_type()
            params.append((param_name, param_type))
            if self.match("SYMBOL", ","):
                continue
            if self.peek().value == ")":
                continue
        
        return_type = "void"
        if self.match("ARROW"):
            return_type = self.parse_type()

        self.consume("SYMBOL", "{", "Expected '{' to start function body")
        body = []
        while not self.match("SYMBOL", "}"):
            body.append(self.parse_statement())
        return FuncNode(name, params, return_type, body)

    def parse_statement(self):
        if self.match("KEYWORD", "let"):
            name = self.consume("IDENTIFIER", msg="Expected variable name").value
            type_ = None
            if self.match("SYMBOL", ":"):
                type_ = self.parse_type()
            self.consume("SYMBOL", "=", "Expected '=' in variable declaration")
            expr = self.parse_expression()
            return VarDeclNode(name, type_, expr)
        
        if self.match("KEYWORD", "return"):
            expr = None
            if self.peek().value != "}" and self.peek().type != "EOF":
                expr = self.parse_expression()
            return ReturnNode(expr)

        if self.match("KEYWORD", "if"):
            cond = self.parse_expression()
            self.consume("SYMBOL", "{", "Expected '{' after if condition")
            then_branch = []
            while not self.match("SYMBOL", "}"):
                then_branch.append(self.parse_statement())
            else_branch = []
            if self.match("KEYWORD", "else"):
                if self.match("KEYWORD", "if"):
                    # else if
                    else_branch = [self.parse_statement()]
                else:
                    self.consume("SYMBOL", "{", "Expected '{' after else")
                    while not self.match("SYMBOL", "}"):
                        else_branch.append(self.parse_statement())
            return IfNode(cond, then_branch, else_branch)

        if self.match("KEYWORD", "while"):
            cond = self.parse_expression()
            self.consume("SYMBOL", "{", "Expected '{' after while condition")
            body = []
            while not self.match("SYMBOL", "}"):
                body.append(self.parse_statement())
            return WhileNode(cond, body)

        # Assignment or standalone expression
        expr = self.parse_expression()
        if self.match("SYMBOL", "="):
            if not isinstance(expr, IdentifierNode):
                print(f"Error: Left-hand side of assignment must be an identifier", file=sys.stderr)
                sys.exit(1)
            rhs = self.parse_expression()
            return AssignNode(expr.name, rhs)
        
        return expr

    def parse_expression(self):
        # We parse binary operations with pipeline precedence
        return self.parse_pipeline()

    def parse_pipeline(self):
        expr = self.parse_equality()
        while self.match("PIPE"):
            # The right-hand side of a pipeline must be a function call, or an identifier which represents a function call
            rhs = self.parse_equality()
            if isinstance(rhs, IdentifierNode):
                # e.g., val |> print -> print(val)
                expr = CallNode(rhs.name, [expr])
            elif isinstance(rhs, CallNode):
                # e.g., val |> push(arr) -> push(expr, arr)
                rhs.args.insert(0, expr)
                expr = rhs
            else:
                print("Error: Right-hand side of pipeline operator must be a function call or identifier", file=sys.stderr)
                sys.exit(1)
        return expr

    def parse_equality(self):
        expr = self.parse_relational()
        while True:
            tok = self.peek()
            if tok.type in ["EQ", "NEQ"]:
                self.advance()
                rhs = self.parse_relational()
                expr = BinaryOpNode(tok.value, expr, rhs)
            else:
                break
        return expr

    def parse_relational(self):
        expr = self.parse_additive()
        while True:
            tok = self.peek()
            if tok.type in ["LEQ", "GEQ"] or (tok.type == "SYMBOL" and tok.value in "<>"):
                self.advance()
                rhs = self.parse_additive()
                expr = BinaryOpNode(tok.value, expr, rhs)
            else:
                break
        return expr

    def parse_additive(self):
        expr = self.parse_multiplicative()
        while True:
            tok = self.peek()
            if tok.type == "SYMBOL" and tok.value in "+-":
                self.advance()
                rhs = self.parse_multiplicative()
                expr = BinaryOpNode(tok.value, expr, rhs)
            else:
                break
        return expr

    def parse_multiplicative(self):
        expr = self.parse_primary()
        while True:
            tok = self.peek()
            if tok.type == "SYMBOL" and tok.value in "*/%":
                self.advance()
                rhs = self.parse_primary()
                expr = BinaryOpNode(tok.value, expr, rhs)
            else:
                break
        return expr

    def parse_primary(self):
        tok = self.peek()
        if tok.type == "NUMBER":
            self.advance()
            return LiteralNode("int", int(tok.value))
        
        if tok.type == "STRING":
            self.advance()
            return LiteralNode("string", tok.value)
        
        if tok.type == "KEYWORD" and tok.value in ["true", "false"]:
            self.advance()
            return LiteralNode("bool", tok.value == "true")
        
        if tok.type == "IDENTIFIER":
            self.advance()
            name = tok.value
            
            # Function/Constructor call?
            if self.match("SYMBOL", "("):
                args = []
                while not self.match("SYMBOL", ")"):
                    args.append(self.parse_expression())
                    if self.match("SYMBOL", ","):
                        continue
                    if self.peek().value == ")":
                        continue
                expr = CallNode(name, args)
            else:
                expr = IdentifierNode(name)
            
            # Array index or field access or method-like pipeline can follow
            while True:
                if self.match("SYMBOL", "["):
                    idx_expr = self.parse_expression()
                    self.consume("SYMBOL", "]", "Expected closing ']' for array indexing")
                    expr = ArrayIndexNode(expr, idx_expr)
                else:
                    break
            return expr

        if self.match("SYMBOL", "["):
            elements = []
            while not self.match("SYMBOL", "]"):
                elements.append(self.parse_expression())
                if self.match("SYMBOL", ","):
                    continue
                if self.peek().value == "]":
                    continue
            return ArrayLiteralNode(elements)

        if self.match("SYMBOL", "("):
            expr = self.parse_expression()
            self.consume("SYMBOL", ")", "Expected closing ')'")
            return expr

        print(f"Error at {tok.line}:{tok.col} - Unexpected expression token '{tok.value}' (type: {tok.type})", file=sys.stderr)
        sys.exit(1)


# Code Generator to convert AST into highly optimized C
class CodeGenerator:
    def __init__(self):
        self.output = []
        self.temp_var_counter = 0
        self.var_types = {} # variable name -> type
        self.func_types = { # func_name -> (return_type, [param_types])
            "print": ("void", ["string"]),
            "print_int": ("void", ["int"]),
            "print_bool": ("void", ["bool"]),
            "length": ("int", ["string"]),
            "push": ("void", ["array[string]", "string"]), # or ints
            "read_file": ("string", ["string"]),
            "write_file": ("void", ["string", "string"]),
            "split": ("array[string]", ["string", "string"]),
            "join": ("string", ["array[string]", "string"]),
            "char_at": ("string", ["string", "int"]),
            "int_to_string": ("string", ["int"]),
            "string_equal": ("bool", ["string", "string"]),
        }
        self.structs = {}

    def get_c_type(self, slop_type):
        if slop_type == "int":
            return "int64_t"
        if slop_type == "float":
            return "double"
        if slop_type == "bool":
            return "bool"
        if slop_type == "string":
            return "SlopString"
        if slop_type == "void":
            return "void"
        if slop_type.startswith("array["):
            return "SlopArray"
        if slop_type in self.structs:
            return f"struct {slop_type}"
        return slop_type

    def generate(self, node):
        self.output.append('#include "slop_rt.h"\n')
        self.output.append('int slop_arena_depth = 0;\n\n')
        
        # Step 1: Forward declarations of structs
        for decl in node.declarations:
            if isinstance(decl, StructNode):
                self.structs[decl.name] = decl
                self.output.append(f"struct {decl.name};\n")
        
        # Step 2: Define structs
        for decl in node.declarations:
            if isinstance(decl, StructNode):
                self.output.append(f"struct {decl.name} {{\n")
                for f_name, f_type in decl.fields:
                    self.output.append(f"    {self.get_c_type(f_type)} {f_name};\n")
                self.output.append("};\n\n")

        # Step 3: Forward declarations of functions
        for decl in node.declarations:
            if isinstance(decl, FuncNode):
                self.func_types[decl.name] = (decl.return_type, [t for n, t in decl.params])
                params_c = []
                for p_name, p_type in decl.params:
                    params_c.append(f"{self.get_c_type(p_type)} {p_name}")
                params_str = ", ".join(params_c)
                self.output.append(f"{self.get_c_type(decl.return_type)} fn_{decl.name}({params_str});\n")
        self.output.append("\n")

        # Step 4: Implement functions
        for decl in node.declarations:
            if isinstance(decl, FuncNode):
                self.generate_function(decl)

        # Step 5: Implement main entry point
        self.output.append("""
int main(int argc, char** argv) {
    slop_arena_depth = 1;
    SlopArena* local_arena = slop_get_arena(slop_arena_depth);
    
    // Convert CLI arguments into SlopArray of SlopStrings
    SlopArray args = slop_array_create(local_arena);
    for (int i = 0; i < argc; i++) {
        SlopString* s = (SlopString*)slop_arena_alloc(local_arena, sizeof(SlopString));
        *s = slop_string_create(local_arena, argv[i]);
        slop_array_push(local_arena, &args, s);
    }
    
    // Call main Slop function
    fn_main(args);
    return 0;
}
""")
        return "".join(self.output)

    def generate_function(self, func_node):
        params_c = []
        # Save old variable types
        old_var_types = self.var_types.copy()
        for p_name, p_type in func_node.params:
            self.var_types[p_name] = p_type
            params_c.append(f"{self.get_c_type(p_type)} {p_name}")
        params_str = ", ".join(params_c)

        ret_type = self.get_c_type(func_node.return_type)
        self.output.append(f"{ret_type} fn_{func_node.name}({params_str}) {{\n")
        
        # Function prolog
        self.output.append("    slop_arena_depth++;\n")
        self.output.append("    SlopArena* local_arena = slop_get_arena(slop_arena_depth);\n")
        self.output.append("    size_t saved_offset = slop_arena_save(local_arena);\n")
        
        # Compile statements
        for stmt in func_node.body:
            self.generate_statement(stmt)

        # Function epilog (if void return is implicit/fallback)
        if func_node.return_type == "void":
            self.output.append("    slop_arena_restore(local_arena, saved_offset);\n")
            self.output.append("    slop_arena_depth--;\n")
        
        self.output.append("}\n\n")
        self.var_types = old_var_types

    def generate_statement(self, stmt):
        if isinstance(stmt, VarDeclNode):
            # Type inference if not specified
            expr_code, expr_type = self.generate_expression(stmt.expr)
            var_type = stmt.type or expr_type
            self.var_types[stmt.name] = var_type
            c_type = self.get_c_type(var_type)
            self.output.append(f"    {c_type} {stmt.name} = {expr_code};\n")
            
        elif isinstance(stmt, AssignNode):
            expr_code, _ = self.generate_expression(stmt.expr)
            self.output.append(f"    {stmt.name} = {expr_code};\n")
            
        elif isinstance(stmt, ReturnNode):
            if stmt.expr:
                expr_code, expr_type = self.generate_expression(stmt.expr)
                if expr_type == "string":
                    # Clone string to caller's arena before wiping local arena
                    self.output.append(f"    SlopString _ret = slop_string_clone(slop_get_arena(slop_arena_depth - 1), {expr_code});\n")
                    self.output.append("    slop_arena_restore(local_arena, saved_offset);\n")
                    self.output.append("    slop_arena_depth--;\n")
                    self.output.append("    return _ret;\n")
                elif expr_type.startswith("array["):
                    if "string" in expr_type:
                        self.output.append(f"    SlopArray _ret = slop_array_clone_strings(slop_get_arena(slop_arena_depth - 1), {expr_code});\n")
                    else:
                        self.output.append(f"    SlopArray _ret = slop_array_clone_ints(slop_get_arena(slop_arena_depth - 1), {expr_code});\n")
                    self.output.append("    slop_arena_restore(local_arena, saved_offset);\n")
                    self.output.append("    slop_arena_depth--;\n")
                    self.output.append("    return _ret;\n")
                else:
                    self.output.append("    slop_arena_restore(local_arena, saved_offset);\n")
                    self.output.append("    slop_arena_depth--;\n")
                    self.output.append(f"    return {expr_code};\n")
            else:
                self.output.append("    slop_arena_restore(local_arena, saved_offset);\n")
                self.output.append("    slop_arena_depth--;\n")
                self.output.append("    return;\n")
                
        elif isinstance(stmt, IfNode):
            cond_code, _ = self.generate_expression(stmt.cond)
            self.output.append(f"    if ({cond_code}) {{\n")
            for sub_stmt in stmt.then_branch:
                self.generate_statement(sub_stmt)
            self.output.append("    }")
            if stmt.else_branch:
                self.output.append(" else {\n")
                for sub_stmt in stmt.else_branch:
                    self.generate_statement(sub_stmt)
                self.output.append("    }\n")
            else:
                self.output.append("\n")
                
        elif isinstance(stmt, WhileNode):
            cond_code, _ = self.generate_expression(stmt.cond)
            self.output.append(f"    while ({cond_code}) {{\n")
            for sub_stmt in stmt.body:
                self.generate_statement(sub_stmt)
            self.output.append("    }\n")
            
        else:
            # Standalone expression
            expr_code, _ = self.generate_expression(stmt)
            self.output.append(f"    {expr_code};\n")

    def generate_expression(self, expr):
        if isinstance(expr, LiteralNode):
            if expr.type == "int":
                return str(expr.value), "int"
            elif expr.type == "string":
                # Convert string to static initializer or create dynamically
                # Since dynamic create is very robust with our escaping model, let's use:
                escaped = expr.value.replace('"', '\\"').replace('\n', '\\n')
                return f'slop_string_create(local_arena, "{escaped}")', "string"
            elif expr.type == "bool":
                return "true" if expr.value else "false", "bool"
                
        elif isinstance(expr, IdentifierNode):
            t = self.var_types.get(expr.name, "int")
            return expr.name, t
            
        elif isinstance(expr, BinaryOpNode):
            left_code, left_type = self.generate_expression(expr.left)
            right_code, right_type = self.generate_expression(expr.right)
            
            # String operations
            if left_type == "string" and right_type == "string":
                if expr.op == "+":
                    return f"slop_string_concat(local_arena, {left_code}, {right_code})", "string"
                elif expr.op == "==":
                    return f"slop_string_equal({left_code}, {right_code})", "bool"
                elif expr.op == "!=":
                    return f"!slop_string_equal({left_code}, {right_code})", "bool"
            
            # Normal arithmetic or comparison
            return f"({left_code} {expr.op} {right_code})", left_type
            
        elif isinstance(expr, ArrayIndexNode):
            arr_code, arr_type = self.generate_expression(expr.array_expr)
            idx_code, _ = self.generate_expression(expr.index_expr)
            
            elem_type = "int"
            if arr_type.startswith("array[") and arr_type.endswith("]"):
                elem_type = arr_type[6:-1]
                
            c_elem_type = self.get_c_type(elem_type)
            # Casting pointer in array to correct type
            return f"(*({c_elem_type}*)({arr_code}.data[{idx_code}]))", elem_type
            
        elif isinstance(expr, ArrayLiteralNode):
            # Check types
            elem_type = "int"
            if expr.elements:
                _, elem_type = self.generate_expression(expr.elements[0])
            
            c_elem_type = self.get_c_type(elem_type)
            arr_var = self.fresh_temp()
            # We declare and assign array elements in C
            self.output.append(f"    SlopArray {arr_var} = slop_array_create(local_arena);\n")
            for elem in expr.elements:
                elem_code, _ = self.generate_expression(elem)
                # Allocate element in the local arena so it persists until the array is clone/destroyed
                elem_ptr = self.fresh_temp()
                self.output.append(f"    {c_elem_type}* {elem_ptr} = ({c_elem_type}*)slop_arena_alloc(local_arena, sizeof({c_elem_type}));\n")
                self.output.append(f"    *{elem_ptr} = {elem_code};\n")
                self.output.append(f"    slop_array_push(local_arena, &{arr_var}, {elem_ptr});\n")
            return arr_var, f"array[{elem_type}]"
            
        elif isinstance(expr, CallNode):
            # Check if builtin
            if expr.name == "print":
                arg_code, arg_type = self.generate_expression(expr.args[0])
                if arg_type == "int":
                    return f"slop_print_int({arg_code})", "void"
                elif arg_type == "bool":
                    return f"slop_print_bool({arg_code})", "void"
                else:
                    return f"slop_print_string({arg_code})", "void"
            elif expr.name == "length":
                arg_code, arg_type = self.generate_expression(expr.args[0])
                if arg_type == "string":
                    return f"({arg_code}.length)", "int"
                else:
                    return f"({arg_code}.length)", "int"
            elif expr.name == "push":
                arr_code, arr_type = self.generate_expression(expr.args[0])
                elem_code, elem_type = self.generate_expression(expr.args[1])
                c_elem_type = self.get_c_type(elem_type)
                elem_ptr = self.fresh_temp()
                self.output.append(f"    {c_elem_type}* {elem_ptr} = ({c_elem_type}*)slop_arena_alloc(local_arena, sizeof({c_elem_type}));\n")
                self.output.append(f"    *{elem_ptr} = {elem_code};\n")
                return f"slop_array_push(local_arena, &{arr_code}, {elem_ptr})", "void"
            elif expr.name == "read_file":
                arg_code, _ = self.generate_expression(expr.args[0])
                return f"slop_read_file(local_arena, {arg_code})", "string"
            elif expr.name == "write_file":
                path_code, _ = self.generate_expression(expr.args[0])
                content_code, _ = self.generate_expression(expr.args[1])
                return f"slop_write_file({path_code}, {content_code})", "void"
            elif expr.name == "int_to_string":
                arg_code, _ = self.generate_expression(expr.args[0])
                return f"slop_int_to_string(local_arena, {arg_code})", "string"
            elif expr.name == "string_equal":
                arg1_code, _ = self.generate_expression(expr.args[0])
                arg2_code, _ = self.generate_expression(expr.args[1])
                return f"slop_string_equal({arg1_code}, {arg2_code})", "bool"
            elif expr.name == "split":
                # Returns SlopArray of SlopStrings
                str_code, _ = self.generate_expression(expr.args[0])
                sep_code, _ = self.generate_expression(expr.args[1])
                arr_var = self.fresh_temp()
                self.output.append(f"    SlopArray {arr_var} = slop_array_create(local_arena);\n")
                # Custom inline C string splitting to SlopStrings
                self.output.append(f"""    {{
        SlopString s = {str_code};
        SlopString sep = {sep_code};
        size_t start = 0;
        if (sep.length == 0) {{
            for (size_t i = 0; i < s.length; i++) {{
                SlopString* sub = (SlopString*)slop_arena_alloc(local_arena, sizeof(SlopString));
                *sub = slop_string_create_len(local_arena, s.data + i, 1);
                slop_array_push(local_arena, &{arr_var}, sub);
            }}
        }} else {{
            for (size_t i = 0; i <= s.length - sep.length; ) {{
                if (memcmp(s.data + i, sep.data, sep.length) == 0) {{
                    SlopString* sub = (SlopString*)slop_arena_alloc(local_arena, sizeof(SlopString));
                    *sub = slop_string_create_len(local_arena, s.data + start, i - start);
                    slop_array_push(local_arena, &{arr_var}, sub);
                    i += sep.length;
                    start = i;
                }} else {{
                    i++;
                }}
            }}
            if (start <= s.length) {{
                SlopString* sub = (SlopString*)slop_arena_alloc(local_arena, sizeof(SlopString));
                *sub = slop_string_create_len(local_arena, s.data + start, s.length - start);
                slop_array_push(local_arena, &{arr_var}, sub);
            }}
        }}
    }}
""")
                return arr_var, "array[string]"
            elif expr.name == "join":
                arr_code, _ = self.generate_expression(expr.args[0])
                sep_code, _ = self.generate_expression(expr.args[1])
                res_var = self.fresh_temp()
                self.output.append(f"    SlopString {res_var} = slop_string_create(local_arena, \"\");\n")
                self.output.append(f"""    {{
                    SlopArray arr = {arr_code};
                    SlopString sep = {sep_code};
                    for (size_t i = 0; i < arr.length; i++) {{
                        SlopString* s = (SlopString*)arr.data[i];
                        if (i > 0) {{
                            {res_var} = slop_string_concat(local_arena, {res_var}, sep);
                        }}
                        {res_var} = slop_string_concat(local_arena, {res_var}, *s);
                    }}
                }}
""")
                return res_var, "string"
            elif expr.name == "char_at":
                str_code, _ = self.generate_expression(expr.args[0])
                idx_code, _ = self.generate_expression(expr.args[1])
                res_var = self.fresh_temp()
                self.output.append(f"    SlopString {res_var} = slop_string_create(local_arena, \"\");\n")
                self.output.append(f"""    {{
                    SlopString s = {str_code};
                    int64_t idx = {idx_code};
                    if (idx >= 0 && (size_t)idx < s.length) {{
                        {res_var} = slop_string_create_len(local_arena, s.data + idx, 1);
                    }}
                }}
""")
                return res_var, "string"

            # Custom user-defined functions or struct constructors
            if expr.name in self.structs:
                # Constructor call
                struct_def = self.structs[expr.name]
                struct_var = self.fresh_temp()
                self.output.append(f"    struct {expr.name} {struct_var};\n")
                for i, arg in enumerate(expr.args):
                    field_name = struct_def.fields[i][0]
                    arg_code, _ = self.generate_expression(arg)
                    self.output.append(f"    {struct_var}.{field_name} = {arg_code};\n")
                return struct_var, expr.name
                
            ret_type, _ = self.func_types.get(expr.name, ("int", []))
            args_code = []
            for arg in expr.args:
                code, _ = self.generate_expression(arg)
                args_code.append(code)
            args_str = ", ".join(args_code)
            return f"fn_{expr.name}({args_str})", ret_type
            
        print(f"Error: Unknown expression node: {type(expr)}", file=sys.stderr)
        sys.exit(1)

    def fresh_temp(self):
        self.temp_var_counter += 1
        return f"_tmp_{self.temp_var_counter}"


def main():
    if len(sys.argv) < 2:
        print("Usage: slop_boot.py <input.slop> [output.c]", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file.replace(".slop", ".c")

    if not os.path.exists(input_file):
        print(f"Error: File not found '{input_file}'", file=sys.stderr)
        sys.exit(1)

    with open(input_file, "r") as f:
        source = f.read()

    lexer = Lexer(source)
    tokens = lexer.tokenize()

    parser = Parser(tokens)
    ast = parser.parse()

    codegen = CodeGenerator()
    c_code = codegen.generate(ast)

    with open(output_file, "w") as f:
        f.write(c_code)

    print(f"Successfully compiled {input_file} to {output_file}")


if __name__ == "__main__":
    main()
