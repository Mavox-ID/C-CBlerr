import sys
import os
import subprocess
import tempfile
import re
import platform
from pathlib import Path
from typing import List, Optional, Tuple, Dict

DERR_FLAG = False
SAVE_C_FLAG = False

def _extract_cli_flags():
    global DERR_FLAG, SAVE_C_FLAG
    argv = sys.argv
    new_argv = [argv[0]] if argv else []
    i = 1
    while i < len(argv):
        a = argv[i]
        if a == '-derr':
            DERR_FLAG = True
            i += 1
            continue
        if a == '-c':
            SAVE_C_FLAG = True
            i += 1
            continue
        new_argv.append(a)
        i += 1
    if len(new_argv) != len(argv):
        sys.argv[:] = new_argv

_extract_cli_flags()

core_path = Path(__file__).parent.parent / "core"
if getattr(sys, 'frozen', False):
    try:
        meipass = Path(sys._MEIPASS)
        core_path = meipass / "core"
        sys.path.insert(0, str(meipass.resolve()))
    except Exception:
        core_path = Path(__file__).parent.parent / "core"
        sys.path.insert(0, str(core_path.parent.resolve()))
else:
    sys.path.insert(0, str(Path(__file__).parent.parent.resolve()))

import importlib
try:
    for _name in ("lexer", "flux_parser", "flux_ast"):
        try:
            core_mod = importlib.import_module(f"core.{_name}")
            sys.modules[_name] = core_mod
        except Exception:
            pass
except Exception:
    pass

from core.lexer import tokenize, TokenType
from core.flux_parser import parse
from core.flux_ast import (
    Program, FunctionDef, Return, BinaryOp, Variable, Literal,
    IfStmt, Assign, Compare, Call, WhileLoop, BreakStmt, ContinueStmt,
    StructDef, FieldAccess, ArrayAccess, ArrayLiteral, LogicalOp,
    PointerType, Dereference, InlineAsm, CastExpr, Decorator, ComptimeBlock
)
from core.flux_ast import MatchStmt, Case, ForLoop, EnumDef, AddressOf, SizeOf
from core.debugger import init_debugger, get_debugger, DebugLevel


