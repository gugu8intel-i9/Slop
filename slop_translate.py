#!/usr/bin/env python3
import sys
import os
import ast

class SlopTranslator(ast.NodeVisitor):
    def __init__(self):
        self.output = []
        self.indent_level = 0

    def indent(self):
        return "    " * self.indent_level

    def visit_Module(self, node):
        for stmt in node.body:
            self.visit(stmt)

    def visit_FunctionDef(self, node):
        # Translate arguments and their annotations
        params = []
        for arg in node.args.args:
            arg_name = arg.arg
            arg_type = "int"  # default if no annotation
            if arg.annotation:
                arg_type = self.get_type_name(arg.annotation)
            params.append(f"{arg_name}: {arg_type}")
        
        params_str = ", ".join(params)
        
        # Determine return type
        ret_type = "void"
        if node.returns:
            ret_type = self.get_type_name(node.returns)
            
        self.output.append(f"{self.indent()}fn {node.name}({params_str}) -> {ret_type} {{\n")
        self.indent_level += 1
        for stmt in node.body:
            self.visit(stmt)
        self.indent_level -= 1
        self.output.append(f"{self.indent()}}}\n\n")

    def visit_Assign(self, node):
        # Handle let statements or reassignments
        # For simplicity, if it's the first time we see the variable, we can prepend let
        # To do this safely, we can check if we are inside a function.
        # But since Slop requires 'let' for variable declaration, let's always transpile
        # to 'let name = value' if it's not a known name, or we can use a set of local names.
        target = node.targets[0]
        if isinstance(target, ast.Name):
            var_name = target.id
            val_code = self.visit_expr(node.value)
            # In simple terms: we will use 'let' if it's an assignment. 
            # In our simple translator, we can look at the name.
            # Let's assume all assignments in Python are declarations if we haven't seen them,
            # but to keep it simple and robust, let's prepend 'let' for all assignments
            # unless the target is a subscript or attribute, or if we know it's a reassignment.
            # Let's just output 'let var_name = val_code' for names.
            # To handle reassignments, we can track declared variables.
            # Let's do that!
            if not hasattr(self, 'declared_vars'):
                self.declared_vars = set()
            
            if var_name in self.declared_vars:
                self.output.append(f"{self.indent()}{var_name} = {val_code}\n")
            else:
                self.declared_vars.add(var_name)
                self.output.append(f"{self.indent()}let {var_name} = {val_code}\n")
        elif isinstance(target, ast.Subscript):
            # Array index assignment: arr[idx] = val
            arr_name = self.visit_expr(target.value)
            idx_code = self.visit_expr(target.slice)
            val_code = self.visit_expr(node.value)
            # Since Slop doesn't have direct array index reassignment in our simplified compiler,
            # we can output it or handle it.
            self.output.append(f"{self.indent()}{arr_name}[{idx_code}] = {val_code}\n")

    def visit_Return(self, node):
        if node.value:
            val_code = self.visit_expr(node.value)
            self.output.append(f"{self.indent()}return {val_code}\n")
        else:
            self.output.append(f"{self.indent()}return\n")

    def visit_If(self, node):
        cond_code = self.visit_expr(node.test)
        self.output.append(f"{self.indent()}if {cond_code} {{\n")
        self.indent_level += 1
        for stmt in node.body:
            self.visit(stmt)
        self.indent_level -= 1
        self.output.append(f"{self.indent()}}}")
        if node.orelse:
            self.output.append(" else {\n")
            self.indent_level += 1
            for stmt in node.orelse:
                self.visit(stmt)
            self.indent_level -= 1
            self.output.append(f"{self.indent()}}}\n")
        else:
            self.output.append("\n")

    def visit_While(self, node):
        cond_code = self.visit_expr(node.test)
        self.output.append(f"{self.indent()}while {cond_code} {{\n")
        self.indent_level += 1
        for stmt in node.body:
            self.visit(stmt)
        self.indent_level -= 1
        self.output.append(f"{self.indent()}}}\n")

    def visit_Expr(self, node):
        expr_code = self.visit_expr(node.value)
        self.output.append(f"{self.indent()}{expr_code}\n")

    def visit_expr(self, node):
        if isinstance(node, ast.Constant):
            if isinstance(node.value, str):
                return f'"{node.value}"'
            elif isinstance(node.value, bool):
                return "true" if node.value else "false"
            return str(node.value)
        elif isinstance(node, ast.Name):
            return node.id
        elif isinstance(node, ast.BinOp):
            left = self.visit_expr(node.left)
            right = self.visit_expr(node.right)
            op = self.get_op_symbol(node.op)
            return f"({left} {op} {right})"
        elif isinstance(node, ast.Compare):
            left = self.visit_expr(node.left)
            op = self.get_op_symbol(node.ops[0])
            right = self.visit_expr(node.comparators[0])
            return f"({left} {op} {right})"
        elif isinstance(node, ast.Call):
            func_name = self.visit_expr(node.func)
            args = [self.visit_expr(arg) for arg in node.args]
            args_str = ", ".join(args)
            return f"{func_name}({args_str})"
        elif isinstance(node, ast.Subscript):
            val = self.visit_expr(node.value)
            idx = self.visit_expr(node.slice)
            return f"{val}[{idx}]"
        elif isinstance(node, ast.List):
            elts = [self.visit_expr(elt) for elt in node.elts]
            elts_str = ", ".join(elts)
            return f"[{elts_str}]"
        return ""

    def get_type_name(self, node):
        if isinstance(node, ast.Name):
            name = node.id
            if name == "str":
                return "string"
            if name == "list" or name == "List":
                return "array[string]"
            return name
        elif isinstance(node, ast.Subscript):
            # e.g. List[str] -> array[string] or list -> array
            container = self.visit_expr(node.value)
            if container in ["List", "list", "array"]:
                elem = self.get_type_name(node.slice)
                return f"array[{elem}]"
        return "int"

    def get_op_symbol(self, op):
        if isinstance(op, ast.Add): return "+"
        if isinstance(op, ast.Sub): return "-"
        if isinstance(op, ast.Mult): return "*"
        if isinstance(op, ast.Div): return "/"
        if isinstance(op, ast.Mod): return "%"
        if isinstance(op, ast.Eq): return "=="
        if isinstance(op, ast.NotEq): return "!="
        if isinstance(op, ast.Lt): return "<"
        if isinstance(op, ast.LtE): return "<="
        if isinstance(op, ast.Gt): return ">"
        if isinstance(op, ast.GtE): return ">="
        return "+"

def translate_python_to_slop(py_code):
    parsed = ast.parse(py_code)
    translator = SlopTranslator()
    translator.visit(parsed)
    return "".join(translator.output)

def main():
    if len(sys.argv) < 2:
        print("Usage: slop_translate.py <input.py> [output.slop]", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file.replace(".py", ".slop")

    if not os.path.exists(input_file):
        print(f"Error: File not found '{input_file}'", file=sys.stderr)
        sys.exit(1)

    with open(input_file, "r") as f:
        py_code = f.read()

    slop_code = translate_python_to_slop(py_code)

    with open(output_file, "w") as f:
        f.write(slop_code)

    print(f"Successfully translated Python file {input_file} to native Slop file {output_file}")

if __name__ == "__main__":
    main()
