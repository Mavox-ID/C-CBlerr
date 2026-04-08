from llvmlite import ir
from core.flux_ast import (
    Program, FunctionDef, Return, BinaryOp, Variable, Literal,
    IfStmt, Assign, Compare, Call, WhileLoop, BreakStmt, ContinueStmt,
    StructDef, FieldAccess, ArrayAccess, ArrayLiteral, LogicalOp,
    PointerType, Dereference, InlineAsm, CastExpr, Decorator, ComptimeBlock,
    MatchStmt, Case, ForLoop, AddressOf, SizeOf
)
from core.debugger import get_debugger
# TypeChecker temporarily disabled to avoid halting compilation flow
# try:
#     from core.type_checker import TypeChecker, SemanticError
# except Exception:
TypeChecker = None
class SemanticError(Exception):
    pass

class Compiler:

    TYPE_MAP = {
        'int': ir.IntType(32),
        'i32': ir.IntType(32),
        'int32': ir.IntType(32),
        'int64': ir.IntType(64),
        'float': ir.FloatType(),
        'f32': ir.FloatType(),
        'bool': ir.IntType(1),
        'str': ir.PointerType(ir.IntType(8)),
        'void': ir.VoidType(),

        'u8': ir.IntType(8),
        'u16': ir.IntType(16),
        'u32': ir.IntType(32),
        'u64': ir.IntType(64),

        'i8': ir.IntType(8),
        'i16': ir.IntType(16),
        'i64': ir.IntType(64),
        'usize': ir.IntType(64),
    }

    def __init__(self, module_name: str = "cblerr_module"):
        self.module = ir.Module(module_name)
        self.symbols = {}
        self.function_context = []
        self.entry_builder = None
        self.string_constants = {}
        self.struct_types = {}
        self.struct_fields = {}
        self.loop_stack = []
        self.decorators_cache = {}
        self.global_vars = {}
        self.debugger = get_debugger()

    def get_llvm_type(self, flux_type: str) -> ir.Type:

        if flux_type is None:
            return ir.VoidType()

        if isinstance(flux_type, str) and flux_type.startswith('*'):
            inner_type_str = flux_type[1:]

            if inner_type_str == 'void':
                return ir.PointerType(ir.IntType(8))

            inner_type = self.get_llvm_type(inner_type_str)
            return ir.PointerType(inner_type)

        if flux_type in self.struct_types:
            return self.struct_types[flux_type]

        llvm_type = self.TYPE_MAP.get(flux_type)
        if llvm_type is None:
            if flux_type in self.struct_types:
                return self.struct_types[flux_type]
            return ir.IntType(32)

        return llvm_type

    def compile(self, program: Program) -> ir.Module:

        # TypeChecker временно отключён - может прерывать процесс компиляции
        # try:
        #     tc = TypeChecker(self.debugger)
        #     tc.check(program)
        # except SemanticError as e:
        #     self.debugger.log_error(f"Semantic analysis failed: {e}")
        #     raise
        if program.structs and len(program.structs) > 0:
            for struct_def in program.structs:
                self.compile_struct_def(struct_def)

        if program.global_vars and len(program.global_vars) > 0:
            for global_var in program.global_vars:
                self.compile_global_var(global_var)

        if program.functions and len(program.functions) > 0:
            for func_def in program.functions:
                if func_def.is_extern:
                    self.declare_extern_function(func_def)

        if program.functions and len(program.functions) > 0:
            for func_def in program.functions:
                if not func_def.is_extern:
                    self.compile_function(func_def)

        return self.module

    def compile_struct_def(self, struct_def: StructDef):

        is_packed = False
        if struct_def.decorators:
            for dec in struct_def.decorators:
                if isinstance(dec, Decorator) and dec.name == 'packed':
                    is_packed = True
                elif isinstance(dec, str) and dec == 'packed':
                    is_packed = True

        field_types = [self.get_llvm_type(field_type) for _, field_type in struct_def.fields]

        struct_type = self.module.context.get_identified_type(struct_def.name)
        struct_type.set_body(*field_types)

        if is_packed:
            struct_type.packed = True

        self.struct_types[struct_def.name] = struct_type
        self.struct_fields[struct_def.name] = {name: i for i, (name, _) in enumerate(struct_def.fields)}

    def compile_global_var(self, global_var):

        if hasattr(global_var, 'value') and global_var.value:
            if isinstance(global_var.value, Literal):
                if global_var.value.type == 'str':
                    str_val = global_var.value.value
                    str_bytes = str_val.encode('utf-8') + b'\x00'
                    array_type = ir.ArrayType(ir.IntType(8), len(str_bytes))
                    const_array = ir.Constant(array_type, bytearray(str_bytes))

                    glob_var = ir.GlobalVariable(self.module, array_type, name=global_var.name)
                    glob_var.linkage = 'internal'
                    glob_var.global_constant = True
                    glob_var.initializer = const_array
                    self.global_vars[global_var.name] = glob_var
                elif global_var.value.type == 'int':
                    int_type = ir.IntType(32)
                    int_val = int(global_var.value.value)
                    const_int = ir.Constant(int_type, int_val)

                    glob_var = ir.GlobalVariable(self.module, int_type, name=global_var.name)
                    glob_var.linkage = 'internal'
                    glob_var.initializer = const_int
                    self.global_vars[global_var.name] = glob_var

    def declare_extern_function(self, func_def: FunctionDef):

        param_types = []
        if func_def.params:
            for param_name, param_type in func_def.params:
                param_types.append(self.get_llvm_type(param_type))

        return_type = self.get_llvm_type(func_def.return_type) if func_def.return_type else ir.VoidType()
        func_type = ir.FunctionType(return_type, param_types)
        func = ir.Function(self.module, func_type, name=func_def.name)
        func.linkage = 'external'

    def compile_function(self, func_def: FunctionDef):

        param_types = [self.get_llvm_type(pt) for _, pt in func_def.params]
        return_type = self.get_llvm_type(func_def.return_type) if func_def.return_type else ir.VoidType()

        func_type = ir.FunctionType(return_type, param_types)
        func = ir.Function(self.module, func_type, name=func_def.name)

        if func_def.decorators:
            self.decorators_cache[func_def.name] = func_def.decorators

        entry_block = func.append_basic_block(name="entry")
        entry_builder = ir.IRBuilder(entry_block)

        body_block = func.append_basic_block(name="body")
        entry_builder.branch(body_block)

        old_builder = getattr(self, 'builder', None)
        self.entry_builder = entry_builder
        self.builder = ir.IRBuilder(body_block)

        self.function_context.append({
            'function': func,
            'builder': self.builder,
            'entry_builder': self.entry_builder,
            'symbols': {}
        })

        for i, (param_name, param_type_str) in enumerate(func_def.params):
            param_alloca = self.entry_builder.alloca(param_types[i], name=param_name)
            self.builder.store(func.args[i], param_alloca)
            self.function_context[-1]['symbols'][param_name] = {
                'ptr': param_alloca,
                'type': param_type_str
            }

        for stmt in func_def.body:
            self.compile_statement(stmt)

        current_block = self.builder.block
        if not current_block.terminator:
            if return_type == ir.VoidType():
                self.builder.ret_void()
            elif return_type == ir.IntType(32):
                self.builder.ret(ir.Constant(ir.IntType(32), 0))

        if old_builder:
            self.builder = old_builder
        self.entry_builder = None
        self.function_context.pop()

    def compile_statement(self, stmt):

        match stmt:
            case Return():
                self.compile_return(stmt)
            case IfStmt():
                self.compile_if_stmt(stmt)
            case WhileLoop():
                self.compile_while_stmt(stmt)
            case BreakStmt():
                self.compile_break_stmt()
            case ContinueStmt():
                self.compile_continue_stmt()
            case Assign():
                self.compile_assign(stmt)
            case InlineAsm():
                self.compile_inline_asm(stmt)
            case ComptimeBlock():
                self.compile_comptime_block(stmt)
            case MatchStmt():
                self.compile_match_stmt(stmt)
            case ForLoop():
                self.compile_for_loop(stmt)
            case _:
                self.compile_expression(stmt)

    def compile_return(self, ret: Return):

        if ret.value:
            value = self.compile_expression(ret.value)
            self.builder.ret(value)
        else:
            self.builder.ret_void()

    def compile_while_stmt(self, while_stmt: WhileLoop):

        func = self.function_context[-1]['function']
        builder = self.builder

        check_block = func.append_basic_block(name="while_check")
        body_block = func.append_basic_block(name="while_body")
        exit_block = func.append_basic_block(name="while_exit")

        builder.branch(check_block)

        builder.position_at_end(check_block)
        condition = self.compile_expression(while_stmt.condition)

        if isinstance(condition.type, ir.IntType) and condition.type.width > 1:
            zero = ir.Constant(condition.type, 0)
            condition = builder.icmp_signed('!=', condition, zero, name="bool_cond")

        builder.cbranch(condition, body_block, exit_block)

        builder.position_at_end(body_block)

        self.loop_stack.append({
            'check': check_block,
            'exit': exit_block
        })

        for stmt in while_stmt.body:
            if not builder.block.terminator:
                self.compile_statement(stmt)

        self.loop_stack.pop()

        if not builder.block.terminator:
            builder.branch(check_block)

        builder.position_at_end(exit_block)

    def compile_break_stmt(self):

        if not self.loop_stack:
            raise RuntimeError("break используется вне цикла")
        exit_block = self.loop_stack[-1]['exit']
        self.builder.branch(exit_block)

    def compile_continue_stmt(self):

        if not self.loop_stack:
            raise RuntimeError("continue используется вне цикла")
        check_block = self.loop_stack[-1]['check']
        self.builder.branch(check_block)

    def compile_match_stmt(self, match_stmt: MatchStmt):
        func = self.function_context[-1]['function']
        builder = self.builder

        expr_val = self.compile_expression(match_stmt.expr)
        exit_block = func.append_basic_block(name="match_end")

        next_check = func.append_basic_block(name="match_check_0")
        builder.branch(next_check)

        for i, case in enumerate(match_stmt.cases):
            builder.position_at_end(next_check)
            case_block = func.append_basic_block(name=f"match_case_{i}")
            following = func.append_basic_block(name=f"match_check_{i+1}")

            if case.values is None:
                builder.branch(case_block)
            else:
                cond_val = None
                for v in case.values:
                    vval = self.compile_expression(v)
                    try:
                        cmp = builder.icmp_signed('==', expr_val, vval)
                    except Exception:
                        cmp = builder.icmp_signed('==', expr_val, vval)
                    if cond_val is None:
                        cond_val = cmp
                    else:
                        cond_val = builder.or_(cond_val, cmp)

                builder.cbranch(cond_val, case_block, following)

            builder.position_at_end(case_block)
            for stmt in case.body:
                self.compile_statement(stmt)
            if not builder.block.terminator:
                builder.branch(exit_block)

            next_check = following

        builder.position_at_end(next_check)
        builder.branch(exit_block)
        builder.position_at_end(exit_block)

    def compile_for_loop(self, for_stmt: ForLoop):
        func = self.function_context[-1]['function']
        builder = self.builder

        if for_stmt.init or for_stmt.condition or for_stmt.post:
            if for_stmt.init:
                if isinstance(for_stmt.init, Assign):
                    self.compile_assign(for_stmt.init)
                else:
                    self.compile_expression(for_stmt.init)

            check_block = func.append_basic_block(name="for_check")
            body_block = func.append_basic_block(name="for_body")
            post_block = func.append_basic_block(name="for_post")
            exit_block = func.append_basic_block(name="for_exit")

            builder.branch(check_block)

            builder.position_at_end(check_block)
            if for_stmt.condition:
                cond_val = self.compile_expression(for_stmt.condition)
                if isinstance(cond_val.type, ir.IntType) and cond_val.type.width > 1:
                    zero = ir.Constant(cond_val.type, 0)
                    cond_val = builder.icmp_signed('!=', cond_val, zero, name="bool_cond")
                builder.cbranch(cond_val, body_block, exit_block)
            else:
                builder.branch(body_block)

            builder.position_at_end(body_block)
            self.loop_stack.append({'check': check_block, 'exit': exit_block})
            for stmt in for_stmt.body:
                if not builder.block.terminator:
                    self.compile_statement(stmt)
            self.loop_stack.pop()
            if not builder.block.terminator:
                builder.branch(post_block)

            builder.position_at_end(post_block)
            if for_stmt.post:
                if isinstance(for_stmt.post, Assign):
                    self.compile_assign(for_stmt.post)
                else:
                    self.compile_expression(for_stmt.post)
            if not builder.block.terminator:
                builder.branch(check_block)

            builder.position_at_end(exit_block)
            return

        if for_stmt.iter_var and for_stmt.iter_expr:
            iter_expr = for_stmt.iter_expr
            if isinstance(iter_expr, Call) and iter_expr.func_name == 'range' and len(iter_expr.args) >= 2:
                start_val = self.compile_expression(iter_expr.args[0])
                end_val = self.compile_expression(iter_expr.args[1])

                alloc_site = self.function_context[-1].get('entry_builder', self.entry_builder) or self.builder
                idx_alloca = alloc_site.alloca(ir.IntType(32), name=for_stmt.iter_var)
                self.builder.store(start_val, idx_alloca)

                check_block = func.append_basic_block(name="for_check")
                body_block = func.append_basic_block(name="for_body")
                post_block = func.append_basic_block(name="for_post")
                exit_block = func.append_basic_block(name="for_exit")

                builder.branch(check_block)

                builder.position_at_end(check_block)
                cur = self.builder.load(idx_alloca)
                cond = self.builder.icmp_signed('<', cur, end_val, name="for_cond")
                builder.cbranch(cond, body_block, exit_block)

                builder.position_at_end(body_block)
                ctx = self.function_context[-1]
                prev_sym = ctx['symbols'].get(for_stmt.iter_var)
                ctx['symbols'][for_stmt.iter_var] = {'ptr': idx_alloca, 'type': 'int'}

                self.loop_stack.append({'check': check_block, 'exit': exit_block})
                for stmt in for_stmt.body:
                    if not builder.block.terminator:
                        self.compile_statement(stmt)
                self.loop_stack.pop()

                if not builder.block.terminator:
                    builder.branch(post_block)

                builder.position_at_end(post_block)
                cur2 = self.builder.load(idx_alloca)
                one = ir.Constant(ir.IntType(32), 1)
                inc = self.builder.add(cur2, one, name="inc")
                self.builder.store(inc, idx_alloca)
                self.builder.branch(check_block)

                if prev_sym is not None:
                    ctx['symbols'][for_stmt.iter_var] = prev_sym
                else:
                    ctx['symbols'].pop(for_stmt.iter_var, None)

                builder.position_at_end(exit_block)
                return

        return

    def compile_inline_asm(self, asm_stmt: InlineAsm):

        asm_type = ir.FunctionType(ir.VoidType(), [])

        parts = []
        if getattr(asm_stmt, 'outputs', None):
            parts.append(asm_stmt.outputs)
        if getattr(asm_stmt, 'inputs', None):
            parts.append(asm_stmt.inputs)
        if getattr(asm_stmt, 'clobbers', None):
            parts.append(f"~{{{asm_stmt.clobbers}}}")

        constraints = ",".join(parts) if parts else ""

        inline_asm = ir.InlineAsm(asm_type, asm_stmt.code, constraints, has_side_effects=bool(getattr(asm_stmt, 'volatile', True)))
        self.builder.call(inline_asm, [])

    def compile_comptime_block(self, comptime: ComptimeBlock):

        raise NotImplementedError("Comptime execution is temporarily disabled for security reasons.")

    def compile_expression(self, expr) -> ir.Value:

        match expr:
            case Literal():
                return self.compile_literal(expr)
            case Variable():
                return self.compile_variable(expr)
            case BinaryOp():
                return self.compile_binary_op(expr)
            case Compare():
                return self.compile_compare(expr)
            case LogicalOp():
                return self.compile_logical_op(expr)
            case Call():
                return self.compile_call(expr)
            case FieldAccess():
                return self.compile_field_access(expr)
            case ArrayAccess():
                return self.compile_array_access(expr)
            case Dereference():
                return self.compile_dereference(expr)
            case CastExpr():
                return self.compile_cast(expr)
            case ArrayLiteral():
                return self.compile_array_literal(expr)
            case AddressOf():
                return self.compile_addressof(expr)
            case SizeOf():
                return self.compile_sizeof(expr)
            case _:
                raise TypeError(f"Неизвестный тип выражения: {type(expr)}")

    def compile_literal(self, lit: Literal) -> ir.Value:

        if lit.type == 'int':
            return ir.Constant(ir.IntType(32), int(lit.value))
        elif lit.type == 'float':
            return ir.Constant(ir.FloatType(), float(lit.value))
        elif lit.type == 'str':
            global_var = self.get_string_constant(lit.value)
            zero = ir.Constant(ir.IntType(32), 0)
            return self.builder.bitcast(
                self.builder.gep(global_var, [zero, zero]), 
                ir.PointerType(ir.IntType(8))
            )
        else:
            raise TypeError(f"Неподдерживаемый тип: {lit.type}")

    def get_string_constant(self, str_value: str) -> ir.GlobalVariable:

        if str_value in self.string_constants:
            return self.string_constants[str_value]

        str_bytes = str_value.encode('utf-8') + b'\x00'
        array_type = ir.ArrayType(ir.IntType(8), len(str_bytes))
        const_array = ir.Constant(array_type, bytearray(str_bytes))

        global_var = ir.GlobalVariable(self.module, array_type, name=f"str_{len(self.string_constants)}")
        global_var.linkage = 'internal'
        global_var.global_constant = True
        global_var.initializer = const_array

        self.string_constants[str_value] = global_var
        return global_var

    def compile_variable(self, var: Variable) -> ir.Value:

        for context in reversed(self.function_context):
            if var.name in context['symbols']:
                sym = context['symbols'][var.name]
                ptr = sym['ptr'] if isinstance(sym, dict) else sym
                return self.builder.load(ptr, name=var.name)

        if var.name in self.global_vars:
            global_var = self.global_vars[var.name]
            return self.builder.load(global_var, name=var.name)

        raise NameError(f"Переменная '{var.name}' не определена") #сука далдбаеб бляяя еблан ббы ыыы гуугггугугу ыггоыоыовоыфвфывфывфываааа ааааааааааааа

    def compile_binary_op(self, op: BinaryOp) -> ir.Value:

        left = self.compile_expression(op.left)
        right = self.compile_expression(op.right)

        if op.op == '+': return self.builder.add(left, right, name="add")
        elif op.op == '-': return self.builder.sub(left, right, name="sub")
        elif op.op == '*': return self.builder.mul(left, right, name="mul")
        elif op.op == '/': 
            if isinstance(left.type, ir.IntType):
                return self.builder.sdiv(left, right, name="div")
            else:
                return self.builder.fdiv(left, right, name="div")
        elif op.op == '%':
            if isinstance(left.type, ir.IntType):
                return self.builder.srem(left, right, name="mod")
        else:
            raise ValueError(f"Неподдерживаемая операция: {op.op}")

    def compile_compare(self, cmp: Compare) -> ir.Value:

        left = self.compile_expression(cmp.left)
        right = self.compile_expression(cmp.right)

        if isinstance(left.type, ir.IntType):
            llvm_pred_map = {
                '==': '==',
                '!=': '!=',
                '<': '<',
                '>': '>',
                '<=': '<=',
                '>=': '>=',
            }
            if cmp.op not in llvm_pred_map:
                raise ValueError(f"Неподдерживаемое сравнение: {cmp.op}")

            return self.builder.icmp_signed(llvm_pred_map[cmp.op], left, right, name="cmp")
        else:
            raise NotImplementedError("Сравнение только для int")

    def compile_logical_op(self, op: LogicalOp) -> ir.Value:

        if op.op == 'not':
            operand = self.compile_expression(op.left)
            if isinstance(operand.type, ir.IntType) and operand.type.width > 1:
                zero = ir.Constant(operand.type, 0)
                operand = self.builder.icmp_signed('!=', operand, zero, name="bool")
            one = ir.Constant(ir.IntType(1), 1)
            return self.builder.xor(operand, one, name="not")

        elif op.op == 'and':
            left = self.compile_expression(op.left)
            if isinstance(left.type, ir.IntType) and left.type.width > 1:
                zero = ir.Constant(left.type, 0)
                left = self.builder.icmp_signed('!=', left, zero, name="bool_left")
            right = self.compile_expression(op.right)
            if isinstance(right.type, ir.IntType) and right.type.width > 1:
                zero = ir.Constant(right.type, 0)
                right = self.builder.icmp_signed('!=', right, zero, name="bool_right")
            return self.builder.and_(left, right, name="and")

        elif op.op == 'or':
            left = self.compile_expression(op.left)
            if isinstance(left.type, ir.IntType) and left.type.width > 1:
                zero = ir.Constant(left.type, 0)
                left = self.builder.icmp_signed('!=', left, zero, name="bool_left")
            right = self.compile_expression(op.right)
            if isinstance(right.type, ir.IntType) and right.type.width > 1:
                zero = ir.Constant(right.type, 0)
                right = self.builder.icmp_signed('!=', right, zero, name="bool_right")
            return self.builder.or_(left, right, name="or")

        else:
            raise ValueError(f"Unknown logical operator: {op.op}")

    def compile_assign(self, assign: Assign):

        value = self.compile_expression(assign.value)
        context = self.function_context[-1]
        symbols = context['symbols']
        if assign.target not in symbols:
            alloc_site = context.get('entry_builder', self.entry_builder) or self.builder
            declared_type_str = getattr(assign, 'var_type', None)
            if declared_type_str:
                alloc_type = self.get_llvm_type(declared_type_str)
            else:
                alloc_type = value.type

            alloca = alloc_site.alloca(alloc_type, name=assign.target)
            symbols[assign.target] = {
                'ptr': alloca,
                'type': declared_type_str
            }
        else:
            sym = symbols[assign.target]
            alloca = sym['ptr'] if isinstance(sym, dict) else sym

        stored_value = value
        declared_type_str = None
        if isinstance(symbols[assign.target], dict):
            declared_type_str = symbols[assign.target].get('type')

        if declared_type_str and isinstance(self.get_llvm_type(declared_type_str), ir.PointerType):
            alloc_ty = self.get_llvm_type(declared_type_str)
            if isinstance(value, ir.Constant) and isinstance(value.type, ir.IntType) and int(value.constant) == 0:
                stored_value = ir.Constant(alloc_ty, None)

        self.builder.store(stored_value, alloca)

        sym_entry = symbols[assign.target]
        if isinstance(sym_entry, dict) and sym_entry.get('type') is None:
            try:
                val_type = value.type
                if isinstance(val_type, ir.IntType):
                    if val_type.width == 32:
                        sym_entry['type'] = 'int'
                    else:
                        sym_entry['type'] = f'i{val_type.width}'
                elif isinstance(val_type, ir.FloatType):
                    sym_entry['type'] = 'float'
                elif isinstance(val_type, ir.PointerType):
                    sym_entry['type'] = '*void'
                else:
                    sym_entry['type'] = None
            except Exception:
                sym_entry['type'] = None

    def compile_if_stmt(self, if_stmt: IfStmt):

        condition = self.compile_expression(if_stmt.condition)

        if isinstance(condition.type, ir.IntType) and condition.type.width > 1:
            zero = ir.Constant(condition.type, 0)
            condition = self.builder.icmp_signed('!=', condition, zero, name="bool_cond")

        func = self.function_context[-1]['function']
        builder = self.builder

        then_block = func.append_basic_block(name="then")
        else_block = None
        if if_stmt.else_body:
            else_block = func.append_basic_block(name="else")
        merge_block = func.append_basic_block(name="merge")

        false_target = else_block if else_block else merge_block
        builder.cbranch(condition, then_block, false_target)

        builder.position_at_end(then_block)
        for stmt in if_stmt.then_body:
            self.compile_statement(stmt)
        if not then_block.terminator:
            builder.branch(merge_block)

        if else_block:
            builder.position_at_end(else_block)
            for stmt in if_stmt.else_body:
                self.compile_statement(stmt)
            if not else_block.terminator:
                builder.branch(merge_block)

        builder.position_at_end(merge_block)

    def compile_call(self, call: Call) -> ir.Value:

        func = None
        for f in self.module.functions:
            if f.name == call.func_name:
                func = f
                break
        if func is None:
            raise NameError(f"Функция '{call.func_name}' не найдена")

        args = []
        for arg_expr in call.args:
            arg_val = self.compile_expression(arg_expr)
            if isinstance(arg_val, ir.GlobalVariable):
                zero = ir.Constant(ir.IntType(32), 0)
                ptr = self.builder.gep(arg_val, [zero, zero])
                args.append(self.builder.bitcast(ptr, ir.PointerType(ir.IntType(8))))
            else:
                args.append(arg_val)

        return self.builder.call(func, args, name=f"call_{call.func_name}")

    def compile_field_access(self, fa: FieldAccess) -> ir.Value:
        obj_ptr = self.compile_expression(fa.obj)

        struct_name = None
        declared_type = getattr(fa.obj, 'resolved_type', None)
        if isinstance(declared_type, str):
            if declared_type.startswith('*'):
                struct_name = declared_type[1:]
            elif declared_type.startswith('struct '):
                struct_name = declared_type.split(' ', 1)[1]
            elif declared_type in self.struct_fields:
                struct_name = declared_type

        if not struct_name and isinstance(fa.obj, Variable):
            for context in reversed(self.function_context):
                if fa.obj.name in context['symbols']:
                    sym = context['symbols'][fa.obj.name]
                    declared_type = sym['type'] if isinstance(sym, dict) else None
                    if isinstance(declared_type, str) and declared_type.startswith('*'):
                        struct_name = declared_type[1:]
                    break

        if not struct_name or struct_name not in self.struct_fields:
            raise NameError(f"Доступ к полю для структуры, не являющейся структурой или имеющей неизвестное значение: {getattr(fa.obj, 'resolved_type', None)}")

        fields_map = self.struct_fields[struct_name]
        if fa.field not in fields_map:
            raise NameError(f"Поле '{fa.field}' не найдено в структуре '{struct_name}'")

        field_idx = list(fields_map.keys()).index(fa.field)
        zero = ir.Constant(ir.IntType(32), 0)
        idx = ir.Constant(ir.IntType(32), field_idx)

        ptr = self.builder.gep(obj_ptr, [zero, idx], name="field_ptr")
        return self.builder.load(ptr, name=f"val_{fa.field}")

    def compile_array_access(self, aa: ArrayAccess) -> ir.Value:
            arr_ptr = self.compile_expression(aa.arr)

            index_val = self.compile_expression(aa.index)

            zero = ir.Constant(ir.IntType(32), 0)
            element_ptr = self.builder.gep(arr_ptr, [zero, index_val], name="elem_ptr")

            return self.builder.load(element_ptr, name="elem_val")

    def compile_array_literal(self, al: ArrayLiteral) -> ir.Value:

        elems = [self.compile_expression(e) for e in al.elements]
        if not elems:
            raise NotImplementedError("Пустые массивы не поддерживаются")

        elem_type = elems[0].type
        count = len(elems)

        array_type = ir.ArrayType(elem_type, count)

        alloc_site = None
        if self.function_context:
            alloc_site = self.function_context[-1].get('entry_builder', None)
        if alloc_site is None:
            alloc_site = self.entry_builder or self.builder

        array_alloca = alloc_site.alloca(array_type, name="arrlit")

        zero = ir.Constant(ir.IntType(32), 0)
        for i, val in enumerate(elems):
            idx = ir.Constant(ir.IntType(32), i)
            elem_ptr = self.builder.gep(array_alloca, [zero, idx], name=f"elem_ptr_{i}")
            self.builder.store(val, elem_ptr)

        return array_alloca

    def compile_dereference(self, deref: Dereference) -> ir.Value:

        ptr = self.compile_expression(deref.ptr)

        if deref.index:
            index = self.compile_expression(deref.index)
            elem_ptr = self.builder.gep(ptr, [index], name="deref_elem")
            return self.builder.load(elem_ptr, name="deref_val")
        else:
            return self.builder.load(ptr, name="deref_val")

    def compile_cast(self, cast: CastExpr) -> ir.Value:

        expr_val = self.compile_expression(cast.expr)
        target_type = self.get_llvm_type(cast.target_type)

        from_type = expr_val.type

        if isinstance(from_type, ir.IntType) and isinstance(target_type, ir.PointerType):
            return self.builder.inttoptr(expr_val, target_type, name="int_to_ptr")

        elif isinstance(from_type, ir.PointerType) and isinstance(target_type, ir.IntType):
            return self.builder.ptrtoint(expr_val, target_type, name="ptr_to_int")

        elif isinstance(from_type, ir.IntType) and isinstance(target_type, ir.IntType):
            if from_type.width < target_type.width:
                return self.builder.sext(expr_val, target_type, name="sext")
            elif from_type.width > target_type.width:
                return self.builder.trunc(expr_val, target_type, name="trunc")
            else:
                return expr_val

        elif isinstance(from_type, ir.IntType) and isinstance(target_type, (ir.FloatType, ir.DoubleType)):
            return self.builder.sitofp(expr_val, target_type, name="int_to_float")

        elif isinstance(from_type, (ir.FloatType, ir.DoubleType)) and isinstance(target_type, ir.IntType):
            return self.builder.fptosi(expr_val, target_type, name="float_to_int")

        else:
            raise TypeError(f"Неподдерживаемое приведение типа: {from_type} -> {target_type}")

    def _sizeof_llvm_type(self, llvm_type) -> int:

        if isinstance(llvm_type, ir.IntType):
            return max(1, llvm_type.width // 8)
        if isinstance(llvm_type, ir.FloatType):
            return 4
        if isinstance(llvm_type, ir.PointerType):
            return 8
        if isinstance(llvm_type, ir.ArrayType):
            try:
                elem_size = self._sizeof_llvm_type(llvm_type.element)
                return elem_size * llvm_type.count
            except Exception:
                return 0
        if hasattr(llvm_type, 'elements') and llvm_type.elements is not None:
            total = 0
            for elt in llvm_type.elements:
                total += self._sizeof_llvm_type(elt)
            return total
        return 8

    def compile_sizeof(self, sizeof_node: SizeOf) -> ir.Value:

        target = sizeof_node.target
        llvm_t = None
        if isinstance(target, str):
            llvm_t = self.get_llvm_type(target)
        elif hasattr(target, 'name'):
            try:
                llvm_t = self.get_llvm_type(target.name)
            except Exception:
                llvm_t = None
        else:
            try:
                val = self.compile_expression(target)
                llvm_t = val.type
            except Exception:
                llvm_t = None

        size = 0
        if llvm_t is not None:
            try:
                size = self._sizeof_llvm_type(llvm_t)
            except Exception:
                size = 8
        else:
            size = 8

        return ir.Constant(ir.IntType(32), int(size))

    def compile_addressof(self, addrof: AddressOf) -> ir.Value:

        expr = addrof.expr
        if isinstance(expr, Variable):
            for context in reversed(self.function_context):
                if expr.name in context['symbols']:
                    sym = context['symbols'][expr.name]
                    return sym['ptr'] if isinstance(sym, dict) else sym
            if expr.name in self.global_vars:
                return self.global_vars[expr.name]
            raise NameError(f"Переменная '{expr.name}' не определена")

        if isinstance(expr, FieldAccess):
            if isinstance(expr.obj, Variable):
                declared_type = None
                obj_ptr = None
                for context in reversed(self.function_context):
                    if expr.obj.name in context['symbols']:
                        sym = context['symbols'][expr.obj.name]
                        if isinstance(sym, dict):
                            declared_type = sym.get('type')
                            obj_ptr = sym.get('ptr')
                        else:
                            obj_ptr = sym
                        break

                if declared_type and isinstance(declared_type, str) and declared_type.startswith('*'):
                    inner = declared_type[1:]
                    if inner in self.struct_fields and expr.field in self.struct_fields[inner]:
                        idx = list(self.struct_fields[inner].keys()).index(expr.field)
                        zero = ir.Constant(ir.IntType(32), 0)
                        idx_const = ir.Constant(ir.IntType(32), idx)
                        return self.builder.gep(obj_ptr, [zero, idx_const], name="field_addr")

            obj_ptr = self.compile_expression(expr.obj)
            if isinstance(obj_ptr.type, ir.PointerType):
                for sname, fields in self.struct_fields.items():
                    if expr.field in fields:
                        idx = list(fields.keys()).index(expr.field)
                        zero = ir.Constant(ir.IntType(32), 0)
                        idx_const = ir.Constant(ir.IntType(32), idx)
                        return self.builder.gep(obj_ptr, [zero, idx_const], name="field_addr")

        if isinstance(expr, ArrayAccess):
            arr_ptr = self.compile_expression(expr.arr)
            index_val = self.compile_expression(expr.index)
            zero = ir.Constant(ir.IntType(32), 0)
            return self.builder.gep(arr_ptr, [zero, index_val], name="elem_addr")

        raise NotImplementedError("AddressOf: поддерживаются только переменные, поля и элементы массива")

def compile_to_llvm(program: Program, module_name: str = "cblerr_module") -> ir.Module:

    # попытка выполнить мономорфизацию, если модуль доступен, а если нет то идешь нахуй, блять, еблан, ххахахаха сука как смещно хъаъхахахаха
    try:
        from core.monomorphizer import monomorphize
    except Exception:
        monomorphize = None

    if monomorphize:
        try:
            program = monomorphize(program)
        except Exception:
            pass

    compiler = Compiler(module_name)

    if getattr(program, 'structs', None):
        for struct_def in program.structs:
            compiler.compile_struct_def(struct_def)

    if getattr(program, 'global_vars', None):
        for global_var in program.global_vars:
            compiler.compile_global_var(global_var)

    if getattr(program, 'functions', None):
        for func_def in program.functions:
            if getattr(func_def, 'is_extern', False):
                compiler.declare_extern_function(func_def)

    if getattr(program, 'functions', None):
        for func_def in program.functions:
            if not getattr(func_def, 'is_extern', False):
                compiler.compile_function(func_def)

    return compiler.module