class CCodeGenerator:
    def __init__(self, module_name: str = "cblerr_module", link_mode: Optional[str] = None, is_gui_app: bool = False):
        self.module_name = module_name
        self.link_mode = link_mode
        self.is_gui_app = is_gui_app
        self.code_lines = []
        self.indent_level = 0
        self.struct_definitions = {}
        self.function_declarations = {}
        self.global_vars = {}
        self.string_constants = {}
        self.current_function = None
        self.break_stack = []
        self.continue_stack = []
        self.string_counter = 0
        self.local_vars_stack = []

    def generate(self, program: Program) -> str:
        if self.link_mode == 'static':
            self.emit_line('#define CBLERR_LINK_STATIC 1')
        elif self.link_mode == 'dynamic':
            self.emit_line('#define CBLERR_LINK_DYNAMIC 1')
            
        self.emit_line("typedef signed char int8_t;")
        self.emit_line("typedef short int16_t;")
        self.emit_line("typedef int int32_t;")
        self.emit_line("typedef long long int64_t;")
        self.emit_line("typedef unsigned char uint8_t;")
        self.emit_line("typedef unsigned short uint16_t;")
        self.emit_line("typedef unsigned int uint32_t;")
        self.emit_line("typedef unsigned long long uint64_t;")
        self.emit_line("#if defined(__GNUC__) || defined(__clang__)")
        self.emit_line("typedef __SIZE_TYPE__ size_t;")
        self.emit_line("#else")
        self.emit_line("#if defined(_WIN64)")
        self.emit_line("typedef unsigned long long size_t;")
        self.emit_line("#else")
        self.emit_line("typedef unsigned int size_t;")
        self.emit_line("#endif")
        self.emit_line("#endif")
        self.emit_line("#define bool _Bool")
        self.emit_line("#define true 1")
        self.emit_line("#define false 0")
        self.emit_line("#define NULL ((void*)0)")
        self.emit_line("")
        self.emit_line("typedef struct { const char* data; int64_t length; } flux_string;")
        self.emit_line("")
        
        self.emit_line("#if defined(_WIN32) || defined(__WIN32__)")
        self.emit_line("extern void* __stdcall LoadLibraryA(const void*);")
        self.emit_line("extern void* __stdcall GetProcAddress(void*, const void*);")
        self.emit_line("extern void* __stdcall GetModuleHandleA(const void*);")
        self.emit_line("extern void __stdcall ExitProcess(uint32_t);")
        self.emit_line("int _fltused = 0;")       
        self.emit_line("void __main(void) {}")    
        self.emit_line("#endif")
        self.emit_line("")
        
        self.emit_line("extern void* malloc(size_t);")
        self.emit_line("extern void* calloc(size_t, size_t);")
        self.emit_line("extern void* realloc(void*, size_t);")
        self.emit_line("extern void free(void*);")
        self.emit_line("extern void* memset(void*, int, size_t);")
        self.emit_line("extern void* memcpy(void*, const void*, size_t);")
        self.emit_line("extern void* memmove(void*, const void*, size_t);")
        self.emit_line("extern int strcmp(const char*, const char*);")
        self.emit_line("extern int printf(const char*, ...);")
        self.emit_line("extern int sprintf(char*, const char*, ...);")
        self.emit_line("extern int puts(const char*);")
        self.emit_line("extern int putchar(int);")
        self.emit_line("extern int scanf(const char*, ...);")
        self.emit_line("extern void exit(int);")
        self.emit_line("extern void* fopen(const char*, const char*);")
        self.emit_line("extern int fclose(void*);")
        self.emit_line("extern int fgetc(void*);")
        self.emit_line("extern int fputc(int, void*);")
        self.emit_line("extern int feof(void*);")
        self.emit_line("extern int system(const char*);")
        self.emit_line("extern double pow(double, double);")
        self.emit_line("")
        
        self.emit_line("#if defined(_WIN32) || defined(__WIN32__)")
        self.emit_line("typedef int32_t (__stdcall *PFN_SWCA)(void*, void*);")
        self.emit_line("static inline int32_t Cblerr_SetWindowCompositionAttribute(void* hwnd, void* data) {")
        self.emit_line("    void* hUser = LoadLibraryA((const void*)\"user32.dll\");")
        self.emit_line("    if (hUser) {")
        self.emit_line("        PFN_SWCA pfn = (PFN_SWCA)GetProcAddress(hUser, (const void*)\"SetWindowCompositionAttribute\");")
        self.emit_line("        if (pfn) return pfn(hwnd, data);")
        self.emit_line("    }")
        self.emit_line("    return 0;")
        self.emit_line("}")
        self.emit_line("#define SetWindowCompositionAttribute Cblerr_SetWindowCompositionAttribute")
        self.emit_line("#endif")
        self.emit_line("")

        if program.structs:
            for struct_def in program.structs:
                if isinstance(struct_def, EnumDef):
                    continue
                self.emit_line(f"struct {struct_def.name};")
            self.emit_line("")
        if program.structs:
            for struct_def in program.structs:
                if isinstance(struct_def, EnumDef):
                    self.generate_enum(struct_def)
                else:
                    self.generate_struct_def(struct_def)
            self.emit_line("")
        if program.global_vars:
            for global_var in program.global_vars:
                self.generate_global_var(global_var)
            self.emit_line("")
            
        if program.functions:
            skip_std = {'malloc', 'calloc', 'realloc', 'free', 'memset', 'memcpy', 'memmove', 'printf', 'sprintf',
                        'puts', 'putchar', 'scanf', 'exit', 'fopen', 'fgetc', 'feof', 'fclose', 'fputc', 'system',
                        'SetWindowCompositionAttribute', 'LoadLibraryA', 'GetProcAddress', 'GetModuleHandleA', 'ExitProcess'}
            for func_def in program.functions:
                if hasattr(func_def, 'is_extern') and func_def.is_extern:
                    if func_def.name in skip_std:
                        continue
                    sig = self.generate_function_signature(func_def)
                    self.emit_line(f"{sig};")
            for func_def in program.functions:
                if hasattr(func_def, 'is_extern') and func_def.is_extern:
                    continue
                sig = self.generate_function_signature(func_def)
                self.emit_line(f"{sig};")
            has_forwardes = any(
                not (hasattr(f, 'is_extern') and f.is_extern) for f in program.functions
            )
            if has_forwardes:
                self.emit_line("")
        if program.functions:
            for func_def in program.functions:
                if hasattr(func_def, 'is_extern') and func_def.is_extern:
                    continue
                self.generate_function_def(func_def)
                self.emit_line("")
                
        self.emit_line("")
        self.emit_line("#if defined(_WIN32) || defined(__WIN32__)")
        if self.is_gui_app:
            self.emit_line("void CblerrStartup(void) { WinMain(GetModuleHandleA(NULL), NULL, (void*)\"\", 5); ExitProcess(0); }")
        else:
            self.emit_line("void CblerrStartup(void) { main(); ExitProcess(0); }")
        self.emit_line("#endif")
        
        return "\n".join(self.code_lines)

    def emit_line(self, line: str = ""):
        if line:
            self.code_lines.append("    " * self.indent_level + line)
        else:
            self.code_lines.append("")

    def emit_block(self, lines: List[str]):
        for line in lines:
            self.emit_line(line)

    def _escape_string(self, s: str) -> str:
        s = s.replace('\\', '\\\\')
        s = s.replace('"', '\\"')
        s = s.replace('\n', '\\n')
        s = s.replace('\r', '\\r')
        s = s.replace('\t', '\\t')
        return s

    def generate_struct_def(self, struct_def):
        self.emit_line(f"struct {struct_def.name} {{")
        self.indent_level += 1
        if isinstance(struct_def.fields, dict):
            fields_items = struct_def.fields.items()
        else:
            fields_items = struct_def.fields
        for item in fields_items:
            if isinstance(item, tuple):
                field_name, field_type = item
            else:
                continue
            if isinstance(field_type, str) and field_type.startswith('*fn('):
                decl = self.get_c_declaration(field_type, field_name)
                self.emit_line(f"{decl};")
            else:
                c_type = self.get_c_type(field_type)
                self.emit_line(f"{c_type} {field_name};")
        self.indent_level -= 1
        self.emit_line("};")

    def generate_global_var(self, global_var):
        if isinstance(global_var.var_type, str) and global_var.var_type.startswith('*fn('):
            decl = self.get_c_declaration(global_var.var_type, global_var.name)
            self.global_vars[global_var.name] = global_var
            if hasattr(global_var, 'value') and global_var.value:
                value_code = self.generate_expression(global_var.value)
                self.emit_line(f"{decl} = {value_code};")
            else:
                self.emit_line(f"{decl};")
            return
        c_type = self.get_c_type(global_var.var_type)
        if hasattr(global_var, 'var_type') and hasattr(global_var.var_type, 'name') and getattr(global_var.var_type, 'name', None) == 'array':
            inner = global_var.var_type.args[0] if getattr(global_var.var_type, 'args', None) else 'int'
            inner_c = self.get_c_type(inner)
            c_type = f"{inner_c}[]"
        self.global_vars[global_var.name] = global_var
        if hasattr(global_var, 'value') and global_var.value:
            value_code = self.generate_expression(global_var.value)
            if isinstance(c_type, str) and c_type.endswith('[]'):
                inner = c_type[:-2]
                self.emit_line(f"{inner} {global_var.name}[] = {value_code};")
            else:
                self.emit_line(f"{c_type} {global_var.name} = {value_code};")
        else:
            if isinstance(c_type, str) and c_type.endswith('[]'):
                inner = c_type[:-2]
                self.emit_line(f"{inner} {global_var.name}[];")
            else:
                self.emit_line(f"{c_type} {global_var.name};")

    def generate_function_signature(self, func_def) -> str:
        return_type = self.get_c_type(func_def.return_type)
        params = []
        if func_def.params:
            for param_tuple in func_def.params:
                if isinstance(param_tuple, tuple) and len(param_tuple) >= 2:
                    param_name, param_type = param_tuple[0], param_tuple[1]
                    if isinstance(param_type, str) and param_type.startswith('*fn('):
                        params.append(self.get_c_declaration(param_type, param_name))
                    else:
                        param_c_type = self.get_c_type(param_type)
                        params.append(f"{param_c_type} {param_name}")
        params_str = ", ".join(params) if params else "void"
        
        if platform.system() == 'Windows':
            cdecl_funcs = {'malloc', 'calloc', 'realloc', 'free', 'memset', 'memcpy', 'memmove', 
                           'printf', 'sprintf', 'puts', 'putchar', 'scanf', 'exit', 'fopen', 
                           'fgetc', 'feof', 'fclose', 'fputc', 'system', 'wsprintfA'}
            
            if hasattr(func_def, 'is_extern') and func_def.is_extern:
                if func_def.name not in cdecl_funcs:
                    return f"{return_type} __stdcall {func_def.name}({params_str})"
                    
            if func_def.name in ('WinMain', 'WindowProc'):
                return f"{return_type} __stdcall {func_def.name}({params_str})"
                
        return f"{return_type} {func_def.name}({params_str})"

    def generate_function_def(self, func_def):
        sig = self.generate_function_signature(func_def)
        if hasattr(func_def, 'is_extern') and func_def.is_extern:
            self.emit_line(f"{sig};")
            return
        
        self.emit_line(f"{sig} {{")
        self.indent_level += 1
        self.current_function = func_def.name
        self.local_vars_stack.append({})

        if func_def.params:
            for param_tuple in func_def.params:
                if isinstance(param_tuple, tuple) and len(param_tuple) >= 2:
                    pname, ptype = param_tuple[0], param_tuple[1]
                    try:
                        if isinstance(ptype, str) and ptype.startswith('*fn('):
                            p_c = self.get_c_declaration(ptype, pname)
                        else:
                            p_c = self.get_c_type(ptype)
                    except Exception:
                        p_c = None
                    if p_c:
                        self.local_vars_stack[-1][pname] = p_c

        has_explicit_return = bool(func_def.body and isinstance(func_def.body[-1], Return))
        if func_def.body:
            for stmt in func_def.body:
                self.generate_statement(stmt)

        if not has_explicit_return and func_def.return_type != 'void' and func_def.return_type is not None:
            if func_def.return_type in ['int', 'i32', 'int32']:
                self.emit_line("return 0;")
            else:
                self.emit_line(f"return ({self.get_c_type(func_def.return_type)})0;")
        self.indent_level -= 1
        self.emit_line("}")
        if self.local_vars_stack:
            self.local_vars_stack.pop()

    def generate_statement(self, stmt):
        if isinstance(stmt, Return):
            self.generate_return(stmt)
        elif isinstance(stmt, Assign):
            self.generate_assign(stmt)
        elif isinstance(stmt, IfStmt):
            self.generate_if(stmt)
        elif isinstance(stmt, WhileLoop):
            self.generate_while(stmt)
        elif isinstance(stmt, ForLoop):
            self.generate_for(stmt)
        elif isinstance(stmt, MatchStmt):
            self.generate_match(stmt)
        elif isinstance(stmt, EnumDef):
            self.generate_enum(stmt)
        elif isinstance(stmt, InlineAsm):
            self.emit_line(f'__asm__("{self._escape_string(stmt.code)}");')
        elif isinstance(stmt, BreakStmt):
            self.emit_line("break;")
        elif isinstance(stmt, ContinueStmt):
            self.emit_line("continue;")
        elif isinstance(stmt, Call):
            expr = self.generate_expression(stmt)
            self.emit_line(f"{expr};")
        else:
            expr_code = self.generate_expression(stmt)
            if expr_code and expr_code != "0":
                self.emit_line(f"{expr_code};")

    def generate_return(self, ret_stmt):
        if ret_stmt.value:
            value_code = self.generate_expression(ret_stmt.value)
            self.emit_line(f"return {value_code};")
        else:
            self.emit_line("return;")

    def generate_assign(self, assign_stmt):
        if isinstance(assign_stmt.target, str):
            target = assign_stmt.target
            value = self.generate_expression(assign_stmt.value)
            if hasattr(assign_stmt, 'var_type') and assign_stmt.var_type:
                if isinstance(assign_stmt.var_type, str) and assign_stmt.var_type.startswith('*fn('):
                    decl = self.get_c_declaration(assign_stmt.var_type, target)
                    self.emit_line(f"{decl} = {value};")
                    if self.local_vars_stack:
                        self.local_vars_stack[-1][target] = decl
                else:
                    c_type = self.get_c_type(assign_stmt.var_type)
                    self.emit_line(f"{c_type} {target} = {value};")
                    if self.local_vars_stack:
                        self.local_vars_stack[-1][target] = c_type
            else:
                self.emit_line(f"{target} = {value};")
        else:
            target = self.generate_expression(assign_stmt.target)
            value = self.generate_expression(assign_stmt.value)
            self.emit_line(f"{target} = {value};")

    def generate_if(self, if_stmt):
        condition = self.generate_expression(if_stmt.condition)
        self.emit_line(f"if ({condition}) {{")
        self.indent_level += 1
        for stmt in if_stmt.then_body:
            self.generate_statement(stmt)
        self.indent_level -= 1
        if if_stmt.else_body:
            self.emit_line("} else {")
            self.indent_level += 1
            for stmt in if_stmt.else_body:
                self.generate_statement(stmt)
            self.indent_level -= 1
            self.emit_line("}")
        else:
            self.emit_line("}")

    def generate_while(self, while_stmt):
        condition = self.generate_expression(while_stmt.condition)
        self.emit_line(f"while ({condition}) {{")
        self.indent_level += 1
        for stmt in while_stmt.body:
            self.generate_statement(stmt)
        self.indent_level -= 1
        self.emit_line("}")

    def generate_for(self, for_stmt: ForLoop):
        if for_stmt.init or for_stmt.condition or for_stmt.post:
            init_code = ""
            if for_stmt.init:
                if isinstance(for_stmt.init, Assign):
                    if hasattr(for_stmt.init, 'var_type') and for_stmt.init.var_type:
                        ctype = self.get_c_type(for_stmt.init.var_type)
                        val = self.generate_expression(for_stmt.init.value)
                        init_code = f"{ctype} {for_stmt.init.target} = {val}"
                    else:
                        init_code = f"{for_stmt.init.target} = {self.generate_expression(for_stmt.init.value)}"
                else:
                    init_code = self.generate_expression(for_stmt.init)
            cond_code = self.generate_expression(for_stmt.condition) if for_stmt.condition else "1"
            post_code = self.generate_expression(for_stmt.post) if for_stmt.post else ""
            self.emit_line(f"for ({init_code}; {cond_code}; {post_code}) {{")
            self.indent_level += 1
            for stmt in for_stmt.body:
                self.generate_statement(stmt)
            self.indent_level -= 1
            self.emit_line("}")
            return

        if for_stmt.iter_var and for_stmt.iter_expr:
            ie = for_stmt.iter_expr
            if isinstance(ie, Call) and isinstance(ie.func_name, str) and ie.func_name == 'range' and len(ie.args) >= 2:
                start = self.generate_expression(ie.args[0])
                end = self.generate_expression(ie.args[1])
                self.emit_line(f"for (int {for_stmt.iter_var} = {start}; {for_stmt.iter_var} < {end}; ++{for_stmt.iter_var}) {{")
                self.indent_level += 1
                for stmt in for_stmt.body:
                    self.generate_statement(stmt)
                self.indent_level -= 1
                self.emit_line("}")
                return

            iter_resolved = getattr(ie, 'resolved_type', None)
            if isinstance(ie, Variable) and iter_resolved and isinstance(iter_resolved, str) and iter_resolved.startswith('array<'):
                inner = iter_resolved[len('array<'):-1]
                elem_c = self.get_c_type(inner)
                arr_name = self.generate_expression(ie)
                idx_name = f"__idx_{for_stmt.iter_var}"
                len_expr = f"(sizeof({arr_name})/sizeof({arr_name}[0]))"
                self.emit_line(f"for (int {idx_name} = 0; {idx_name} < {len_expr}; ++{idx_name}) {{")
                self.indent_level += 1
                self.emit_line(f"{elem_c} {for_stmt.iter_var} = {arr_name}[{idx_name}];")
                for stmt in for_stmt.body:
                    self.generate_statement(stmt)
                self.indent_level -= 1
                self.emit_line("}")
                return

            self.emit_line("{")
            self.indent_level += 1
            self.emit_line("TODO: implement generic iteration")
            for stmt in for_stmt.body:
                self.generate_statement(stmt)
            self.indent_level -= 1
            self.emit_line("}")

    def generate_match(self, match_stmt: MatchStmt):
        expr_code = self.generate_expression(match_stmt.expr)
        expr_type = getattr(match_stmt.expr, 'resolved_type', None)
        first = True
        for case in match_stmt.cases:
            if case.values is None:
                if first:
                    self.emit_line("if (1) {")
                else:
                    self.emit_line("else {")
                self.indent_level += 1
                for stmt in case.body:
                    self.generate_statement(stmt)
                self.indent_level -= 1
                self.emit_line("}")
                first = False
                continue

            conds = []
            for v in case.values:
                vcode = self.generate_expression(v)
                if expr_type == 'str' or getattr(v, 'resolved_type', None) == 'str':
                    conds.append(f"(strcmp({expr_code}.data, {vcode}.data) == 0)")
                else:
                    conds.append(f"({expr_code} == {vcode})")
            cond_code = " || ".join(conds)
            if first:
                self.emit_line(f"if ({cond_code}) {{")
            else:
                self.emit_line(f"else if ({cond_code}) {{")
            self.indent_level += 1
            for stmt in case.body:
                self.generate_statement(stmt)
            self.indent_level -= 1
            self.emit_line("}")
            first = False

    def generate_enum(self, enum_def: EnumDef):
        self.emit_line(f"typedef enum {{")
        self.indent_level += 1
        members_lines = []
        for name, val in enum_def.members:
            if val is not None:
                vcode = self.generate_expression(val)
                members_lines.append(f"{name} = {vcode},")
            else:
                members_lines.append(f"{name},")
        for line in members_lines:
            self.emit_line(line)
        self.indent_level -= 1
        self.emit_line(f"}} {enum_def.name};")
    
    def generate_expression(self, expr) -> str:
        if expr is None:
            return "0"
        
        if isinstance(expr, Literal):
            if isinstance(expr.value, str):
                escaped = self._escape_string(expr.value)
                length = len(expr.value)
                return f'(flux_string){{"{escaped}", {length}}}'
            elif isinstance(expr.value, float):
                return str(expr.value)
            elif isinstance(expr.value, bool):
                return "true" if expr.value else "false"
            else:
                return str(expr.value)
        
        elif isinstance(expr, Variable):
            return expr.name
        
        elif isinstance(expr, BinaryOp):
            left = self.generate_expression(expr.left)
            right = self.generate_expression(expr.right)

            if expr.op == '**':
                return f"pow({left}, {right})"

            op_map = {
                '+': '+', '-': '-', '*': '*', '/': '/',
                '%': '%', '&': '&', '|': '|', '^': '^',
                '<<': '<<', '>>': '>>'
            }
            op = op_map.get(expr.op, expr.op)
            return f"({left} {op} {right})"
        
        elif isinstance(expr, Compare):
            left = self.generate_expression(expr.left)
            right = self.generate_expression(expr.right)
            left_t = getattr(expr.left, 'resolved_type', None)
            right_t = getattr(expr.right, 'resolved_type', None)
            if left_t == 'str' or right_t == 'str':
                cmp_map = {
                    '==': '== 0',
                    '!=': '!= 0',
                    '<': '< 0',
                    '>': '> 0',
                    '<=': '<= 0',
                    '>=': '>= 0'
                }
                suffix = cmp_map.get(expr.op, f"== 0")
                return f"(strcmp({left}.data, {right}.data) {suffix})"
            else:
                op_map = {
                    '<': '<', '>': '>', '<=': '<=', '>=': '>=',
                    '==': '==', '!=': '!='
                }
                op = op_map.get(expr.op, expr.op)
                return f"({left} {op} {right})"
        
        elif isinstance(expr, LogicalOp):
            if expr.op == 'not':
                left = self.generate_expression(expr.left)
                return f"(!{left})"
                
            left = self.generate_expression(expr.left)
            right = self.generate_expression(expr.right)
            op = '&&' if expr.op == 'and' else '||'
            return f"({left} {op} {right})"
        
        elif isinstance(expr, Call):
            fname = expr.func_name if isinstance(expr.func_name, str) else (expr.func_name.name if isinstance(expr.func_name, Variable) else None)
            if fname == 'print':
                return self._generate_print_call(expr)
            if fname == 'len':
                if not expr.args or len(expr.args) != 1:
                    return '0'
                a = expr.args[0]
                a_code = self.generate_expression(a)
                a_t = getattr(a, 'resolved_type', None) or getattr(a, 'type', None)
                if a_t == 'str':
                    return f"({a_code}.length)"
                if isinstance(a, Variable) or (isinstance(a_t, str) and a_t.startswith('array')):
                    return f"(sizeof({a_code})/sizeof({a_code}[0]))"
                return f"(sizeof({a_code})/sizeof({a_code}[0]))"

            args = [self.generate_expression(arg) for arg in (expr.args or [])]
            args_str = ", ".join(args)
            if isinstance(expr.func_name, str):
                func_code = expr.func_name
            else:
                func_code = self.generate_expression(expr.func_name)
            return f"{func_code}({args_str})"
        
        elif isinstance(expr, ArrayAccess):
            array = self.generate_expression(expr.arr)
            index = self.generate_expression(expr.index)
            return f"{array}[{index}]"
        
        elif isinstance(expr, FieldAccess):
            obj = self.generate_expression(expr.obj)
            obj_type = getattr(expr.obj, 'resolved_type', None)
            use_arrow = False
            if isinstance(obj_type, str) and (obj_type.startswith('*') or obj_type.startswith('ptr<')):
                use_arrow = True
            else:
                if self.local_vars_stack and hasattr(expr.obj, 'name'):
                    name = getattr(expr.obj, 'name')
                    try:
                        declared = self.local_vars_stack[-1].get(name)
                        if declared and ('*' in declared):
                            use_arrow = True
                    except Exception:
                        pass
            if use_arrow:
                return f"{obj}->{expr.field}"
            return f"{obj}.{expr.field}"
        
        elif isinstance(expr, Dereference):
            ptr = self.generate_expression(expr.ptr)
            return f"(*{ptr})"
        
        elif isinstance(expr, CastExpr):
            c_type = self.get_c_type(expr.target_type)
            value = self.generate_expression(expr.expr)   
            return f"(({c_type}){value})"
        elif isinstance(expr, ArrayLiteral):
            elems = [self.generate_expression(e) for e in expr.elements]
            if getattr(expr, 'is_struct_init', False):
                return "{" + ", ".join(elems) + "}"

            elem_c = None
            if getattr(expr, 'array_type', None):
                elem_c = self.get_c_type(expr.array_type)
            elif getattr(expr, 'resolved_type', None) and expr.resolved_type.startswith('array<'):
                inner = expr.resolved_type[len('array<'):-1]
                elem_c = self.get_c_type(inner)
            else:
                if expr.elements:
                    first = expr.elements[0]
                    ft = getattr(first, 'resolved_type', None) or getattr(first, 'type', None)
                    elem_c = self.get_c_type(ft) if ft else 'int32_t'
                else:
                    elem_c = 'int32_t'

            return f'({elem_c}[]){{' + ", ".join(elems) + '}'
        elif isinstance(expr, AddressOf):
            inner = self.generate_expression(expr.expr)
            return f"&{inner}"
        elif isinstance(expr, SizeOf):
            tgt = expr.target
            if isinstance(tgt, str):
                ctype = self.get_c_type(tgt)
                return f"sizeof({ctype})"
            elif hasattr(tgt, 'name'):
                return f"sizeof(struct {tgt.name})"
            else:
                inner = self.generate_expression(tgt)
                return f"sizeof({inner})"
        
        else:
            return "0"
    
    def _generate_print_call(self, expr) -> str:
        if not getattr(expr, 'args', None):
            return 'printf("\\n")'

        fmt_parts = []
        args_list = []

        for arg in expr.args:
            if isinstance(arg, Literal) and isinstance(arg.value, str):
                s = self._escape_string(arg.value)
                fmt_parts.append(s)
                continue

            t = getattr(arg, 'resolved_type', None)
            spec = "%d"
            if t in ('float', 'f32', 'f64', 'double'):
                spec = "%f"
            elif t == 'str':
                spec = "%s"
            elif isinstance(arg, Literal):
                v = arg.value
                if isinstance(v, float):
                    spec = "%f"
                elif isinstance(v, str):
                    spec = "%s"
                else:
                    spec = "%d"

            fmt_parts.append(spec)
            code = self.generate_expression(arg)
            if t == 'str':
                code = f"{code}.data"
            args_list.append(code)

        fmt = "".join(fmt_parts)
        if fmt and not fmt.endswith("\\n"):
            fmt += "\\n"
        elif not fmt:
            fmt = "\\n"

        fmt_escaped = self._escape_string(fmt)
        if args_list:
            return f'printf("{fmt_escaped}", {", ".join(args_list)})'
        return f'printf("{fmt_escaped}")'

    
    def get_c_type(self, flux_type) -> str:
        if flux_type is None or flux_type == 'void':
            return "void"
        
        type_map = {
            'int': 'int32_t',
            'i32': 'int32_t',
            'int32': 'int32_t',
            'int64': 'int64_t',
            'i64': 'int64_t',
            'i16': 'int16_t',
            'i8': 'int8_t',
            'u8': 'uint8_t',
            'u16': 'uint16_t',
            'u32': 'uint32_t',
            'u64': 'uint64_t',
            'float': 'float',
            'f32': 'float',
            'f64': 'double',
            'bool': 'bool',
            'str': 'flux_string',
        }
        if hasattr(flux_type, 'name') and hasattr(flux_type, 'args'):
            if flux_type.name == 'array':
                inner = flux_type.args[0] if flux_type.args else 'int'
                inner_c = self.get_c_type(inner)
                return f"{inner_c}*"
            if flux_type.args:
                return self.get_c_type(flux_type.args[0])
            return "int32_t"
        
        if flux_type in type_map:
            return type_map[flux_type]
        if isinstance(flux_type, str) and flux_type.startswith('*'):
            inner = flux_type[1:]
            inner_c_type = self.get_c_type(inner)
            return f"{inner_c_type}*"
        
        if isinstance(flux_type, str) and flux_type.startswith('ptr<'):
            inner = flux_type[4:-1]
            inner_c_type = self.get_c_type(inner)
            return f"{inner_c_type}*"
        
        if isinstance(flux_type, str) and flux_type not in type_map:
            return f"struct {flux_type}"
        
        return "int32_t"

    def get_c_declaration(self, flux_type, name: str) -> str:
        if not isinstance(flux_type, str):
            return f"{self.get_c_type(flux_type)} {name}"

        def _parse_fn_sig(sig: str) -> tuple[list[str], str]:
            if not sig.startswith('fn('):
                raise ValueError(f"Invalid function signature: {sig}")
            start = sig.find('(')
            pos = start + 1
            depth = 1
            while pos < len(sig) and depth > 0:
                ch = sig[pos]
                if ch == '(':
                    depth += 1
                elif ch == ')':
                    depth -= 1
                pos += 1
            if depth != 0:
                raise ValueError(f"Unbalanced parentheses in signature: {sig}")
            params_section = sig[start+1:pos-1].strip()
            rest = sig[pos:].strip()
            ret = 'void'
            if rest.startswith('->'):
                ret = rest[2:]

            params = []
            if params_section:
                cur = ''
                pdepth = 0
                gdepth = 0
                for ch in params_section:
                    if ch == '<':
                        gdepth += 1; cur += ch
                    elif ch == '>':
                        gdepth -= 1; cur += ch
                    elif ch == '(':
                        pdepth += 1; cur += ch
                    elif ch == ')':
                        pdepth -= 1; cur += ch
                    elif ch == ',' and pdepth == 0 and gdepth == 0:
                        params.append(cur.strip()); cur = ''
                    else:
                        cur += ch
                if cur.strip():
                    params.append(cur.strip())
            return params, ret

        is_ptr = False
        inner = flux_type
        if flux_type.startswith('*'):
            is_ptr = True
            inner = flux_type[1:]

        params, ret = _parse_fn_sig(inner)
        param_cs = [self.get_c_type(p) for p in params] if params else ['void']
        ret_c = self.get_c_type(ret)
        if is_ptr:
            return f"{ret_c} (*{name})({', '.join(param_cs)})"
        return f"{ret_c} {name}({', '.join(param_cs)})"


