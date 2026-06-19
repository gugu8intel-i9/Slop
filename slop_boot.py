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

            if char == '=' and self.peek(1) == '>':
                l, c = self.line, self.col
                self.advance()
                self.advance()
                tokens.append(Token("FAT_ARROW", "=>", l, c))
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
            if char in "()[]{}:,;+-%*/=.":
                l, c = self.line, self.col
                self.advance()
                tokens.append(Token("SYMBOL", char, l, c))
                continue

            if char in "<>":
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
                if name in ["fn", "let", "if", "else", "while", "return", "struct", "true", "false", "match", "for", "in", "raw"]:
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
    def __init__(self, name, fields, methods):
        self.name = name
        self.fields = fields 
        self.methods = methods 

class FuncNode(ASTNode):
    def __init__(self, name, params, return_type, body, struct_context=None):
        self.name = name
        self.params = params 
        self.return_type = return_type
        self.body = body 
        self.struct_context = struct_context 

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

class MatchNode(ASTNode):
    def __init__(self, expr, cases):
        self.expr = expr
        self.cases = cases 

class RawBlockNode(ASTNode):
    def __init__(self, code):
        self.code = code

class ExprNode(ASTNode):
    pass

class LiteralNode(ExprNode):
    def __init__(self, type_, value):
        self.type = type_ 
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

class MemberCallNode(ExprNode):
    def __init__(self, object_expr, method_name, args):
        self.object_expr = object_expr
        self.method_name = method_name
        self.args = args

class FieldAccessNode(ExprNode):
    def __init__(self, object_expr, field_name):
        self.object_expr = object_expr
        self.field_name = field_name

class ArrayIndexNode(ExprNode):
    def __init__(self, array_expr, index_expr):
        self.array_expr = array_expr
        self.index_expr = index_expr

class ArrayLiteralNode(ExprNode):
    def __init__(self, elements, element_type=None):
        self.elements = elements
        self.element_type = element_type 

