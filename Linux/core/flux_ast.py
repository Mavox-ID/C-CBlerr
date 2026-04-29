from dataclasses import dataclass, field
from typing import Any

@dataclass
class Literal:
    value: Any
    type: str

@dataclass
class StringLiteral:
    value: str
    length: int

@dataclass
class Variable:
    name: str

@dataclass
class BinaryOp:
    op: str
    left: Any
    right: Any

@dataclass
class Compare:
    op: str
    left: Any
    right: Any

@dataclass
class LogicalOp:
    op: str
    left: Any
    right: Any | None = None

@dataclass
class WalrusExpr:
    target: Any
    value: Any

@dataclass
class Assign:
    target: str
    value: Any
    var_type: str | None = None

@dataclass
class Return:
    value: Any | None = None
    is_endofcode: bool = False

@dataclass
class IfStmt:
    condition: Any
    then_body: list[Any]
    else_body: list[Any] | None = None

@dataclass
class WhileLoop:
    condition: Any
    body: list[Any]

@dataclass
class BreakStmt:
    pass

@dataclass
class ContinueStmt:
    pass

@dataclass
class Call:
    func_name: Any
    args: list[Any]
    type_args: list[Any] | None = None

@dataclass
class FieldAccess:
    obj: Any
    field: str

@dataclass
class ArrayAccess:
    arr: Any
    index: Any

@dataclass
class ArrayLiteral:
    elements: list[Any]
    array_type: str | None = None

@dataclass
class PointerType:
    base_type: str

@dataclass
class Dereference:
    ptr: Any
    index: Any | None = None

@dataclass
class InlineAsm:
    code: str
    outputs: str = ""
    inputs: str = ""
    clobbers: str = ""
    volatile: bool = True

@dataclass
class CastExpr:
    expr: Any
    target_type: Any

@dataclass
class Decorator:
    name: str
    args: list[str] | None = None

@dataclass
class ComptimeBlock:
    code: str

@dataclass
class Case:
    values: list[Any] | None
    body: list[Any]

@dataclass
class MatchStmt:
    expr: Any
    cases: list[Case]

@dataclass
class ForLoop:
    iter_var: str | None
    iter_expr: Any | None
    init: Any | None
    condition: Any | None
    post: Any | None
    body: list[Any]

@dataclass
class EnumDef:
    name: str
    members: list[tuple[str, Any | None]]

@dataclass
class AddressOf:
    expr: Any

@dataclass
class SizeOf:
    target: Any

@dataclass
class GenericType:
    name: str
    args: list[Any]

@dataclass
class GlobalVariable:
    name: str
    var_type: str
    value: Any | None = None
    is_const: bool = False

@dataclass
class FunctionDef:
    name: str
    params: list[tuple[str, Any]]
    return_type: Any | None
    body: list[Any]
    is_extern: bool = False
    decorators: list[Decorator] | None = None
    is_vararg: bool = False

@dataclass
class StructDef:
    name: str
    fields: list[tuple[str, Any]]
    decorators: list[Decorator] | None = None

@dataclass
class ImportStmt:
    module_name: str
    items: list[str] | None = None

@dataclass
class FromImportStmt:
    module_name: str
    items: list[str]
    aliases: dict | None = None

@dataclass
class Program:
    functions: list[FunctionDef] = field(default_factory=list)
    structs: list[StructDef] = field(default_factory=list)
    imports: list[Any] = field(default_factory=list)
    global_vars: list[GlobalVariable] = field(default_factory=list)

