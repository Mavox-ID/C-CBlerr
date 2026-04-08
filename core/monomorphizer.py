from copy import deepcopy
from typing import Any
from core.flux_ast import (
    Program, FunctionDef, Call, GenericType, StructDef,
    Assign, CastExpr, ArrayLiteral, BinaryOp, Compare, LogicalOp,
    Variable, Return, IfStmt, WhileLoop, ForLoop, MatchStmt, Case
)
from core.flux_ast import StructDef as FluxStructDef

def stringify_type(t: str) -> str:

    s = str(t)
    return s.replace('<', '_').replace('>', '_')

def type_to_str(t: Any) -> str:
    if isinstance(t, str):
        return t
    if isinstance(t, GenericType):
        args = ','.join(type_to_str(a) for a in t.args)
        return f"{t.name}<{args}>"
    return str(t)

def collect_placeholders_from_type(t: Any, acc: list[str]):
    if isinstance(t, str):
        if t and t[0].isupper() and t not in acc:
            acc.append(t)
    elif isinstance(t, GenericType):
        if t.name and t.name[0].isupper() and t.name not in acc:
            acc.append(t.name)
        for a in t.args:
            collect_placeholders_from_type(a, acc)

def collect_placeholders_from_func(f: FunctionDef) -> list[str]:
    acc: list[str] = []
    for _, ptype in f.params:
        collect_placeholders_from_type(ptype, acc)
    if f.return_type is not None:
        collect_placeholders_from_type(f.return_type, acc)
    return acc

def replace_types_in_node(node: Any, mapping: dict) -> None:

    from core.flux_ast import (
        Assign, CastExpr, FunctionDef, StructDef, Call, GenericType,
        ArrayLiteral, BinaryOp, Compare, LogicalOp, Variable, Return,
        IfStmt, WhileLoop, ForLoop, MatchStmt, Case
    )

    def resolve_type(t):
        if t is None:
            return None
        if isinstance(t, str):
            return mapping.get(t, t)
        if isinstance(t, GenericType):
            args = [resolve_type(a) for a in t.args]
            return f"{t.name}<{','.join(args)}>"
        return t

    if node is None:
        return

    if isinstance(node, list):
        for x in node:
            replace_types_in_node(x, mapping)
        return

    if isinstance(node, Assign):
        if isinstance(node.var_type, str) and node.var_type in mapping:
            node.var_type = mapping[node.var_type]
        else:
            node.var_type = resolve_type(node.var_type)
        replace_types_in_node(node.value, mapping)
        return

    if isinstance(node, CastExpr):
        node.target_type = resolve_type(node.target_type)
        replace_types_in_node(node.expr, mapping)
        return

    if isinstance(node, Call):
        for i, a in enumerate(node.args or []):
            replace_types_in_node(a, mapping)
        if node.type_args:
            new_args = []
            for ta in node.type_args:
                if isinstance(ta, str) and ta in mapping:
                    new_args.append(mapping[ta])
                else:
                    new_args.append(resolve_type(ta))
            node.type_args = new_args
        return

    if isinstance(node, GenericType):
        return

    if isinstance(node, ArrayLiteral):
        for e in node.elements:
            replace_types_in_node(e, mapping)
        return

    if isinstance(node, (BinaryOp, Compare, LogicalOp)):
        replace_types_in_node(node.left, mapping)
        if hasattr(node, 'right') and node.right is not None:
            replace_types_in_node(node.right, mapping)
        return

    if isinstance(node, IfStmt):
        replace_types_in_node(node.condition, mapping)
        for s in node.then_body:
            replace_types_in_node(s, mapping)
        if node.else_body:
            for s in node.else_body:
                replace_types_in_node(s, mapping)
        return

    if isinstance(node, WhileLoop):
        replace_types_in_node(node.condition, mapping)
        for s in node.body:
            replace_types_in_node(s, mapping)
        return

    if isinstance(node, ForLoop):
        if node.init:
            replace_types_in_node(node.init, mapping)
        if node.condition:
            replace_types_in_node(node.condition, mapping)
        if node.post:
            replace_types_in_node(node.post, mapping)
        for s in node.body:
            replace_types_in_node(s, mapping)
        return

    if isinstance(node, MatchStmt):
        replace_types_in_node(node.expr, mapping)
        for c in node.cases:
            for v in (c.values or []):
                replace_types_in_node(v, mapping)
            for s in c.body:
                replace_types_in_node(s, mapping)
        return

    if isinstance(node, Return):
        if node.value is not None:
            replace_types_in_node(node.value, mapping)
        return

    if isinstance(node, Variable):
        return

    for attr in ('left', 'right', 'condition', 'value', 'expr', 'body'):
        if hasattr(node, attr):
            val = getattr(node, attr)
            replace_types_in_node(val, mapping)