class ListComprehensionNode(ExprNode):
    def __init__(self, element_expr, var_name, source_expr):
        self.element_expr = element_expr
        self.var_name = var_name
        self.source_expr = source_expr

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
        methods = []
        while not self.match("SYMBOL", "}"):
            if self.match("KEYWORD", "fn"):
                methods.append(self.parse_function(struct_context=name))
            else:
                field_name = self.consume("IDENTIFIER", msg="Expected field name").value
                self.consume("SYMBOL", ":", "Expected ':' after field name")
                field_type = self.parse_type()
                fields.append((field_name, field_type))
                if self.match("SYMBOL", ","):
                    continue
                if self.peek().value == "}":
                    continue
        return StructNode(name, fields, methods)

    def parse_function(self, struct_context=None):
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
        return FuncNode(name, params, return_type, body, struct_context)

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

        if self.match("KEYWORD", "raw"):
            self.consume("SYMBOL", "{", "Expected '{' after raw")
            code_str = self.consume("STRING", msg="Expected C code string inside raw block").value
            self.consume("SYMBOL", "}", "Expected '}' after raw code string")
            return RawBlockNode(code_str)

        if self.match("KEYWORD", "if"):
            cond = self.parse_expression()
            self.consume("SYMBOL", "{", "Expected '{' after if condition")
            then_branch = []
            while not self.match("SYMBOL", "}"):
                then_branch.append(self.parse_statement())
            else_branch = []
            if self.match("KEYWORD", "else"):
                if self.match("KEYWORD", "if"):
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

        if self.match("KEYWORD", "match"):
            expr = self.parse_expression()
            self.consume("SYMBOL", "{", "Expected '{' after match expression")
            cases = []
            while not self.match("SYMBOL", "}"):
                pattern = None
                if self.match("KEYWORD", "else") or self.match("IDENTIFIER", "_"):
                    pattern = None
                else:
                    pattern = self.parse_expression()
                self.consume("FAT_ARROW", msg="Expected '=>' in match arm")
                
                body = []
                if self.match("SYMBOL", "{"):
                    while not self.match("SYMBOL", "}"):
                        body.append(self.parse_statement())
                else:
                    body.append(self.parse_statement())
                cases.append((pattern, body))
                if self.match("SYMBOL", ","):
                    continue
            return MatchNode(expr, cases)

        # Assignment or standalone expression
        expr = self.parse_expression()
        if self.match("SYMBOL", "="):
            rhs = self.parse_expression()
            return AssignNode(expr, rhs)
        
        return expr

    def parse_expression(self):
        return self.parse_pipeline()

    def parse_pipeline(self):
        expr = self.parse_equality()
        while self.match("PIPE"):
            rhs = self.parse_equality()
            if isinstance(rhs, IdentifierNode):
                expr = CallNode(rhs.name, [expr])
            elif isinstance(rhs, CallNode):
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
            
            # Postfix operations
            while True:
                if self.match("SYMBOL", "."):
                    member_name = self.consume("IDENTIFIER", msg="Expected member identifier").value
                    if self.match("SYMBOL", "("):
                        args = []
                        while not self.match("SYMBOL", ")"):
                            args.append(self.parse_expression())
                            if self.match("SYMBOL", ","):
                                continue
                            if self.peek().value == ")":
                                continue
                        expr = MemberCallNode(expr, member_name, args)
                    else:
                        expr = FieldAccessNode(expr, member_name)
                elif self.match("SYMBOL", "["):
                    idx_expr = self.parse_expression()
                    self.consume("SYMBOL", "]", "Expected closing ']' for array indexing")
                    expr = ArrayIndexNode(expr, idx_expr)
                else:
                    break
            return expr

        if self.match("SYMBOL", "["):
            elements = []
            if self.peek().value != "]":
                first = self.parse_expression()
                if self.match("KEYWORD", "for"):
                    var_name = self.consume("IDENTIFIER", msg="Expected variable name in comprehension").value
                    self.consume("KEYWORD", "in", "Expected 'in' in comprehension")
                    source_expr = self.parse_expression()
                    self.consume("SYMBOL", "]", "Expected closing ']'")
                    return ListComprehensionNode(first, var_name, source_expr)
                
                elements.append(first)
                while self.match("SYMBOL", ","):
                    elements.append(self.parse_expression())
            self.consume("SYMBOL", "]", "Expected closing ']'")
            return ArrayLiteralNode(elements)

        if self.match("SYMBOL", "("):
            expr = self.parse_expression()
            self.consume("SYMBOL", ")", "Expected closing ')'")
            return expr

        print(f"Error at {tok.line}:{tok.col} - Unexpected expression token '{tok.value}' (type: {tok.type})", file=sys.stderr)
        sys.exit(1)