class StandaloneCompiler:
    def __init__(self, source_file: str, output_exe: str, verbose: bool = True,
                 link_mode: Optional[str] = None, stack_reserve: Optional[int] = None,
                 compiler_type: Optional[str] = None):
        self.source_file = Path(source_file)
        self.output_exe = Path(output_exe)
        self.verbose = verbose
        
        self.system = platform.system()
        self.is_windows = self.system == 'Windows'
        
        self.temp_dir = Path(tempfile.gettempdir()) / "cblerr_standalone"
        self.temp_dir.mkdir(exist_ok=True)
        
        self.c_file = self.temp_dir / f"{self.source_file.stem}.c"
        self.obj_file = self.temp_dir / f"{self.source_file.stem}.obj" if self.is_windows else self.temp_dir / f"{self.source_file.stem}.o"
        self.link_mode = link_mode
        self.stack_reserve = stack_reserve
        self.compiler_type = self._select_compiler(compiler_type)
    
    def log(self, message: str, level: str = "INFO"):
        if self.verbose:
            message = message.replace("✓", "[OK]").replace("✗", "[FAIL]")
            
            MAGENTA = "\033[35m"  
            CYAN = "\033[36m"    
            RED = "\033[31m"     
            RESET = "\033[0m"    
            
            message = re.sub(r'\[(\d+)/(\d+)\]', f'{CYAN}[\\1/\\2]{RESET}', message)
            message = re.sub(r'\[INFO\]', f'{CYAN}[INFO]{RESET}', message)
            
            if level == "INFO":
                level_colored = f"{MAGENTA}[{level}]{RESET}"
            elif level == "WARN":
                level_colored = f"{RED}[{level}]{RESET}"
            else:
                level_colored = f"[{level}]"
            
            print(f"{level_colored} {message}")

    def _select_compiler(self, forced_compiler: Optional[str]) -> str:
        if forced_compiler:
            compiler_name = forced_compiler.lower()
            if compiler_name == 'mingw':
                compiler_name = 'gcc'
            if compiler_name == 'lld':
                compiler_name = 'clang'
            
            if self._compiler_exists(compiler_name):
                return compiler_name
            else:
                self.log(f"[ОШИБКА] Компилятор {forced_compiler} не найден", "ERROR")
                self.log(f"[КАК ИСПРАВИТЬ] Установите {forced_compiler} или выберите другой (--gcc, --clang, --lld, --mingw)", "ERROR")
                raise RuntimeError(f"Компилятор {forced_compiler} не найден")
        
        try:
            if self.is_windows:
                if self._compiler_exists('gcc'):
                    return 'gcc'
                if self._compiler_exists('clang'):
                    return 'clang'
            else:
                if self._compiler_exists('gcc'):
                    return 'gcc'
                if self._compiler_exists('clang'):
                    return 'clang'
        except Exception:
            pass
        
        return 'gcc'
    
    def _compiler_exists(self, compiler_name: str) -> bool:
        try:
            if self.is_windows:
                cmd = f'{compiler_name}.exe'
            else:
                cmd = compiler_name
            result = subprocess.run([cmd, '--version'], capture_output=True, text=True, timeout=5)
            return result.returncode == 0
        except Exception:
            return False

    def _get_compiler_flags(self) -> str:
        env_cflags = os.getenv('CBLERR_CFLAGS')
        if env_cflags:
            return env_cflags
        
        flags = '-std=c11 -Os -s -ffunction-sections -fdata-sections -fno-ident'
        
        if self.is_windows:
            flags += ' -fno-asynchronous-unwind-tables -fno-unwind-tables -fomit-frame-pointer -mno-stack-arg-probe -fno-math-errno'
            
        return flags
    
    def _get_linker_flags(self) -> str:
        flags = []
        
        if self.is_windows:
            flags.append('-nostartfiles')
            flags.append('-Wl,--entry=CblerrStartup')
            
        if self.compiler_type in ('gcc', 'clang', 'lld'):
            if self.link_mode == 'static' and not self.is_windows:
                flags.append('-static')
                
            flags.append('-Wl,--gc-sections')
            
            if self.is_windows:
                flags.append('-Wl,--build-id=none')
                flags.append('-Wl,--no-seh')
                flags.append('-Wl,--file-alignment=512')
                flags.append('-Wl,--section-alignment=4096')
                
        if getattr(self, 'is_gui_app', False) and self.is_windows:
            flags.append('-mwindows')
        
        if self.stack_reserve:
            flags.append(f'-Wl,--stack,{self.stack_reserve}')
        
        return ' '.join(flags)

    def _handle_compile_error(self, error_output: str, debugger) -> bool:
        try:
            if "undefined reference" in error_output or "ld returned 1" in error_output:
                print(f"\n\033[31m[ОШИБКА ЛИНКЕРА (ld)]\033[0m\n{error_output.strip()}", file=sys.stderr)
                return True

            if 'implicit declaration of function' in error_output.lower():
                m = re.search(r"implicit declaration of function '([^']+)'", error_output, flags=re.I)
                if m:
                    func_name = m.group(1)
                    source = None
                    if self.source_file.exists():
                        with open(self.source_file, 'r', encoding='utf-8') as f:
                            source = f.read()
                    
                    lines = source.splitlines() if source else []
                    for line_num, line in enumerate(lines, 1):
                        if func_name in line:
                            exc = SyntaxError(f"Неизвестная функция '{func_name}'")
                            exc.lineno = line_num
                            exc.offset = line.find(func_name) + 1
                            debugger.display_syntax_error(exc, source=source, filename=str(self.source_file))
                            return True
                return True
            
            source = None
            if self.source_file.exists():
                with open(self.source_file, 'r', encoding='utf-8') as f:
                    source = f.read()
            
            m = re.search(r"error:\s*'([^']+)'", error_output)
            if m and source:
                identifier = m.group(1)
                lines = source.splitlines()
                for line_num, line in enumerate(lines, 1):
                    if identifier in line:
                        exc = SyntaxError(f"Неизвестный идентификатор '{identifier}'")
                        exc.lineno = line_num
                        exc.offset = line.find(identifier) + 1
                        debugger.display_syntax_error(exc, source=source, filename=str(self.source_file))
                        return True
            
            if m and source is None:
                identifier = m.group(1)
                debugger.display_syntax_error(
                    SyntaxError(f"Неизвестный идентификатор '{identifier}'"),
                    source=source,
                    filename=str(self.source_file)
                )
                return True
            
            m = re.search(r"error:\s*(.+?)(?:\n|$)", error_output)
            if m:
                msg = m.group(1).strip()
                if source:
                    debugger.display_syntax_error(
                        SyntaxError(msg),
                        source=source,
                        filename=str(self.source_file)
                    )
                    return True
                else:
                    print(f"[ОШИБКА] {msg}", file=sys.stderr)
                    return True
        except Exception:
            pass
        return False
    
    def compile(self) -> bool:
        debugger = init_debugger(DebugLevel.INFO)
        self.debugger = debugger
        try:
            debugger.log_info("Начало компиляции...")
            self.log(f"Консольный Компилятор CBlerr (CCC)")
            self.log(f"ОС: {self.system}")
            self.log(f"Выводимый файл: {self.output_exe}")

            self.log("\n[1/4] Читаем код...")
            if not self.source_file.exists():
                self.log(f"Файл не найден!: {self.source_file}", "ERROR")
                return False

            with open(self.source_file, 'r', encoding='utf-8') as f:
                source = f.read()
            self.log(f"  Прочитано {len(source)} байтов из {self.source_file}")

            self.log("\n[2/4] Токенизация...")
            tokens = tokenize(source, str(self.source_file))
            self.log(f"  Сгенерировано {len(tokens)} токенов")

            self.log("\n[3/4] Парсинг кода...")
            ast = parse(tokens)
            self.log(f"  AST Успешно создано!")

            try:
                from core.module_loader import inline_imports
                ast = inline_imports(ast, self.source_file)
            except Exception as e:
                self.log(f'Ошибка "Import": {e}', "ERROR")
                return False

            try:
                from core.flux_ast import Return, Literal
                main_fn = None
                self.is_gui_app = False
                
                for fn in ast.functions:
                    if fn.name == 'main':
                        main_fn = fn
                    elif fn.name == 'WinMain':
                        self.is_gui_app = True

                if main_fn and not getattr(main_fn, 'is_extern', False):
                    found_return0 = False
                    found_endofcode = False
                    for stmt in main_fn.body:
                        if isinstance(stmt, Return):
                            val = getattr(stmt, 'value', None)
                            if isinstance(val, Literal) and getattr(val, 'type', None) in ('int', 'i32', 'int32') and int(getattr(val, 'value', 0)) == 0:
                                found_return0 = True
                                if getattr(stmt, 'is_endofcode', False):
                                    found_endofcode = True
                    if found_return0 and not found_endofcode:
                        self.log('Используйте "endofcode" вместо return 0. Только по вашему желанию!')
            except Exception:
                import traceback
                traceback.print_exc()
                self.log("Постобработка AST завершилась неудачей.", "ERROR")
                return False

            self.log("\n[4/4] Генерирую код...")
            generator = CCodeGenerator(link_mode=self.link_mode, is_gui_app=getattr(self, 'is_gui_app', False))
            c_code = generator.generate(ast)

            with open(self.c_file, 'w', encoding='utf-8') as f:
                f.write(c_code)
            self.log(f"  Сгенерировано {len(c_code)} байтов Си кода.")
            self.log(f"  Си кoд сохранен в:{self.c_file}")

            self.log("\n[5/5] Компилируем Си код в исполняемый файл...")
            success = self._compile_c_to_exe()
            if success:
                print("\033[92mКомпиляция успешна!\033[0m")
            return success

        except (SyntaxError, NameError) as e:
            if DERR_FLAG:
                try:
                    debugger.critical_dump(e)
                except Exception:
                    pass
                import traceback
                traceback.print_exc()
                return False
            else:
                src = locals().get('source', None)
                try:
                    debugger.display_syntax_error(e, source=src, filename=str(self.source_file))
                except Exception:
                    print(f"[ОШИБКА!] ОШИБКА КОДА: {e}")
                return False
        except Exception as e:
            try:
                debugger.critical_dump(e)
            except Exception:
                pass
            self.log(f"[ФАТАЛЬНО] {e}", "ERROR")
            import traceback
            traceback.print_exc()
            return False
    
    def _compile_c_to_exe(self) -> bool:
        if self.compiler_type == 'gcc':
            if self.is_windows:
                return self._compile_mingw()
            else:
                return self._compile_gcc()
        elif self.compiler_type == 'clang':
            return self._compile_clang()
        elif self.compiler_type == 'lld':
            return self._compile_lld()
        
        if self.is_windows:
            return self._compile_msvc() or self._compile_mingw()
        else:
            return self._compile_gcc() or self._compile_clang() or self._compile_lld()
    
    def _compile_msvc(self) -> bool:
        self.log("Пробую MSVC...")
        
        try:
            cl_exe = self._find_msvc_cl()
            if not cl_exe:
                self.log("  MSVC не найден!", "WARN")
                return False
            
            runtime_c = Path(__file__).parent.parent / 'lib' / 'cblerr_engine_runtime.c'
            srcs = [str(self.c_file)]
            if runtime_c.exists():
                srcs.append(str(runtime_c))

            msvc_compile_flags = '/O2 /GS- /GR- /Zc:threadSafeInit- /Oi /Os /Gy'
            msvc_link_flags = '/NODEFAULTLIB /INCREMENTAL:NO /OPT:REF /OPT:ICF /ALIGN:16'
            
            if getattr(self, 'is_gui_app', False):
                msvc_link_flags += ' /SUBSYSTEM:WINDOWS /ENTRY:CblerrStartup'
            else:
                msvc_link_flags += ' /SUBSYSTEM:CONSOLE /ENTRY:CblerrStartup'
                
            if self.stack_reserve:
                msvc_link_flags += f' /STACK:{self.stack_reserve}'

            libs = ['opengl32.lib', 'winmm.lib', 'kernel32.lib', 'user32.lib', 'msvcrt.lib', 'gdi32.lib']

            cmd = [cl_exe] + msvc_compile_flags.split() + srcs + [f'/Fe{self.output_exe}', '/link'] + msvc_link_flags.split() + libs
            
            self.log(f"  Запускаю: cl.exe для компиляции кода...")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)

            exe_found = self.output_exe.exists() or (self.is_windows and self.output_exe.with_suffix('.exe').exists())

            if result.returncode == 0 and exe_found:
                self.log(f"  Компиляция через MSVC удачна!")
                try:
                    keep = SAVE_C_FLAG or os.getenv('CBLERR_KEEP_C', '0') == '1'
                    if keep:
                        self.log(f"  Оставляю Си файл из - за флага -c: {self.c_file}")
                    else:
                        if self.c_file.exists(): os.remove(self.c_file)
                        if self.obj_file.exists(): os.remove(self.obj_file)
                except Exception as e:
                    self.log(f"  Ошибка удаления: {e}", "WARN")
                return True
            else:
                combined = (result.stdout or "") + ("\n" if result.stdout and result.stderr else "") + (result.stderr or "")
                if not self._handle_compile_error(combined, self.debugger):
                    self.log(f"  Компиляция через MSVC неудачна!: {combined}", "WARN")
                return False
                
        except Exception as e:
            self.log(f"  Ошибка MSVC: {e}", "WARN")
            return False
    
    def _compile_mingw(self) -> bool:
        self.log("Пробую MinGW (gcc.exe)...")
        
        try:
            srcs = [str(self.c_file)]
            
            if self.is_windows:
                libs = ['-lopengl32', '-lwinmm', '-lmsvcrt', '-lkernel32', '-luser32', '-lgdi32']
            else:
                libs = ['-lm', '-lc']

            cflags_str = self._get_compiler_flags()
            ldflags_str = self._get_linker_flags()

            cmd = ['gcc.exe'] + cflags_str.split() + srcs + ['-o', str(self.output_exe)] + ldflags_str.split() + libs

            self.log(f"  Запускаю: gcc.exe для компиляции кода (Ультра-размер)...")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)

            exe_found = self.output_exe.exists() or (self.is_windows and self.output_exe.with_suffix('.exe').exists())

            if result.returncode == 0 and exe_found:
                try:
                    exe_p = self.output_exe if self.output_exe.exists() else self.output_exe.with_suffix('.exe')
                    phys = os.path.getsize(str(exe_p))
                    self.log(f"Размер итогового файла: {phys/1024.0:.2f} КБ")

                    keep = SAVE_C_FLAG or os.getenv('CBLERR_KEEP_C', '0') == '1'
                    if keep:
                        self.log(f"  Оставляю временный Си файл: {self.c_file}")
                    else:
                        if self.c_file.exists(): os.remove(self.c_file)
                        if self.obj_file.exists(): os.remove(self.obj_file)
                except Exception as e:
                    pass
                return True
            else:
                combined = (result.stdout or "") + ("\n" if result.stdout and result.stderr else "") + (result.stderr or "")
                if not self._handle_compile_error(combined, self.debugger):
                    self.log(f"  Компиляция через MinGW неудачна!: {combined}", "WARN")
                return False
                
        except FileNotFoundError:
            self.log("  MinGW не найден", "WARN")
            return False
        except Exception as e:
            self.log(f"  Ошибка MinGW: {e}", "WARN")
            return False

    def _compile_gcc(self) -> bool:
        self.log("Пробую GCC (Linux)...")
        
        try:
            srcs = [str(self.c_file)]
            libs = ['-lm', '-lc']

            cflags_str = self._get_compiler_flags()
            ldflags_str = self._get_linker_flags()
            
            cmd = ['gcc'] + cflags_str.split() + srcs + ['-o', str(self.output_exe)] + ldflags_str.split() + libs

            self.log(f"  Запускаю: gcc для компиляции кода...")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)

            exe_found = self.output_exe.exists()

            if result.returncode == 0 and exe_found:
                self.log(f"  Компиляция через GCC успешна!")
                return True
            else:
                combined = (result.stdout or "") + ("\n" if result.stdout and result.stderr else "") + (result.stderr or "")
                if not self._handle_compile_error(combined, self.debugger):
                    self.log(f"  Ошибка компиляции GCC: {combined}", "WARN")
                return False
                
        except FileNotFoundError:
            self.log("  GCC не найден!", "WARN")
            return False
        except Exception as e:
            self.log(f"  Ошибка GCC: {e}", "WARN")
            return False
    
    def _compile_clang(self) -> bool:
        self.log("Пробую Clang...")
        
        try:
            srcs = [str(self.c_file)]
            if self.is_windows:
                libs = ['-lopengl32', '-lwinmm', '-lkernel32', '-luser32', '-lmsvcrt', '-lgdi32']
            else:
                libs = ['-lm', '-lc']

            cflags_str = self._get_compiler_flags()
            ldflags_str = self._get_linker_flags()
            
            clang_cmd = 'clang.exe' if self.is_windows else 'clang'
            cmd = [clang_cmd] + cflags_str.split() + srcs + ['-o', str(self.output_exe)] + ldflags_str.split() + libs

            self.log(f"  Запускаю: {clang_cmd} для компиляции кода...")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)

            exe_found = self.output_exe.exists() or (self.is_windows and self.output_exe.with_suffix('.exe').exists())

            if result.returncode == 0 and exe_found:
                self.log(f"  Компиляция через Clang успешна!")
                return True
            else:
                combined = (result.stdout or "") + ("\n" if result.stdout and result.stderr else "") + (result.stderr or "")
                if not self._handle_compile_error(combined, self.debugger):
                    self.log(f"  Компиляция через Clang не удалась: {combined}", "WARN")
                return False
                
        except FileNotFoundError:
            self.log("  Clang не найден!", "WARN")
            return False
        except Exception as e:
            self.log(f"  Ошибка Clang: {e}", "WARN")
            return False
    
    def _compile_lld(self) -> bool:
        self.log("Пробую LLD...")
        return self._compile_clang()  
    
    def _find_msvc_cl(self) -> Optional[str]:
        common_paths = [
            r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.39.33519\bin\Hostx64\x64\cl.exe",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\cl.exe",
            r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\bin\Hostx64\x64\cl.exe",
        ]
        for path in common_paths:
            if Path(path).exists():
                return path
        try:
            result = subprocess.run(['where', 'cl.exe'], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip().split('\n')[0]
        except:
            pass
        
        return None


def main():
    if len(sys.argv) < 2:
        print("Использование: python build/build.py <исходный_файл.cbl> [опции]")
        print("Опции:")
        print("  -o <файл>    Путь к выходному исполняемому файлу")
        print("  -t <цель>    Целевая платформа: windows (по умолчанию) или linux")
        print("  --verbose    Показывать подробный вывод")
        print("  -c           Сохранить сгенерированный C-файл")
        print("  -static      Принудительная статическая линковка (передает -static в gcc)")
        print("  -dynamic     Принудительная динамическая линковка (использует -shared-libgcc и implib)")
        print("  --stack-size <байты|K|M>  Зарезервировать стек (в байтах или с суффиксом K/M)")
        print("  --gcc        Использовать GCC компилятор")
        print("  --clang      Использовать Clang компилятор")
        print("  --lld        Использовать Clang + LLD линкер")
        print("  --mingw      Использовать MinGW (GCC для Windows)")
        sys.exit(1)
    
    source_file = sys.argv[1]
    output_exe = None
    target = "windows"
    verbose = True
    link_mode = None
    stack_size = None
    compiler_type = None
    
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '-o' and i + 1 < len(sys.argv):
            output_exe = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '-t' and i + 1 < len(sys.argv):
            target = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--verbose':
            verbose = True
            i += 1
        elif sys.argv[i] == '-static':
            link_mode = 'static'
            i += 1
        elif sys.argv[i] == '-dynamic':
            link_mode = 'dynamic'
            i += 1
        elif sys.argv[i] == '--gcc':
            compiler_type = 'gcc'
            i += 1
        elif sys.argv[i] == '--clang':
            compiler_type = 'clang'
            i += 1
        elif sys.argv[i] == '--lld':
            compiler_type = 'lld'
            i += 1
        elif sys.argv[i] == '--mingw':
            compiler_type = 'mingw'
            i += 1
        elif sys.argv[i] in ('--stack-size',) and i + 1 < len(sys.argv):
            raw = sys.argv[i + 1]
            try:
                s = raw.strip().upper()
                if s.endswith('M'):
                    stack_size = int(float(s[:-1]) * 1024 * 1024)
                elif s.endswith('K'):
                    stack_size = int(float(s[:-1]) * 1024)
                else:
                    stack_size = int(s)
            except Exception:
                print(f"Неверный размер стека: {raw}")
                sys.exit(1)
            i += 2
        else:
            i += 1
    
    if not output_exe:
        source_path = Path(source_file)
        exe_ext = '.exe' if target == 'windows' else ''
        output_exe = source_path.stem + exe_ext
    
    compiler = StandaloneCompiler(source_file, output_exe, verbose=verbose, link_mode=link_mode, stack_reserve=stack_size, compiler_type=compiler_type)
    success = compiler.compile()
    
    if success:
        print(f"Исполняемый файл: {compiler.output_exe}")
        sys.exit(0)
    else:
        print(f"\n[НЕУДАЧА] Ошибка компиляции!")
        sys.exit(1)

if __name__ == "__main__":
    main()