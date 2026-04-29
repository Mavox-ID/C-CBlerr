from typing import Any, Dict, List
from core.debugger import get_debugger
from core.flux_ast import (
    Program, FunctionDef, StructDef, GlobalVariable,
    Variable, FieldAccess, Call, Literal, ArrayLiteral,
    Assign, IfStmt, WhileLoop, Return, Case, MatchStmt,
    BreakStmt, ContinueStmt,
    ForLoop, EnumDef, AddressOf, SizeOf, GenericType, CastExpr, ArrayAccess
)

class SemanticError(Exception):
    pass

def _type_to_str(t: Any) -> str:
    if t is None:
        raise SemanticError("Invalid expression type") 
    if isinstance(t, str):
        return t
    if isinstance(t, GenericType):
        args = ','.join(_type_to_str(a) for a in t.args)
        return f"{t.name}<{args}>"
    return str(t)

class TypeChecker:
    def __init__(self, debugger=None):
        self.debugger = debugger or get_debugger()
        self.struct_fields: Dict[str, Dict[str, Any]] = {}
        self.functions: Dict[str, Dict[str, Any]] = {}

        self.globals: Dict[str, str] = {
            'true': 'bool',
            'false': 'bool'
        }

        self.current_return_type: str | None = None

        self.reserved_functions = {
            'printf', 'malloc', 'free', 'exit', 'memcpy', 'memset', 'puts', 'putchar', 'scanf'
        }

    def _collect_calls(self, node: Any) -> set:
        calls = set()
        if node is None:
            return calls
        if isinstance(node, Call):
            if isinstance(node.func_name, str):
                calls.add(node.func_name)
            if not isinstance(node.func_name, str):
                calls |= self._collect_calls(node.func_name)
            for a in (node.args or []):
                calls |= self._collect_calls(a)
            return calls
        if isinstance(node, (list, tuple)):
            for n in node:
                calls |= self._collect_calls(n)
            return calls
        if hasattr(node, '__dict__'):
            for v in vars(node).values():
                if isinstance(v, (list, tuple)):
                    for x in v:
                        calls |= self._collect_calls(x)
                else:
                    calls |= self._collect_calls(v)
        return calls

    def check(self, program: Program) -> Program:
        for s in program.structs:
            if isinstance(s, StructDef):
                fields = {}
                if isinstance(s.fields, dict):
                    for k, v in s.fields.items():
                        fields[k] = v
                else:
                    for name, t in s.fields:
                        fields[name] = t
                self.struct_fields[s.name] = fields

        for f in program.functions:
            param_types = [_type_to_str(pt) for _, pt in f.params]
            ret = _type_to_str(f.return_type) if f.return_type is not None else None
            self.functions[f.name] = {'params': param_types, 'return': ret}

        for g in program.global_vars:
            if hasattr(g, 'var_type'):
                self.globals[g.name] = _type_to_str(g.var_type)
            if getattr(g, 'value', None) is not None:
                from core.flux_ast import StringLiteral
                if not isinstance(g.value, (Literal, StringLiteral)):
                    self._error(f"Global variable '{g.name}' must be initialized with a constant literal (string, number, or boolean); complex initialization of global variables must be done in main()")

        found_non_extern = False
        for f in program.functions:
            is_extern = getattr(f, 'is_extern', False)
            if is_extern and found_non_extern:
                self._error("extern declarations must be placed at the top of the file")
            if not is_extern:
                found_non_extern = True

        name_to_index = {fn.name: idx for idx, fn in enumerate(program.functions)}
        for idx, fn in enumerate(program.functions):
            if fn.name == 'main' and idx != len(program.functions) - 1:
                self._error("main() MUST ALWAYS be the last function in the file")

        for idx, fn in enumerate(program.functions):
            calls = self._collect_calls(fn.body)
            for callee in calls:
                if callee in name_to_index and name_to_index[callee] > idx:
                    self._error(f"Function '{callee}' must be defined before it is called by '{fn.name}' (Top-Down Rule)")

        for f in program.functions:
            if f.name in self.reserved_functions and not getattr(f, 'is_extern', False):
                self._error(f"Redefinition of reserved function '{f.name}' is not allowed; declare it as extern to use the runtime implementation.")

        for f in program.functions:
            if f.return_type is not None:
                ret_str = _type_to_str(f.return_type)
                if ret_str.startswith('i'):
                    if not f.body or not isinstance(f.body[-1], Return):
                        self._error(f"Function '{f.name}', declared with return type '{ret_str}', must end with an explicit return statement (<expr>")

        for f in program.functions:
            self._check_function(f)

        return program

    def _error(self, msg: str) -> None:
        self.debugger.log_error(msg)
        raise SemanticError(msg)

    def _check_function(self, f: FunctionDef) -> None:
        symbols: Dict[str, str] = {}
        origins: Dict[str, str] = {}
        for name, ptype in f.params:
            symbols[name] = _type_to_str(ptype)
            origins[name] = 'param'

        self.current_return_type = _type_to_str(f.return_type) if f.return_type is not None else None

        for stmt in f.body:
            self._check_statement(stmt, symbols, origins)

        self.current_return_type = None

    def _check_statement(self, stmt: Any, symbols: Dict[str, str], origins: Dict[str, str]) -> None:
        match stmt:
            case Assign():
                val_t = self._check_expression(stmt.value, symbols, origins)
                declared = getattr(stmt, 'var_type', None)
                if declared:
                    declared_str = _type_to_str(declared)
                    if declared_str != val_t:
                        self._error(f"Type mismatch in assignment '{stmt.target}': {declared_str} != {val_t}")
                    if isinstance(stmt.target, str):
                        symbols[stmt.target] = declared_str
                else:
                    if isinstance(stmt.target, str):
                        symbols[stmt.target] = val_t

                if isinstance(stmt.target, str):
                    tname = stmt.target
                    origins[tname] = 'local'
                    if isinstance(stmt.value, Call) and getattr(stmt.value, 'func_name', '') in ('malloc', 'calloc'):
                        cname = stmt.value.func_name if isinstance(stmt.value.func_name, str) else (stmt.value.func_name.name if isinstance(stmt.value.func_name, Variable) else None)
                        if cname in ('malloc', 'calloc'):
                            origins[tname] = 'heap'
                    elif isinstance(stmt.value, ArrayLiteral):
                        origins[tname] = 'stack_array'
                    elif isinstance(stmt.value, AddressOf):
                        inner = stmt.value.expr
                        if isinstance(inner, ArrayLiteral):
                            origins[tname] = 'stack_ptr'
                        elif isinstance(inner, Variable):
                            inner_name = inner.name
                            if origins.get(inner_name) in ('stack_array', 'stack_ptr'):
                                origins[tname] = 'stack_ptr'
                            elif origins.get(inner_name) == 'heap':
                                origins[tname] = 'heap'

            case Return():
                if stmt.value is not None:
                    if isinstance(stmt.value, AddressOf):
                        inner = stmt.value.expr
                        if isinstance(inner, ArrayLiteral):
                            self._error("Returning the address of a local array literal is forbidden; allocate on the heap using malloc and return that pointer.")
                        if isinstance(inner, Variable):
                            vname = inner.name
                            if origins.get(vname) != 'heap':
                                self._error("Returning address of a local variable is forbidden; allocate on heap with malloc to return a pointer")
                        if isinstance(inner, FieldAccess):
                            base = inner.obj
                            if isinstance(base, Variable):
                                bname = base.name
                                if origins.get(bname) != 'heap':
                                    self._error("Returning the address of a local struct field is forbidden; to return a pointer to its field, allocate the struct on the heap.")
                    if isinstance(stmt.value, Variable):
                        v = stmt.value.name
                        if origins.get(v) in ('stack_ptr', 'stack_array'):
                            self._error("Returning a pointer to a locally allocated array/variable is forbidden; allocate on the heap and return that pointer.")

                    vtype = self._check_expression(stmt.value, symbols, origins)
                else:
                    vtype = 'void'

                if self.current_return_type is not None:
                    expected = self.current_return_type
                    if expected == 'void':
                        if vtype != 'void':
                            self._error(f"Function declared as void but return contains a value of type {vtype}")
                    else:
                        if vtype == 'void':
                            self._error(f"Function declared as {expected}, but return has no value")
                        else:
                            if isinstance(expected, str) and isinstance(vtype, str):
                                if expected.startswith('i') and vtype.startswith('i'):
                                    pass
                                elif expected != vtype:
                                    self._error(f"Return type mismatch: expected {expected}, got {vtype}")
                            else:
                                if expected != vtype:
                                    self._error(f"Return type mismatch: expected {expected}, got {vtype}")

            case IfStmt():
                cond_t = self._check_expression(stmt.condition, symbols, origins)
                if cond_t not in ('int', 'bool'):
                    self._error(f"If condition must be boolean/int, got {cond_t}")
                for s in stmt.then_body:
                    self._check_statement(s, symbols, origins)
                if stmt.else_body:
                    for s in stmt.else_body:
                        self._check_statement(s, symbols, origins)

            case WhileLoop():
                cond_t = self._check_expression(stmt.condition, symbols, origins)
                if cond_t not in ('int', 'bool'):
                    self._error(f"While condition must be boolean/int, got {cond_t}")
                for s in stmt.body:
                    self._check_statement(s, symbols, origins)

            case ContinueStmt():
                return

            case BreakStmt():
                return

            case ForLoop():
                if stmt.init:
                    if isinstance(stmt.init, Assign):
                        self._check_statement(stmt.init, symbols, origins)
                    else:
                        self._check_expression(stmt.init, symbols, origins)
                if stmt.condition:
                    ct = self._check_expression(stmt.condition, symbols, origins)
                    if ct not in ('int', 'bool'):
                        self._error(f"For condition must be boolean/int, got {ct}")
                if stmt.post:
                    if isinstance(stmt.post, Assign):
                        self._check_statement(stmt.post, symbols, origins)
                    else:
                        self._check_expression(stmt.post, symbols, origins)
                inner_sym = dict(symbols)
                inner_origins = dict(origins)
                if stmt.iter_var:
                    iter_type = 'int'
                    if getattr(stmt, 'iter_expr', None) is not None:
                        try:
                            iter_expr_t = self._check_expression(stmt.iter_expr, symbols, origins)
                            if isinstance(iter_expr_t, str):
                                if iter_expr_t.startswith('array<') and iter_expr_t.endswith('>'):
                                    iter_type = iter_expr_t[6:-1]
                                elif iter_expr_t.startswith('*'):
                                    iter_type = iter_expr_t[1:]
                        except SemanticError:
                            pass
                    inner_sym[stmt.iter_var] = iter_type
                    inner_origins[stmt.iter_var] = 'local'
                for s in stmt.body:
                    self._check_statement(s, inner_sym, inner_origins)

            case MatchStmt():
                e_t = self._check_expression(stmt.expr, symbols, origins)
                for c in stmt.cases:
                    if c.values is not None:
                        for v in c.values:
                            vt = self._check_expression(v, symbols, origins)
                            if vt != e_t:
                                self._error(f"Type of match case value {vt} differs from the match expression type. {e_t}")
                    for s in c.body:
                        self._check_statement(s, symbols, origins)

            case _:
                self._check_expression(stmt, symbols, origins)

    def _check_expression(self, expr: Any, symbols: Dict[str, str], origins: Dict[str, str] = None) -> str:
        if expr is None:
            self._error("Invalid expression type")

        if isinstance(expr, Literal):
            expr.resolved_type = expr.type
            return expr.type

        if isinstance(expr, Variable):
            if expr.name in symbols:
                expr.resolved_type = symbols[expr.name]
                return expr.resolved_type
            if expr.name in self.globals:
                expr.resolved_type = self.globals[expr.name]
                return expr.resolved_type
            if expr.name in self.functions:
                sig = self.functions[expr.name]
                params = sig.get('params', []) or []
                ret = sig.get('return', None) or 'void'
                params_str = ','.join(params)
                fn_str = f"fn({params_str})->{ret}"
                expr.resolved_type = fn_str
                return expr.resolved_type
            self._error(f"Undefined variable '{expr.name}'")

        if isinstance(expr, FieldAccess):
            obj_t = self._check_expression(expr.obj, symbols, origins)
            if isinstance(obj_t, str) and obj_t == 'str':
                if expr.field == 'data':
                    expr.resolved_type = '*void'
                    return '*void'
                if expr.field == 'length':
                    expr.resolved_type = 'i32'
                    return 'i32'
            struct_name = None
            if isinstance(obj_t, str) and obj_t.startswith('*'):
                struct_name = obj_t[1:]
            elif isinstance(obj_t, str) and obj_t.startswith('struct '):
                struct_name = obj_t.split(' ', 1)[1]
            elif isinstance(obj_t, str) and obj_t in self.struct_fields:
                struct_name = obj_t

            if not struct_name or struct_name not in self.struct_fields:
                self._error(f"Field access on non-struct type: {obj_t}")

            if expr.field not in self.struct_fields[struct_name]:
                self._error(f"Field '{expr.field}' not found in struct '{struct_name}'")

            ftype = _type_to_str(self.struct_fields[struct_name][expr.field])
            expr.resolved_type = ftype
            return ftype

        if isinstance(expr, Call):
            if isinstance(expr.func_name, str) and expr.func_name == 'print':
                for a in (expr.args or []):
                    self._check_expression(a, symbols, origins)
                expr.resolved_type = 'void'
                return 'void'

            if isinstance(expr.func_name, str) and expr.func_name == 'range':
                for a in expr.args:
                    self._check_expression(a, symbols, origins)
                expr.resolved_type = 'range'
                return 'range'

            if isinstance(expr.func_name, str) and expr.func_name == 'len':
                if not expr.args or len(expr.args) != 1:
                    self._error("len() expects a single argument")
                at = self._check_expression(expr.args[0], symbols, origins)
                expr.resolved_type = 'int'
                return 'int'

            if isinstance(expr.func_name, str):
                if expr.func_name not in self.functions:
                    self._error(f"Call to unknown function '{expr.func_name}'")
                sig = self.functions[expr.func_name]
                args = expr.args or []
                for i, a in enumerate(args):
                    at = self._check_expression(a, symbols, origins)
                    if i < len(sig['params']) and sig['params'][i] is not None:
                        expected = sig['params'][i]
                        if at == 'str' and isinstance(expected, str) and expected.startswith('*'):
                            ok = False
                            if isinstance(a, FieldAccess) and a.field == 'data':
                                ok = True
                            if isinstance(a, CastExpr) and isinstance(a.expr, FieldAccess) and a.expr.field == 'data':
                                ok = True
                            if not ok:
                                self._error(f"Passing `str` directly to function '{expr.func_name}'; pass `<your_string>.data as *void` if a pointer is expected.")
                        if at != expected:
                            if not (isinstance(at, str) and at.startswith('i') and isinstance(expected, str) and expected.startswith('i')):
                                self._error(f"Argument type mismatch in call to function '{expr.func_name}': {at} != {expected}")
                expr.resolved_type = sig['return'] or 'void'
                return expr.resolved_type

            callee_t = self._check_expression(expr.func_name, symbols, origins)

            def _parse_fn_sig(sig: str) -> tuple[list[str], str]:
                if not sig.startswith('fn('):
                    raise SemanticError(f"Invalid function signature: {sig}")
                i = 3
                depth_paren = 0
                depth_angle = 0
                params_buf = ''
                while i < len(sig):
                    c = sig[i]
                    if c == '(':
                        depth_paren += 1
                        params_buf += c
                    elif c == ')':
                        if depth_paren == 0:
                            break
                        depth_paren -= 1
                        params_buf += c
                    else:
                        params_buf += c
                    i += 1

                start = sig.find('(', 2)
                if start == -1:
                    raise SemanticError(f"Invalid function signature: {sig}")
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
                    raise SemanticError(f"Unbalanced parentheses in function signature: {sig}")
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
                            gdepth += 1
                            cur += ch
                        elif ch == '>':
                            gdepth -= 1
                            cur += ch
                        elif ch == '(':
                            pdepth += 1
                            cur += ch
                        elif ch == ')':
                            pdepth -= 1
                            cur += ch
                        elif ch == ',' and pdepth == 0 and gdepth == 0:
                            params.append(cur.strip())
                            cur = ''
                        else:
                            cur += ch
                    if cur.strip():
                        params.append(cur.strip())
                return params, ret

            if isinstance(callee_t, str) and (callee_t.startswith('*fn(') or callee_t.startswith('fn(')):
                sig_str = callee_t[1:] if callee_t.startswith('*') else callee_t
                params, ret = _parse_fn_sig(sig_str)
                args = expr.args or []
                if len(args) != len(params):
                    self._error(f"Argument count mismatch in pointer call: {len(args)} != {len(params)}")
                for i, a in enumerate(args):
                    at = self._check_expression(a, symbols, origins)
                    expected = params[i]
                    if at == 'str' and isinstance(expected, str) and expected.startswith('*'):
                        ok = False
                        if isinstance(a, FieldAccess) and a.field == 'data':
                            ok = True
                        if isinstance(a, CastExpr) and isinstance(a.expr, FieldAccess) and a.expr.field == 'data':
                            ok = True
                        if not ok:
                            self._error(f"Passing `str` directly to a function pointer; pass `<your_string>.data as *void` if a pointer is expected.")
                    if at != expected:
                        if not (isinstance(at, str) and at.startswith('i') and isinstance(expected, str) and expected.startswith('i')):
                            self._error(f"Argument type mismatch in pointer call: {at} != {expected}")
                expr.resolved_type = ret or 'void'
                return expr.resolved_type

            self._error(f"Call on object is not a function pointer: {callee_t}")

        if isinstance(expr, ArrayLiteral):
            if not expr.elements:
                self._error("Empty array literal is not supported for type inference")
            elem_t = self._check_expression(expr.elements[0], symbols, origins)
            for e in expr.elements[1:]:
                t = self._check_expression(e, symbols, origins)
                if t != elem_t:
                    self._error(f"Array literal elements must have the same type: {t} != {elem_t}")
            expr.resolved_type = f"array<{elem_t}>"
            return expr.resolved_type

        if isinstance(expr, CastExpr):
            _ = self._check_expression(expr.expr, symbols, origins)
            tgt = _type_to_str(expr.target_type)
            expr.resolved_type = tgt
            return tgt

        if isinstance(expr, ArrayAccess):
            arr_t = self._check_expression(expr.arr, symbols, origins)
            _ = self._check_expression(expr.index, symbols, origins)
            if isinstance(arr_t, str):
                if arr_t.startswith('*'):
                    inner = arr_t[1:]
                    expr.resolved_type = inner
                    return inner
                if arr_t.startswith('array<') and arr_t.endswith('>'):
                    inner = arr_t[6:-1]
                    expr.resolved_type = inner
                    return inner
            expr.resolved_type = 'void'
            self._error("Invalid expression type")

        if isinstance(expr, AddressOf):
            inner_t = self._check_expression(expr.expr, symbols, origins)
            ptr_t = f"*{inner_t}"
            expr.resolved_type = ptr_t
            return ptr_t

        if isinstance(expr, SizeOf):
            expr.resolved_type = 'int'
            return 'int'

        if hasattr(expr, 'op') and hasattr(expr, 'left') and getattr(expr, 'right', None) is None:
            if expr.op == 'not':
                lt = self._check_expression(expr.left, symbols, origins)
                if lt not in ('int', 'bool'):
                    self._error(f"Logical operator 'not' requires boolean/int, got {lt}")
                expr.resolved_type = 'bool'
                return 'bool'

        if hasattr(expr, 'op') and hasattr(expr, 'left') and getattr(expr, 'right', None) is not None:
            lt = self._check_expression(expr.left, symbols, origins)
            rt = self._check_expression(expr.right, symbols, origins)
            if lt != rt:
                if not (lt.startswith('i') and rt.startswith('i')):
                    self._error(f"Binary operator type mismatch: {lt} {expr.op} {rt}")
            if expr.op == '+' and lt == 'str' and rt == 'str':
                expr.resolved_type = 'str'
                return 'str'
            if expr.op in ('+', '-', '*', '/', '%') and lt.startswith('i') and rt.startswith('i'):
                expr.resolved_type = lt
                return lt
            if expr.op in ('<<', '>>', '&', '|', '^') and lt.startswith('i') and rt.startswith('i'):
                expr.resolved_type = lt
                return lt
            if expr.op in ('==', '!=', '<', '>', '<=', '>='):
                if (isinstance(lt, str) and lt.startswith('f')) or (isinstance(rt, str) and rt.startswith('f')):
                    self._error("Direct float comparison is not supported in the current backend; convert to int or use a helper function")
                expr.resolved_type = 'bool'
                return 'bool'
            if expr.op in ('and', 'or'):
                if lt not in ('int', 'bool') or rt not in ('int', 'bool'):
                    self._error("Logical operators 'and'/'or' must operate on boolean/int values; do not use them for null checks. Use nested if-expressions instead.")
                expr.resolved_type = 'bool'
                return 'bool'

        if hasattr(expr, '__dict__'):
            for _, v in vars(expr).items():
                if isinstance(v, (list, tuple)):
                    for x in v:
                        if hasattr(x, '__dict__'):
                            self._check_expression(x, symbols, origins)
                elif hasattr(v, '__dict__'):
                    self._check_expression(v, symbols, origins)

        self._error(f"Cannot determine expression type for node: {type(expr).__name__}")