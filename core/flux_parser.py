from typing import Any, List
from core.lexer import Token, TokenType
from core.flux_ast import (
    Program, FunctionDef, Return, BinaryOp, Variable, Literal,
    IfStmt, Assign, Compare, Call, WhileLoop, BreakStmt, ContinueStmt,
    StructDef, FieldAccess, ArrayAccess, ArrayLiteral, LogicalOp,
    PointerType, Dereference, InlineAsm, CastExpr, Decorator, ComptimeBlock,
    MatchStmt, Case, ForLoop, EnumDef, AddressOf, SizeOf, GenericType
    , ImportStmt, FromImportStmt
)

from core.debugger import get_debugger

class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.pos = 0
        self.debugger = get_debugger()

    def current_token(self) -> Token | None:
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def peek_token(self, offset: int = 1) -> Token | None:
        idx = self.pos + offset
        return self.tokens[idx] if idx < len(self.tokens) else None

    def advance(self) -> Token | None:
        if self.pos < len(self.tokens):
            t = self.tokens[self.pos]
            self.pos += 1
            return t
        return None

    def expect(self, token_type: TokenType, error_msg: str = None, strict: bool = True):
        token = self.current_token()
        if not token or token.type != token_type:
            msg = error_msg or f"Expected {token_type}, got {token.type if token else 'EOF'}"
            if strict:
                raise SyntaxError(f"{msg} at line {token.line if token else '?'}")
            else:
                self.debugger.log_warning(msg)
                return token
        return self.advance()

    def skip_newlines(self) -> None:
        while self.current_token() and self.current_token().type == TokenType.NEWLINE:
            self.advance()

    def _scan_postfix_for_assignment(self, start_pos: int):

        tokens = self.tokens
        pos = start_pos
        if pos >= len(tokens) or tokens[pos].type != TokenType.NAME:
            return ('expr', pos)
        pos += 1

        while pos < len(tokens):
            t = tokens[pos]
            if t.type == TokenType.LT:
                depth = 1
                pos += 1
                while pos < len(tokens) and depth > 0:
                    tt = tokens[pos]
                    if tt.type == TokenType.LT:
                        depth += 1
                    elif tt.type == TokenType.GT:
                        depth -= 1
                        if depth == 0:
                            pos += 1
                            break
                    elif tt.type in (TokenType.NEWLINE, TokenType.EOF):
                        return ('expr', pos)
                    pos += 1
                continue

            if t.type == TokenType.LPAREN:
                return ('call', pos)

            if t.type == TokenType.DOT:
                pos += 1
                if pos < len(tokens) and tokens[pos].type == TokenType.NAME:
                    pos += 1
                    continue
                return ('expr', pos)

            if t.type == TokenType.LBRACKET:
                depth = 1
                pos += 1
                while pos < len(tokens) and depth > 0:
                    if tokens[pos].type == TokenType.LBRACKET:
                        depth += 1
                    elif tokens[pos].type == TokenType.RBRACKET:
                        depth -= 1
                    elif tokens[pos].type in (TokenType.NEWLINE, TokenType.EOF):
                        return ('expr', pos)
                    pos += 1
                continue

            break

        if pos < len(tokens) and tokens[pos].type in (
            TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN
        ):
            return ('assign', pos)

        return ('expr', pos)

    def parse_type(self) -> Any:
        token = self.current_token()

        if token and token.type == TokenType.MULTIPLY:
            star_count = 0
            while self.current_token() and self.current_token().type == TokenType.MULTIPLY:
                star_count += 1
                self.advance()
            base = self.parse_type()
            result = base
            for _ in range(star_count):
                result = f"*{result}"
            return result

        type_tokens = {
            TokenType.NAME, TokenType.INT, TokenType.STR, TokenType.BOOL, TokenType.FLOAT, TokenType.VOID,
            TokenType.U8, TokenType.U16, TokenType.U32, TokenType.U64,
            TokenType.I8, TokenType.I16, TokenType.I32, TokenType.I64,
        }

        if token and token.type in type_tokens:
            name = token.value if getattr(token, 'value', None) is not None else token.type.name.lower()
            self.advance()

            if self.current_token() and self.current_token().type == TokenType.LT:
                self.advance()
                args: list[Any] = []
                while True:
                    args.append(self.parse_type())
                    if self.current_token() and self.current_token().type == TokenType.COMMA:
                        self.advance()
                        continue
                    break
                if self.current_token() and self.current_token().type == TokenType.GT:
                    self.advance()
                else:
                    self.debugger.log_warning(f"Expected '>' after generic types for {name}")
                return GenericType(name, args)

            return name

        if token:
            raise SyntaxError(f"Unknown token type: {token.type} (value={token.value}) at line {token.line}")
            self.advance()
            return token.value

        raise SyntaxError("Expected type but reached end of file (EOF)")

    def parse_import(self):
        self.expect(TokenType.IMPORT, "Expected 'import'")
        token = self.current_token()
        if not token:
            raise SyntaxError("Expected module name after import, but reached EOF")
        if token.type == TokenType.STRING:
            module_name = token.value
            self.advance()
        elif token.type == TokenType.NAME:
            module_name = token.value
            self.advance()
        else:
            self.debugger.log_warning("Expected name or string after import")
            module_name = token.value if getattr(token, 'value', None) is not None else ''
            self.advance()
        from core.flux_ast import ImportStmt
        return ImportStmt(module_name, None)

    def parse_from_import(self):
        self.expect(TokenType.FROM, "Expected 'from'")
        token = self.current_token()
        if not token:
            raise SyntaxError("Expected module name after from, but reached EOF")
        if token.type == TokenType.STRING:
            module = token.value
            self.advance()
        elif token.type == TokenType.NAME:
            module = token.value
            self.advance()
        else:
            raise SyntaxError("Expected name or string after from")
        self.expect(TokenType.IMPORT, "Expected 'import' after module name")
        items = []
        while True:
            t = self.expect(TokenType.NAME, "Expected import name")
            items.append(t.value if t else None)
            if self.current_token() and self.current_token().type == TokenType.COMMA:
                self.advance()
                continue
            break
        from core.flux_ast import FromImportStmt
        return FromImportStmt(module, items, None)

    def parse_global_var(self):
        is_const = False
        if self.current_token() and self.current_token().type == TokenType.CONST:
            is_const = True
            self.advance()
        name = self.expect(TokenType.NAME, "Expected global variable name").value
        self.expect(TokenType.COLON, "Expected ':' after global variable name")
        var_type = self.parse_type()
        value = None
        if self.current_token() and self.current_token().type == TokenType.ASSIGN:
            self.advance()
            value = self.parse_expression()
        self.skip_newlines()
        from core.flux_ast import GlobalVariable
        return GlobalVariable(name, var_type, value, is_const)

    def parse(self) -> Program:
        functions: list[FunctionDef] = []
        structs: list[StructDef] = []
        imports: list[Any] = []
        global_vars: list[Any] = []

        self.skip_newlines()

        while self.current_token() and self.current_token().type in (TokenType.IMPORT, TokenType.FROM):
            if self.current_token().type == TokenType.IMPORT:
                imports.append(self.parse_import())
            else:
                imports.append(self.parse_from_import())
            self.skip_newlines()

        while self.current_token() and self.current_token().type != TokenType.EOF:
            if self.current_token().type == TokenType.NEWLINE:
                self.skip_newlines()
                continue

            if self.current_token().type == TokenType.COMPTIME:
                _ = self.parse_comptime()
                continue

            if self.current_token().type == TokenType.CONST:
                gv = self.parse_global_var()
                global_vars.append(gv)
                continue

            decorators = None
            if self.current_token() and self.current_token().type == TokenType.AT:
                decorators = self.parse_decorators()

            if self.current_token() and self.current_token().type == TokenType.STRUCT:
                structs.append(self.parse_struct_def(decorators))
                continue

            if self.current_token() and self.current_token().type == TokenType.ENUM:
                enums = self.parse_enum_def()
                structs.append(enums) if enums else None
                continue

            if self.current_token() and self.current_token().type in (TokenType.DEF, TokenType.EXTERN):
                functions.append(self.parse_function(decorators))
                continue

            stmt = self.parse_statement()
            if stmt:
                from core.flux_ast import GlobalVariable
                if isinstance(stmt, Assign):
                    global_vars.append(GlobalVariable(stmt.target, getattr(stmt, 'var_type', None), stmt.value, False))
            else:
                self.advance()

        return Program(functions=functions, structs=structs, imports=imports, global_vars=global_vars)

    def parse_function(self, decorators: list[Decorator] | None = None) -> FunctionDef:
        is_extern = False
        if self.current_token() and self.current_token().type == TokenType.EXTERN:
            is_extern = True
            self.advance()
        self.expect(TokenType.DEF, "Expected 'def'")
        name = self.expect(TokenType.NAME, "Expected function name").value

        self.expect(TokenType.LPAREN, "Expected '('")
        params: list[tuple[str, Any]] = []
        is_vararg = False
        if self.current_token() and self.current_token().type != TokenType.RPAREN:
            while True:
                if self.current_token().type == TokenType.ELLIPSIS:
                    is_vararg = True
                    self.advance()
                    break
                pname = self.expect(TokenType.NAME, "Expected parameter name").value
                self.expect(TokenType.COLON, "Expected ':' after parameter name")
                ptype = self.parse_type()
                params.append((pname, ptype))
                if self.current_token() and self.current_token().type == TokenType.COMMA:
                    self.advance()
                    continue
                break
        self.expect(TokenType.RPAREN, "Expected ')'")

        return_type = None
        if self.current_token() and self.current_token().type == TokenType.ARROW:
            self.advance()
            return_type = self.parse_type()

        if is_extern:
            self.skip_newlines()
            fd = FunctionDef(name, params, return_type, [], is_extern=True, decorators=decorators, is_vararg=is_vararg)
            return fd

        self.expect(TokenType.COLON, "Expected ':' after function signature")
        self.skip_newlines()
        body: list[Any] = []
        if self.current_token() and self.current_token().type == TokenType.INDENT:
            self.advance()
            while self.current_token() and self.current_token().type != TokenType.DEDENT:
                self.skip_newlines()
                if not self.current_token() or self.current_token().type == TokenType.DEDENT:
                    break
                stmt = self.parse_statement()
                if stmt:
                    body.append(stmt)
            if self.current_token() and self.current_token().type == TokenType.DEDENT:
                self.advance()
        else:
            stmt = self.parse_statement()
            if stmt:
                body.append(stmt)

        return FunctionDef(name, params, return_type, body, is_extern=False, decorators=decorators, is_vararg=is_vararg)

    def parse_struct_def(self, decorators: list[Decorator] | None = None) -> StructDef:
        self.expect(TokenType.STRUCT, "Expected 'struct'")
        name = self.expect(TokenType.NAME, "Expected struct name").value
        self.expect(TokenType.COLON, "Expected ':' after struct name")
        self.skip_newlines()
        if self.current_token() and self.current_token().type == TokenType.INDENT:
            self.advance()
        else:
            raise SyntaxError("Expected indent after struct ':'")
        fields: list[tuple[str, Any]] = []
        while self.current_token() and self.current_token().type != TokenType.DEDENT:
            self.skip_newlines()
            if not self.current_token() or self.current_token().type == TokenType.DEDENT:
                break
            fname = self.expect(TokenType.NAME, "Expected field name").value
            self.expect(TokenType.COLON, "Expected ':' after field name")
            ftype = self.parse_type()
            fields.append((fname, ftype))
        if self.current_token() and self.current_token().type == TokenType.DEDENT:
            self.advance()
        s = StructDef(name, fields, decorators=decorators)
        return s

    def parse_enum_def(self) -> EnumDef:
        self.expect(TokenType.ENUM, "Expected 'enum'")
        name = self.expect(TokenType.NAME, "Expected enum name").value
        self.expect(TokenType.COLON, "Expected ':' after enum name")
        self.skip_newlines()
        if self.current_token() and self.current_token().type == TokenType.INDENT:
            self.advance()
        else:
            raise SyntaxError("Expected indent after enum ':'")
        members: list[tuple[str, Any | None]] = []
        while self.current_token() and self.current_token().type != TokenType.DEDENT:
            self.skip_newlines()
            if not self.current_token() or self.current_token().type == TokenType.DEDENT:
                break
            mname = self.expect(TokenType.NAME, "Expected enum member name").value
            mval = None
            if self.current_token() and self.current_token().type == TokenType.ASSIGN:
                self.advance()
                mval = self.parse_expression()
            members.append((mname, mval))
        if self.current_token() and self.current_token().type == TokenType.DEDENT:
            self.advance()
        return EnumDef(name, members)

    def parse_comptime(self) -> ComptimeBlock:
        self.expect(TokenType.COMPTIME, "Expected 'comptime'")
        self.skip_newlines()
        code_parts: list[str] = []
        if self.current_token() and self.current_token().type == TokenType.INDENT:
            self.advance()
            while self.current_token() and self.current_token().type != TokenType.DEDENT:
                t = self.current_token()
                if t.type == TokenType.NEWLINE:
                    code_parts.append('\n')
                else:
                    code_parts.append(t.value or '')
                self.advance()
            if self.current_token() and self.current_token().type == TokenType.DEDENT:
                self.advance()
        self.debugger.log_warning("Encountered @comptime block; execution will be sandboxed (unreliable)")
        return ComptimeBlock(''.join(code_parts))

    def parse_statement(self):
        token = self.current_token()
        if not token:
            return None

        if token.type == TokenType.ASM:
            self.advance()
            self.expect(TokenType.LPAREN, "Expected '(' after asm")
            s = self.expect(TokenType.STRING, "Expected asm string").value
            self.expect(TokenType.RPAREN, "Expected ')'")
            return InlineAsm(s)

        if token.type == TokenType.RETURN:
            return self.parse_return()
        if token.type == TokenType.ENDOFCODE:
            self.advance()
            return Return(Literal(0, 'int'), is_endofcode=True)
        if token.type == TokenType.IF:
            return self.parse_if_stmt()
        if token.type == TokenType.WHILE:
            return self.parse_while_stmt()
        if token.type == TokenType.FOR:
            return self.parse_for_stmt()
        if token.type == TokenType.MATCH:
            return self.parse_match_stmt()
        if token.type == TokenType.BREAK:
            self.advance()
            return BreakStmt()
        if token.type == TokenType.CONTINUE:
            self.advance()
            return ContinueStmt()

        if token.type == TokenType.LET:
            self.advance()
            name = self.expect(TokenType.NAME, "Expected variable name after let").value
            self.expect(TokenType.ASSIGN, "Expected '=' in let")
            val = self.parse_expression()
            return Assign(name, val)

        if token.type == TokenType.NAME:
            if self.peek_token() and self.peek_token().type == TokenType.COLON:
                return self.parse_var_decl()
            kind, _ = self._scan_postfix_for_assignment(self.pos)
            if kind == 'assign':
                left = self.parse_atom_or_access_simple()
                if not self.current_token():
                    raise SyntaxError('Unexpected EOF after variable name')
                if self.current_token().type in (TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN):
                    op = self.current_token().type
                    self.advance()
                    val = self.parse_expression()
                    
                    if op == TokenType.PLUS_ASSIGN:
                        val = BinaryOp('+', left, val)
                    elif op == TokenType.MINUS_ASSIGN:
                        val = BinaryOp('-', left, val)
                        
                    if isinstance(left, Variable):
                        return Assign(left.name, val)
                    return Assign(left, val)
                return left

            return self.parse_expression()
        expr = self.parse_unary()
        if self.current_token() and self.current_token().type in (TokenType.ASSIGN, TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN):
            op = self.current_token().type
            self.advance()
            val = self.parse_expression()
            
            if op == TokenType.PLUS_ASSIGN:
                val = BinaryOp('+', expr, val)
            elif op == TokenType.MINUS_ASSIGN:
                val = BinaryOp('-', expr, val)
                
            if isinstance(expr, Variable):
                return Assign(expr.name, val)
            return Assign(expr, val)
        return expr

    def parse_return(self) -> Return:
        self.expect(TokenType.RETURN, "Expected 'return'")
        if self.current_token() and self.current_token().type not in (TokenType.NEWLINE, TokenType.DEDENT, TokenType.EOF):
            v = self.parse_expression()
        else:
            v = None
        return Return(v)

    def parse_var_decl(self) -> Assign:
        name = self.expect(TokenType.NAME, "Expected variable name").value
        self.expect(TokenType.COLON, "Expected ':'")
        t = self.parse_type()
        val = None
        if self.current_token() and self.current_token().type == TokenType.ASSIGN:
            self.advance()
            val = self.parse_expression()
            return Assign(name, val, t)
        return Assign(name, Literal(0, 'int'), t)

    def parse_atom_or_access_simple(self):
        token = self.current_token()
        if not token or token.type != TokenType.NAME:
            raise SyntaxError("Expected name")
        name = token.value
        self.advance()
        if self.current_token() and self.current_token().type == TokenType.LPAREN:
            expr = self.parse_call(name)
        else:
            expr = Variable(name)
        while self.current_token() and self.current_token().type in (TokenType.DOT, TokenType.LBRACKET):
            if self.current_token().type == TokenType.DOT:
                self.advance()
                field = self.expect(TokenType.NAME, "Expected field name").value
                expr = FieldAccess(expr, field)
            else:
                self.advance()
                idx = self.parse_expression()
                self.expect(TokenType.RBRACKET, "Expected ']'" )
                expr = ArrayAccess(expr, idx)
        if self.current_token() and self.current_token().type == TokenType.LPAREN:
            expr = self.parse_call(expr)
        return expr

    def parse_atom_or_access(self):
        return self.parse_atom_or_access_simple()

    def parse_if_stmt(self) -> IfStmt:
        self.expect(TokenType.IF, "Expected 'if'")
        cond = self.parse_expression()
        self.expect(TokenType.COLON, "Expected ':' after if condition")
        self.skip_newlines()
        self.expect(TokenType.INDENT, "Expected indent for if body")
        then: list[Any] = []
        while self.current_token() and self.current_token().type != TokenType.DEDENT:
            then.append(self.parse_statement())
            self.skip_newlines()
        self.expect(TokenType.DEDENT, "Expected dedent after if body")
        else_body = None
        if self.current_token() and self.current_token().type == TokenType.ELSE:
            self.advance()
            self.expect(TokenType.COLON, "Expected ':' after else")
            self.skip_newlines()
            self.expect(TokenType.INDENT, "Expected indent for else body")
            else_body = []
            while self.current_token() and self.current_token().type != TokenType.DEDENT:
                else_body.append(self.parse_statement())
                self.skip_newlines()
            self.expect(TokenType.DEDENT, "Expected dedent after else body")
        return IfStmt(cond, then, else_body)

    def parse_while_stmt(self) -> WhileLoop:
        self.expect(TokenType.WHILE, "Expected 'while'")
        cond = self.parse_expression()
        self.expect(TokenType.COLON, "Expected ':' after while")
        self.skip_newlines()
        self.expect(TokenType.INDENT, "Expected indent for while body")
        body: list[Any] = []
        while self.current_token() and self.current_token().type != TokenType.DEDENT:
            body.append(self.parse_statement())
            self.skip_newlines()
        self.expect(TokenType.DEDENT, "Expected dedent after while body")
        return WhileLoop(cond, body)

    def parse_for_stmt(self) -> ForLoop:
        self.expect(TokenType.FOR, "Expected 'for'")
        if self.current_token() and self.current_token().type == TokenType.LPAREN:
            self.advance()
            init = None
            if self.current_token() and self.current_token().type != TokenType.SEMICOLON:
                if self.current_token().type == TokenType.LET:
                    init = self.parse_statement()
                else:
                    saved = self.pos
                    try:
                        expr = self.parse_expression()
                        if self.current_token() and self.current_token().type == TokenType.ASSIGN:
                            self.advance()
                            val = self.parse_expression()
                            if hasattr(expr, 'name'):
                                init = Assign(expr.name, val)
                            else:
                                init = Assign(expr, val)
                        else:
                            init = expr
                    except Exception:
                        self.pos = saved
                        init = self.parse_statement()
            self.expect(TokenType.SEMICOLON, "Expected ';' in for header")
            cond = None
            if self.current_token() and self.current_token().type != TokenType.SEMICOLON:
                cond = self.parse_expression()
            self.expect(TokenType.SEMICOLON, "Expected second ';' in for header")
            post = None
            if self.current_token() and self.current_token().type != TokenType.RPAREN:
                saved = self.pos
                try:
                    expr = self.parse_expression()
                    if self.current_token() and self.current_token().type == TokenType.ASSIGN:
                        self.advance()
                        val = self.parse_expression()
                        if hasattr(expr, 'name'):
                            post = Assign(expr.name, val)
                        else:
                            post = Assign(expr, val)
                    else:
                        post = expr
                except Exception:
                    self.pos = saved
                    post = self.parse_statement()
            self.expect(TokenType.RPAREN, "Expected ')' after for header")
            self.expect(TokenType.COLON, "Expected ':' after for header")
            self.skip_newlines()
            self.expect(TokenType.INDENT, "Expected indent for for body")
            body: list[Any] = []
            while self.current_token() and self.current_token().type != TokenType.DEDENT:
                body.append(self.parse_statement())
                self.skip_newlines()
            self.expect(TokenType.DEDENT, "Expected dedent after for body")
            return ForLoop(None, None, init, cond, post, body)

        var_name = None
        if self.current_token() and self.current_token().type == TokenType.NAME:
            var_name = self.current_token().value
            self.advance()
        self.expect(TokenType.IN, "Expected 'in' in for statement")
        if self.current_token() and self.current_token().type == TokenType.NUMBER and self.peek_token() and self.peek_token().type == TokenType.RANGE:
            start_tok = self.advance()
            self.advance()
            end_tok = self.expect(TokenType.NUMBER, "Expected end number in range")
            iter_expr = Call('range', [Literal(int(start_tok.value), 'int'), Literal(int(end_tok.value), 'int')])
        else:
            iter_expr = self.parse_expression()
        self.expect(TokenType.COLON, "Expected ':' after for header")
        self.skip_newlines()
        self.expect(TokenType.INDENT, "Expected indent for for body")
        body: list[Any] = []
        while self.current_token() and self.current_token().type != TokenType.DEDENT:
            body.append(self.parse_statement())
            self.skip_newlines()
        self.expect(TokenType.DEDENT, "Expected dedent after for body")
        return ForLoop(var_name, iter_expr, None, None, None, body)

    def parse_match_stmt(self) -> MatchStmt:
        self.expect(TokenType.MATCH, "Expected 'match'")
        expr = self.parse_expression()
        self.expect(TokenType.COLON, "Expected ':' after match expression")
        self.skip_newlines()
        self.expect(TokenType.INDENT, "Expected indent for match body")
        cases: list[Case] = []
        while self.current_token() and self.current_token().type != TokenType.DEDENT:
            if self.current_token().type == TokenType.CASE:
                self.advance()
                vals: list[Any] = []
                while True:
                    vals.append(self.parse_expression())
                    if self.current_token() and self.current_token().type == TokenType.COMMA:
                        self.advance()
                        continue
                    break
                self.expect(TokenType.COLON, "Expected ':' after case values")
                self.skip_newlines()
                self.expect(TokenType.INDENT, "Expected indent for case body")
                body: list[Any] = []
                while self.current_token() and self.current_token().type != TokenType.DEDENT:
                    body.append(self.parse_statement())
                    self.skip_newlines()
                self.expect(TokenType.DEDENT, "Expected dedent after case body")
                cases.append(Case(vals, body))
            elif self.current_token().type == TokenType.DEFAULT:
                self.advance()
                self.expect(TokenType.COLON, "Expected ':' after default")
                self.skip_newlines()
                self.expect(TokenType.INDENT, "Expected indent for default body")
                body: list[Any] = []
                while self.current_token() and self.current_token().type != TokenType.DEDENT:
                    body.append(self.parse_statement())
                    self.skip_newlines()
                self.expect(TokenType.DEDENT, "Expected dedent after default body")
                cases.append(Case(None, body))
            else:
                raise SyntaxError(f"Unexpected token in match body: {self.current_token().type} at line {self.current_token().line}")
                self.advance()
        self.expect(TokenType.DEDENT, "Expected dedent after match body")
        return MatchStmt(expr, cases)

    def parse_expression(self):
        expr = self.parse_logical_or()
        
        if self.current_token() and self.current_token().type == TokenType.WALRUS:
            self.advance()
            val = self.parse_expression()
            from core.flux_ast import WalrusExpr
            return WalrusExpr(expr, val)
            
        return expr

    def parse_logical_or(self):
        left = self.parse_logical_and()
        while self.current_token() and self.current_token().type == TokenType.OR:
            self.advance()
            right = self.parse_logical_and()
            left = LogicalOp('or', left, right)
        return left

    def parse_logical_and(self):
        left = self.parse_logical_not()
        while self.current_token() and self.current_token().type == TokenType.AND:
            self.advance()
            right = self.parse_logical_not()
            left = LogicalOp('and', left, right)
        return left

    def parse_logical_not(self):
        if self.current_token() and self.current_token().type == TokenType.NOT:
            self.advance()
            expr = self.parse_logical_not()
            return LogicalOp('not', expr)
        return self.parse_comparison()

    def parse_comparison(self):
        left = self.parse_additive()
        comps = (TokenType.EQ, TokenType.NE, TokenType.LT, TokenType.GT, TokenType.LE, TokenType.GE)
        while self.current_token() and self.current_token().type in comps:
            t = self.advance()
            op_map = {
                TokenType.EQ: '==', TokenType.NE: '!=', TokenType.LT: '<', TokenType.GT: '>', TokenType.LE: '<=', TokenType.GE: '>='
            }
            right = self.parse_additive()
            left = Compare(op_map[t.type], left, right)
        return left

    def parse_additive(self):
        left = self.parse_multiplicative()
        while self.current_token() and self.current_token().type in (TokenType.PLUS, TokenType.MINUS):
            t = self.advance()
            right = self.parse_multiplicative()
            left = BinaryOp(t.value, left, right)
        return left

    def parse_multiplicative(self):
        left = self.parse_power()
        while self.current_token() and self.current_token().type in (TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO):
            t = self.advance()
            right = self.parse_power()
            left = BinaryOp(t.value, left, right)
        return left

    def parse_power(self):
        left = self.parse_unary()
        while self.current_token() and self.current_token().type == TokenType.POW:
            t = self.advance()
            right = self.parse_unary()
            left = BinaryOp(t.value, left, right)
        return left

    def parse_unary(self):
        token = self.current_token()
        if token and token.type == TokenType.MULTIPLY:
            self.advance()
            expr = self.parse_unary()
            return Dereference(expr)
        if token and token.type == TokenType.AMP:
            self.advance()
            var = self.parse_unary()
            return AddressOf(var)
        if token and token.type == TokenType.MINUS:
            self.advance()
            expr = self.parse_unary()
            return BinaryOp('-', Literal(0, 'int'), expr)
        if token and token.type == TokenType.SIZEOF:
            self.advance()
            if self.current_token() and self.current_token().type == TokenType.LPAREN:
                self.advance()
                t = self.parse_type()
                self.expect(TokenType.RPAREN, "Expected ')' after sizeof type")
                return SizeOf(t)
            else:
                self.debugger.log_warning("Expected '(' after sizeof")
        return self.parse_atom()

    def parse_atom(self):
        token = self.current_token()
        if not token:
            raise SyntaxError("Unexpected end of file in expression")

        if token.type == TokenType.STRING:
            self.advance()
            expr: Any = Literal(token.value, 'str')

        elif token.type == TokenType.NUMBER:
            self.advance()
            if '.' in token.value:
                expr = Literal(float(token.value), 'float')
            else:
                expr = Literal(int(token.value, 0), 'int')

        elif token.type == TokenType.NAME:
            name = token.value
            self.advance()

            type_args = None

            if self.current_token() and self.current_token().type == TokenType.LT:
                saved_pos = self.pos
                try:
                    self.advance()
                    args: list[Any] = []
                    while True:
                        args.append(self.parse_type())
                        if self.current_token() and self.current_token().type == TokenType.COMMA:
                            self.advance()
                            continue
                        break
                    if self.current_token() and self.current_token().type == TokenType.GT:
                        self.advance()
                        type_args = args
                    else:
                        self.pos = saved_pos
                except SyntaxError:
                    self.pos = saved_pos

            if self.current_token() and self.current_token().type == TokenType.LPAREN:
                expr = self.parse_call(name, type_args)
            else:
                expr = Variable(name)

        elif token.type == TokenType.LPAREN:
            self.advance()
            expr = self.parse_expression()
            self.expect(TokenType.RPAREN, "Expected ')'")

        elif token.type == TokenType.LBRACKET or token.type == TokenType.LBRACE:
            is_struct_init = (token.type == TokenType.LBRACE)
            end_tok = TokenType.RBRACKET if token.type == TokenType.LBRACKET else TokenType.RBRACE
            self.advance()
            elems: list[Any] = []
            if not (self.current_token() and self.current_token().type == end_tok):
                while True:
                    elems.append(self.parse_expression())
                    if self.current_token() and self.current_token().type == TokenType.COMMA:
                        self.advance(); continue
                    break
            if end_tok == TokenType.RBRACKET:
                self.expect(TokenType.RBRACKET, "Expected ']' in array literal")
            else:
                self.expect(TokenType.RBRACE, "Expected '}' in struct initializer")

            arr = ArrayLiteral(elems)
            if is_struct_init:
                setattr(arr, 'is_struct_init', True)
            expr = arr

        else:
            raise SyntaxError(f"Unexpected token {token.type} in atom")

        while self.current_token() and self.current_token().type in (TokenType.DOT, TokenType.LBRACKET, TokenType.AS):
            if self.current_token().type == TokenType.DOT:
                self.advance()
                fld = self.expect(TokenType.NAME, "Expected field name").value
                expr = FieldAccess(expr, fld)

            elif self.current_token().type == TokenType.LBRACKET:
                self.advance()
                idx = self.parse_expression()
                self.expect(TokenType.RBRACKET, "Expected ']' in index")
                expr = ArrayAccess(expr, idx)

            elif self.current_token().type == TokenType.AS:
                self.advance()
                tgt = self.parse_type()
                expr = CastExpr(expr, tgt)

        if self.current_token() and self.current_token().type == TokenType.LPAREN:
            expr = self.parse_call(expr)

        return expr

    def parse_call(self, func_name: Any, type_args: list[Any] | None = None) -> Call:
        self.expect(TokenType.LPAREN, "Expected '(' after call name")
        args: list[Any] = []
        if self.current_token() and self.current_token().type != TokenType.RPAREN:
            while True:
                args.append(self.parse_expression())
                if self.current_token() and self.current_token().type == TokenType.COMMA:
                    self.advance(); continue
                break
        self.expect(TokenType.RPAREN, "Expected ')' after call arguments")
        return Call(func_name, args, type_args)

def parse(tokens: List[Token]) -> Program:
    return Parser(tokens).parse()