def monomorphize(program: Program) -> Program:
    prog = deepcopy(program)

    def find_function(name: str) -> FunctionDef | None:
        for f in prog.functions:
            if f.name == name:
                return f
        return None

    def clone_and_instantiate_function(orig_name: str, type_args: list[Any]) -> str | None:
        orig = find_function(orig_name)
        if not orig:
            return None

        placeholders = collect_placeholders_from_func(orig)
        mapping: dict = {}
        for i, ph in enumerate(placeholders):
            if i < len(type_args):
                concrete = type_to_str(type_args[i])
                mapping[ph] = concrete

        mangled_suffix = '__'.join(stringify_type(type_to_str(t)) for t in type_args)
        mangled_name = f"{orig.name}__{mangled_suffix}"

        if find_function(mangled_name):
            return mangled_name

        new_def = deepcopy(orig)
        new_def.name = mangled_name

        new_params = []
        for pname, ptype in new_def.params:
            if isinstance(ptype, str) and ptype in mapping:
                new_params.append((pname, mapping[ptype]))
            else:
                new_params.append((pname, type_to_str(ptype) if not isinstance(ptype, str) else ptype))
        new_def.params = new_params

        if isinstance(new_def.return_type, str) and new_def.return_type in mapping:
            new_def.return_type = mapping[new_def.return_type]
        else:
            if new_def.return_type is not None:
                new_def.return_type = type_to_str(new_def.return_type)

        for stmt in new_def.body:
            replace_types_in_node(stmt, mapping)

        prog.functions.append(new_def)
        return mangled_name

    def find_struct(name: str) -> StructDef | None:
        for s in prog.structs:
            if hasattr(s, 'name') and s.name == name:
                return s
        return None

    def clone_and_instantiate_struct(orig_name: str, type_args: list[Any]) -> str | None:
        orig = find_struct(orig_name)
        if not orig:
            return None

        placeholders: list[str] = []
        for _, ftype in (orig.fields if isinstance(orig.fields, list) else orig.fields.items()):
            collect_placeholders_from_type(ftype, placeholders)

        mapping: dict = {}
        for i, ph in enumerate(placeholders):
            if i < len(type_args):
                mapping[ph] = type_to_str(type_args[i])

        mangled_suffix = '__'.join(stringify_type(type_to_str(t)) for t in type_args)
        mangled_name = f"{orig.name}__{mangled_suffix}"

        if find_struct(mangled_name):
            return mangled_name

        new_def = deepcopy(orig)
        new_def.name = mangled_name

        for i, (fname, ftype) in enumerate(new_def.fields):
            if isinstance(ftype, str) and ftype in mapping:
                new_def.fields[i] = (fname, mapping[ftype])
            else:
                replace_types_in_node(ftype, mapping)

        prog.structs.append(new_def)
        return mangled_name

    def visit(node: Any):
        if node is None:
            return None
        if isinstance(node, list):
            for i, x in enumerate(node):
                node[i] = visit(x)
            return node

        if isinstance(node, Call):
            node.args = [visit(a) for a in node.args]

            if node.type_args:
                mangled = clone_and_instantiate_function(node.func_name, node.type_args)
                if mangled:
                    node.func_name = mangled
                node.type_args = None
            return node

        if isinstance(node, GenericType):
            mangled = clone_and_instantiate_struct(node.name, node.args)
            if mangled:
                return mangled
            return type_to_str(node)

        if hasattr(node, '__dict__'):
            for k, v in vars(node).items():
                if isinstance(v, list):
                    for i, x in enumerate(v):
                        v[i] = visit(x)
                else:
                    setattr(node, k, visit(v))

        return node

    for f in list(prog.functions):
        f.body = [visit(s) for s in f.body]

    prog.global_vars = [visit(g) for g in prog.global_vars]
    prog.structs = [visit(s) for s in prog.structs]

    return prog