# Code Generator
class CodeGenerator:
    def __init__(self):
        self.output = []
        self.temp_var_counter = 0
        self.var_types = {} 
        self.func_types = { 
            "print": ("void", ["string"]),
            "print_int": ("void", ["int"]),
            "print_bool": ("void", ["bool"]),
            "length": ("int", ["string"]),
            "push": ("void", ["array[string]", "string"]), 
            "read_file": ("string", ["string"]),
            "write_file": ("void", ["string", "string"]),
            "split": ("array[string]", ["string", "string"]),
            "join": ("string", ["array[string]", "string"]),
            "char_at": ("string", ["string", "int"]),
            "int_to_string": ("string", ["int"]),
            "string_equal": ("bool", ["string", "string"]),
            "peek_byte": ("int", ["int"]),
            "poke_byte": ("void", ["int", "int"]),
            "peek_int": ("int", ["int"]),
            "poke_int": ("void", ["int", "int"]),
            "get_address": ("int", ["int"]),
            "slop_pack": ("string", ["array[string]"]),
            "slop_unpack": ("array[string]", ["string"]),
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

    def wrap_parens(self, code):
        if code.startswith("(") and code.endswith(")"):
            return code
        return f"({code})"

    def generate(self, node):
        self.output.append('#include "slop_rt.h"\n')
        self.output.append('int slop_arena_depth = 0;\n\n')
        
        # Step 1: Forward declarations of structs
        for decl in node.declarations:
            if isinstance(decl, StructNode):
                self.structs[decl.name] = decl
                self.output.append(f"struct {decl.name};\n")
        
        # Step 2: Define struct layouts
        for decl in node.declarations:
            if isinstance(decl, StructNode):
                self.output.append(f"struct {decl.name} {{\n")
                for f_name, f_type in decl.fields:
                    self.output.append(f"    {self.get_c_type(f_type)} {f_name};\n")
                self.output.append("};\n\n")

        # Step 3: Forward declarations of functions and struct methods
        for decl in node.declarations:
            if isinstance(decl, FuncNode):
                self.func_types[decl.name] = (decl.return_type, [t for n, t in decl.params])
                params_c = []
                for p_name, p_type in decl.params:
                    params_c.append(f"{self.get_c_type(p_type)} {p_name}")
                self.output.append(f"{self.get_c_type(decl.return_type)} fn_{decl.name}({', '.join(params_c)});\n")
            elif isinstance(decl, StructNode):
                for m in decl.methods:
                    self.func_types[f"{decl.name}_{m.name}"] = (m.return_type, [decl.name] + [t for n, t in m.params])
                    params_c = [f"struct {decl.name} this"]
                    for p_name, p_type in m.params:
                        params_c.append(f"{self.get_c_type(p_type)} {p_name}")
                    self.output.append(f"{self.get_c_type(m.return_type)} fn_{decl.name}_{m.name}({', '.join(params_c)});\n")
        self.output.append("\n")

        # Step 4: Implement functions and methods
        for decl in node.declarations:
            if isinstance(decl, FuncNode):
                self.generate_function(decl)
            elif isinstance(decl, StructNode):
                for m in decl.methods:
                    self.generate_function(m)

        # Step 5: Implement main entry point
        self.output.append("""
int main(int argc, char** argv) {
    slop_arena_depth = 1;
    SlopArena* local_arena = slop_get_arena(slop_arena_depth);
    
    SlopArray args = slop_array_create(local_arena);
    for (int i = 0; i < argc; i++) {
        SlopString* s = (SlopString*)slop_arena_alloc(local_arena, sizeof(SlopString));
        *s = slop_string_create(local_arena, argv[i]);
        slop_array_push(local_arena, &args, s);
    }
    
    fn_main(args);
    return 0;
}
""")
        return "".join(self.output)

    def generate_function(self, func_node):
        params_c = []
        old_var_types = self.var_types.copy()
        
        if func_node.struct_context:
            self.var_types["this"] = func_node.struct_context
            params_c.append(f"struct {func_node.struct_context} this")
            
        for p_name, p_type in func_node.params:
            self.var_types[p_name] = p_type
            params_c.append(f"{self.get_c_type(p_type)} {p_name}")

        ret_type = self.get_c_type(func_node.return_type)
        func_name = f"{func_node.struct_context}_{func_node.name}" if func_node.struct_context else func_node.name
        self.output.append(f"{ret_type} fn_{func_name}({', '.join(params_c)}) {{\n")
        
        self.output.append("    slop_arena_depth++;\n")
        self.output.append("    SlopArena* local_arena = slop_get_arena(slop_arena_depth);\n")
        self.output.append("    size_t saved_offset = slop_arena_save(local_arena);\n")
        
        for stmt in func_node.body:
            self.generate_statement(stmt)

        if func_node.return_type == "void":
            self.output.append("    slop_arena_restore(local_arena, saved_offset);\n")
            self.output.append("    slop_arena_depth--;\n")
        
        self.output.append("}\n\n")
        self.var_types = old_var_types

    def generate_statement(self, stmt):
        if isinstance(stmt, VarDeclNode):
            expr_code, expr_type = self.generate_expression(stmt.expr)
            var_type = stmt.type or expr_type
            self.var_types[stmt.name] = var_type
            self.output.append(f"    {self.get_c_type(var_type)} {stmt.name} = {expr_code};\n")
            
        elif isinstance(stmt, AssignNode):
            target_code, _ = self.generate_expression(stmt.name)
            expr_code, _ = self.generate_expression(stmt.expr)
            self.output.append(f"    {target_code} = {expr_code};\n")
            
        elif isinstance(stmt, ReturnNode):
            if stmt.expr:
                expr_code, expr_type = self.generate_expression(stmt.expr)
                if expr_type == "string":
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

        elif isinstance(stmt, RawBlockNode):
            self.output.append(f"    {stmt.code}\n")
                
        elif isinstance(stmt, IfNode):
            cond_code, _ = self.generate_expression(stmt.cond)
            self.output.append(f"    if {self.wrap_parens(cond_code)} {{\n")
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
            self.output.append(f"    while {self.wrap_parens(cond_code)} {{\n")
            for sub_stmt in stmt.body:
                self.generate_statement(sub_stmt)
            self.output.append("    }\n")

        elif isinstance(stmt, MatchNode):
            expr_code, expr_type = self.generate_expression(stmt.expr)
            first = True
            for pattern, body_stmts in stmt.cases:
                if pattern is None: 
                    self.output.append(" else {\n")
                else:
                    pat_code, _ = self.generate_expression(pattern)
                    cond_str = f"({expr_code} == {pat_code})"
                    if expr_type == "string":
                        cond_str = f"slop_string_equal({expr_code}, {pat_code})"
                    
                    if first:
                        self.output.append(f"    if {self.wrap_parens(cond_str)} {{\n")
                        first = False
                    else:
                        self.output.append(f" else if {self.wrap_parens(cond_str)} {{\n")
                
                for body_stmt in body_stmts:
                    self.generate_statement(body_stmt)
                self.output.append("    }")
            self.output.append("\n")
            
        else:
            expr_code, _ = self.generate_expression(stmt)
            self.output.append(f"    {expr_code};\n")

    def generate_expression(self, expr):
        if isinstance(expr, LiteralNode):
            if expr.type == "int":
                return str(expr.value), "int"
            elif expr.type == "string":
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
            
            if left_type == "string" and right_type == "string":
                if expr.op == "+":
                    return f"slop_string_concat(local_arena, {left_code}, {right_code})", "string"
                elif expr.op == "==":
                    return f"slop_string_equal({left_code}, {right_code})", "bool"
                elif expr.op == "!=":
                    return f"!slop_string_equal({left_code}, {right_code})", "bool"
            
            return f"({left_code} {expr.op} {right_code})", left_type

        elif isinstance(expr, FieldAccessNode):
            obj_code, obj_type = self.generate_expression(expr.object_expr)
            struct_def = self.structs.get(obj_type)
            field_type = "int"
            if struct_def:
                for f_name, f_type in struct_def.fields:
                    if f_name == expr.field_name:
                        field_type = f_type
            return f"({obj_code}.{expr.field_name})", field_type

        elif isinstance(expr, MemberCallNode):
            obj_code, obj_type = self.generate_expression(expr.object_expr)
            func_name = f"{obj_type}_{expr.method_name}"
            ret_type, _ = self.func_types.get(func_name, ("int", []))
            
            args_code = [obj_code]
            for arg in expr.args:
                code, _ = self.generate_expression(arg)
                args_code.append(code)
            
            return f"fn_{func_name}({', '.join(args_code)})", ret_type
            
        elif isinstance(expr, ArrayIndexNode):
            arr_code, arr_type = self.generate_expression(expr.array_expr)
            idx_code, _ = self.generate_expression(expr.index_expr)
            
            elem_type = "int"
            if arr_type.startswith("array[") and arr_type.endswith("]"):
                elem_type = arr_type[6:-1]
                
            c_elem_type = self.get_c_type(elem_type)
            return f"(*({c_elem_type}*)(slop_array_get({arr_code}, {idx_code})))", elem_type
            
        elif isinstance(expr, ArrayLiteralNode):
            elem_type = "int"
            if expr.elements:
                _, elem_type = self.generate_expression(expr.elements[0])
            
            c_elem_type = self.get_c_type(elem_type)
            arr_var = self.fresh_temp()
            self.output.append(f"    SlopArray {arr_var} = slop_array_create(local_arena);\n")
            for elem in expr.elements:
                elem_code, _ = self.generate_expression(elem)
                elem_ptr = self.fresh_temp()
                self.output.append(f"    {c_elem_type}* {elem_ptr} = ({c_elem_type}*)slop_arena_alloc(local_arena, sizeof({c_elem_type}));\n")
                self.output.append(f"    *{elem_ptr} = {elem_code};\n")
                self.output.append(f"    slop_array_push(local_arena, &{arr_var}, {elem_ptr});\n")
            return arr_var, f"array[{elem_type}]"

        elif isinstance(expr, ListComprehensionNode):
            source_code, source_type = self.generate_expression(expr.source_expr)
            elem_type = "int"
            if source_type.startswith("array[") and source_type.endswith("]"):
                elem_type = source_type[6:-1]
            
            old_var_type = self.var_types.get(expr.var_name)
            self.var_types[expr.var_name] = elem_type
            
            arr_var = self.fresh_temp()
            self.output.append(f"    SlopArray {arr_var} = slop_array_create(local_arena);\n")
            idx_var = self.fresh_temp()
            
            self.output.append(f"    for (size_t {idx_var} = 0; {idx_var} < {source_code}.length; {idx_var}++) {{\n")
            self.output.append(f"        {self.get_c_type(elem_type)} {expr.var_name} = *({self.get_c_type(elem_type)}*)({source_code}.data[{idx_var}]);\n")
            
            elt_code, elt_type = self.generate_expression(expr.element_expr)
            c_elt_type = self.get_c_type(elt_type)
            
            elt_ptr = self.fresh_temp()
            self.output.append(f"        {c_elt_type}* {elt_ptr} = ({c_elt_type}*)slop_arena_alloc(local_arena, sizeof({c_elt_type}));\n")
            self.output.append(f"        *{elt_ptr} = {elt_code};\n")
            self.output.append(f"        slop_array_push(local_arena, &{arr_var}, {elt_ptr});\n")
            self.output.append("    }\n")
            
            if old_var_type:
                self.var_types[expr.var_name] = old_var_type
            else:
                del self.var_types[expr.var_name]
                
            return arr_var, f"array[{elt_type}]"
            
        elif isinstance(expr, CallNode):
            if expr.name == "slop_pack":
                arg_code, _ = self.generate_expression(expr.args[0])
                return f"slop_pack_strings(local_arena, {arg_code})", "string"
            elif expr.name == "slop_unpack":
                arg_code, _ = self.generate_expression(expr.args[0])
                return f"slop_unpack_strings(local_arena, {arg_code})", "array[string]"
                
            elif expr.name == "peek_byte":
                addr_code, _ = self.generate_expression(expr.args[0])
                return f"(*(volatile uint8_t*)({addr_code}))", "int"
            elif expr.name == "poke_byte":
                addr_code, _ = self.generate_expression(expr.args[0])
                val_code, _ = self.generate_expression(expr.args[1])
                return f"(*(volatile uint8_t*)({addr_code}) = (uint8_t)({val_code}))", "void"
            elif expr.name == "peek_int":
                addr_code, _ = self.generate_expression(expr.args[0])
                return f"(*(volatile uint64_t*)({addr_code}))", "int"
            elif expr.name == "poke_int":
                addr_code, _ = self.generate_expression(expr.args[0])
                val_code, _ = self.generate_expression(expr.args[1])
                return f"(*(volatile uint64_t*)({addr_code}) = (uint64_t)({val_code}))", "void"
            elif expr.name == "get_address":
                arg_code, _ = self.generate_expression(expr.args[0])
                return f"((int64_t)&({arg_code}))", "int"
                
            elif expr.name == "print":
                arg_code, arg_type = self.generate_expression(expr.args[0])
                if arg_type == "int":
                    return f"slop_print_int({arg_code})", "void"
                elif arg_type == "bool":
                    return f"slop_print_bool({arg_code})", "void"
                else:
                    return f"slop_print_string({arg_code})", "void"
            elif expr.name == "length":
                arg_code, arg_type = self.generate_expression(expr.args[0])
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
                str_code, _ = self.generate_expression(expr.args[0])
                sep_code, _ = self.generate_expression(expr.args[1])
                arr_var = self.fresh_temp()
                self.output.append(f"    SlopArray {arr_var} = slop_array_create(local_arena);\n")
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
            for (size_t i = 0; s.length >= sep.length && i <= s.length - sep.length; ) {{
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
            return f"fn_{expr.name}({', '.join(args_code)})", ret_type
            
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
