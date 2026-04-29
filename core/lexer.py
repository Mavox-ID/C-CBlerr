from dataclasses import dataclass, field
from enum import Enum, auto
from typing import List, Optional, Tuple
from pathlib import Path

class TokenType(Enum):
    DEF = auto()
    RETURN = auto()
    ENDOFCODE = auto()
    IF = auto()
    ELSE = auto()
    EXTERN = auto()
    WHILE = auto()
    FOR = auto()
    BREAK = auto()
    CONTINUE = auto()
    STRUCT = auto()
    CONST = auto()
    IMPORT = auto()
    FROM = auto()
    MODULE = auto()
    AND = auto()
    OR = auto()
    NOT = auto()
    INT = auto()
    STR = auto()
    BOOL = auto()
    FLOAT = auto()
    VOID = auto()
    U8 = auto()
    U16 = auto()
    U32 = auto()
    U64 = auto()
    I8 = auto()
    I16 = auto()
    I32 = auto()
    I64 = auto()
    ASM = auto()
    COMPTIME = auto()
    AS = auto()
    PACKED = auto()
    INLINE = auto()
    AT = auto()
    ELLIPSIS = auto()
    NAME = auto()
    NUMBER = auto()
    STRING = auto()
    PLUS = auto()
    MINUS = auto()
    MULTIPLY = auto()
    DIVIDE = auto()
    MODULO = auto()
    POW = auto()
    EQ = auto()
    NE = auto()
    LT = auto()
    GT = auto()
    LE = auto()
    GE = auto()
    ASSIGN = auto()
    PLUS_ASSIGN = auto()
    MINUS_ASSIGN = auto()
    WALRUS = auto()
    LET = auto()
    LPAREN = auto()
    RPAREN = auto()
    LBRACE = auto()
    RBRACE = auto()
    LBRACKET = auto()
    RBRACKET = auto()
    LANGLE = auto()
    RANGLE = auto()
    COLON = auto()
    ARROW = auto()
    COMMA = auto()
    DOT = auto()
    SEMICOLON = auto()
    AMP = auto()
    PIPE = auto()
    CARET = auto()
    TILDE = auto()
    QUESTION = auto()
    RANGE = auto()
    MATCH = auto()
    CASE = auto()
    DEFAULT = auto()
    ENUM = auto()
    SIZEOF = auto()
    IN = auto()
    NEWLINE = auto()
    INDENT = auto()
    DEDENT = auto()
    EOF = auto()
    ERROR = auto()

@dataclass
class Token:
    type: TokenType
    value: Optional[str] = None
    line: int = 1
    column: int = 0
    end_column: int = field(default=0)

    def __str__(self) -> str:
        if self.value:
            return f"{self.type.name}({self.value!r}) @{self.line}:{self.column}"
        return f"{self.type.name} @{self.line}:{self.column}"

    def get_position(self) -> Tuple[int, int]:
        return (self.line, self.column)

class Lexer:
    KEYWORDS: dict[str, TokenType] = {
        'def': TokenType.DEF,
        'fn': TokenType.DEF,
        'let': TokenType.LET,
        'return': TokenType.RETURN,
        'if': TokenType.IF,
        'else': TokenType.ELSE,
        'extern': TokenType.EXTERN,
        'while': TokenType.WHILE,
        'for': TokenType.FOR,
        'break': TokenType.BREAK,
        'continue': TokenType.CONTINUE,
        'struct': TokenType.STRUCT,
        'const': TokenType.CONST,
        'and': TokenType.AND,
        'or': TokenType.OR,
        'not': TokenType.NOT,
        'int': TokenType.INT,
        'str': TokenType.STR,
        'bool': TokenType.BOOL,
        'float': TokenType.FLOAT,
        'void': TokenType.VOID,
        'endofcode': TokenType.ENDOFCODE,
        'u8': TokenType.U8,
        'u16': TokenType.U16,
        'u32': TokenType.U32,
        'u64': TokenType.U64,
        'i8': TokenType.I8,
        'i16': TokenType.I16,
        'i32': TokenType.I32,
        'i64': TokenType.I64,
        'asm': TokenType.ASM,
        'inline': TokenType.INLINE,
        'comptime': TokenType.COMPTIME,
        'as': TokenType.AS,
        'packed': TokenType.PACKED,
        'import': TokenType.IMPORT,
        'from': TokenType.FROM,
        'module': TokenType.MODULE,
        'match': TokenType.MATCH,
        'case': TokenType.CASE,
        'default': TokenType.DEFAULT,
        'enum': TokenType.ENUM,
        'sizeof': TokenType.SIZEOF,
        'in': TokenType.IN,
    }

    SINGLE_CHAR_TOKENS: dict[str, TokenType] = {
        '(': TokenType.LPAREN,
        ')': TokenType.RPAREN,
        '{': TokenType.LBRACE,
        '}': TokenType.RBRACE,
        '[': TokenType.LBRACKET,
        ']': TokenType.RBRACKET,
        ':': TokenType.COLON,
        ',': TokenType.COMMA,
        '@': TokenType.AT,
        '&': TokenType.AMP,
        '|': TokenType.PIPE,
        '^': TokenType.CARET,
        '~': TokenType.TILDE,
        '?': TokenType.QUESTION,
        ';': TokenType.SEMICOLON,
    }

    def __init__(self, source: str, filename: str = "<stdin>"):
        self.source: str = source
        self.filename: str = filename
        self.pos: int = 0
        self.line: int = 1
        self.column: int = 0
        self.tokens: List[Token] = []
        self.indent_stack: List[int] = [0]
        self.errors: List[Tuple[int, int, str]] = []
        self.recovery_mode: bool = False
        self.nesting_level: int = 0
        
    def current_char(self) -> Optional[str]:
        return self.source[self.pos] if self.pos < len(self.source) else None

    def peek_char(self, offset: int = 1) -> Optional[str]:
        pos = self.pos + offset
        return self.source[pos] if pos < len(self.source) else None

    def advance(self) -> Optional[str]:
        if self.pos >= len(self.source):
            return None
        char = self.source[self.pos]
        self.pos += 1
        if char == '\n':
            self.line += 1
            self.column = 0
        else:
            self.column += 1
        return char

    def skip_whitespace(self) -> None:
        while self.current_char() and self.current_char() in ' \t\r':
            self.advance()

    def skip_comment(self) -> None:
        while self.current_char() and self.current_char() != '\n':
            self.advance()

    def read_identifier(self) -> Token:
        start_line, start_col = self.line, self.column
        result = ''
        while self.current_char() and (self.current_char().isalnum() or self.current_char() == '_'):
            result += self.advance()
        if result in self.KEYWORDS:
            token_type = self.KEYWORDS[result]
        else:
            token_type = TokenType.NAME
        return Token(token_type, result, start_line, start_col, self.column)

    def read_number(self) -> Token:
        start_line, start_col = self.line, self.column
        result = ''
        if self.current_char() == '0':
            result += self.advance()
            char = self.current_char()
            if char and char in 'xX':
                result += self.advance()
                while self.current_char() and self.current_char() in '0123456789abcdefABCDEF':
                    result += self.advance()
                return Token(TokenType.NUMBER, result, start_line, start_col, self.column)
            elif char and char in 'bB':
                result += self.advance()
                while self.current_char() and self.current_char() in '01':
                    result += self.advance()
                return Token(TokenType.NUMBER, result, start_line, start_col, self.column)
        while self.current_char() and self.current_char().isdigit():
            result += self.advance()
        if self.current_char() == '.' and self.peek_char() and self.peek_char().isdigit():
            result += self.advance()
            while self.current_char() and self.current_char().isdigit():
                result += self.advance()
        if self.current_char() and self.current_char() in 'eE':
            result += self.advance()
            if self.current_char() and self.current_char() in '+-':
                result += self.advance()
            if not self.current_char() or not self.current_char().isdigit():
                self._add_error(f"Invalid exponent in number: {result}")
                return Token(TokenType.ERROR, result, start_line, start_col, self.column)
            while self.current_char() and self.current_char().isdigit():
                result += self.advance()
        return Token(TokenType.NUMBER, result, start_line, start_col, self.column)

    def read_string(self, quote_char: str) -> Token:
        start_line, start_col = self.line, self.column
        result = ''
        self.advance()
        while self.current_char() and self.current_char() != quote_char:
            if self.current_char() == '\\':
                self.advance()
                next_char = self.current_char()
                if next_char == 'n':
                    result += '\n'
                    self.advance()
                elif next_char == 't':
                    result += '\t'
                    self.advance()
                elif next_char == 'r':
                    result += '\r'
                    self.advance()
                elif next_char == '\\':
                    result += '\\'
                    self.advance()
                elif next_char == quote_char:
                    result += quote_char
                    self.advance()
                elif next_char == '0':
                    result += '\0'
                    self.advance()
                else:
                    result += next_char if next_char else '\\'
                    if next_char:
                        self.advance()
            elif self.current_char() == '\n':
                self._add_error(f"Unclosed string at line {start_line}")
                break
            else:
                result += self.advance()
        if self.current_char() == quote_char:
            self.advance()
        else:
            self._add_error(f"Unclosed string, started at {start_line}:{start_col}")
        return Token(TokenType.STRING, result, start_line, start_col, self.column)

    def process_indent(self, indent_level: int) -> None:
        current_indent = self.indent_stack[-1]
        if indent_level > current_indent:
            self.indent_stack.append(indent_level)
            self.tokens.append(Token(TokenType.INDENT, None, self.line, 0))
        elif indent_level < current_indent:
            while self.indent_stack and self.indent_stack[-1] > indent_level:
                self.indent_stack.pop()
                self.tokens.append(Token(TokenType.DEDENT, None, self.line, 0))
            if self.indent_stack and self.indent_stack[-1] != indent_level:
                self._add_error(f"Incorrect indentation level at line {self.line}")

    def _add_error(self, message: str) -> None:
        self.errors.append((self.line, self.column, message))
        self.recovery_mode = True

    def tokenize(self) -> List[Token]:
        while self.pos < len(self.source) and self.source[self.pos] in ' \t':
            if self.source[self.pos] == '\t':
                self.column += 4
            else:
                self.column += 1
            self.pos += 1
        line_start = 0
        at_line_start = True
        while self.pos < len(self.source):
            if at_line_start:
                line_start = self.pos
                if self.current_char() == '\n':
                    self.advance()
                    at_line_start = True
                    continue
                indent_level = 0
                temp_pos = self.pos
                while temp_pos < len(self.source) and self.source[temp_pos] == ' ':
                    indent_level += 1
                    temp_pos += 1
                if temp_pos >= len(self.source) or self.source[temp_pos] in '\n#':
                    while self.current_char() and self.current_char() != '\n':
                        self.advance()
                    if self.current_char() == '\n':
                        self.advance()
                    at_line_start = True
                    continue
                if self.tokens or indent_level > 0:
                    self.process_indent(indent_level)
                at_line_start = False
                self.skip_whitespace()
            char = self.current_char()
            if not char:
                break
            if char in ' \t\r':
                self.skip_whitespace()
                continue
            if char == '\n':
                self.advance()
                if self.nesting_level > 0:
                    continue
    
                self.tokens.append(Token(TokenType.NEWLINE, None, self.line, self.column - 1))
                at_line_start = True
                continue

            if char == '#':
                self.skip_comment()
                continue
            if char in ('"', "'"):
                token = self.read_string(char)
                self.tokens.append(token)
                continue
            if char.isdigit():
                token = self.read_number()
                self.tokens.append(token)
                continue
            if char.isalpha() or char == '_':
                token = self.read_identifier()
                self.tokens.append(token)
                continue
            if char == '.' and self.peek_char() == '.' and self.peek_char(2) == '.':
                self.advance()
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.ELLIPSIS, '...', self.line, self.column - 3))
                continue
            if char == '.' and self.peek_char() == '.' and self.peek_char(2) != '.':
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.RANGE, '..', self.line, self.column - 2))
                continue
            if char == '-' and self.peek_char() == '>':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.ARROW, '->', self.line, col))
                continue
            if char == ':' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.WALRUS, ':=', self.line, col))
                continue
            if char == '=' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.EQ, '==', self.line, col))
                continue
            if char == '!' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.NE, '!=', self.line, col))
                continue
            if char == '<' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.LE, '<=', self.line, col))
                continue
            if char == '>' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.GE, '>=', self.line, col))
                continue
            if char == '+' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.PLUS_ASSIGN, '+=', self.line, col))
                continue
            if char == '-' and self.peek_char() == '=':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.MINUS_ASSIGN, '-=', self.line, col))
                continue
            if char == '*' and self.peek_char() == '*':
                col = self.column
                self.advance()
                self.advance()
                self.tokens.append(Token(TokenType.POW, '**', self.line, col))
                continue
            col = self.column
            if char == '+':
                self.advance()
                self.tokens.append(Token(TokenType.PLUS, '+', self.line, col))
            elif char == '-':
                self.advance()
                self.tokens.append(Token(TokenType.MINUS, '-', self.line, col))
            elif char == '*':
                self.advance()
                self.tokens.append(Token(TokenType.MULTIPLY, '*', self.line, col))
            elif char == '/':
                self.advance()
                self.tokens.append(Token(TokenType.DIVIDE, '/', self.line, col))
            elif char == '%':
                self.advance()
                self.tokens.append(Token(TokenType.MODULO, '%', self.line, col))
            elif char == '=':
                self.advance()
                self.tokens.append(Token(TokenType.ASSIGN, '=', self.line, col))
            elif char == '<':
                self.advance()
                self.tokens.append(Token(TokenType.LT, '<', self.line, col))
            elif char == '>':
                self.advance()
                self.tokens.append(Token(TokenType.GT, '>', self.line, col))
            elif char == '!':
                self.advance()
                self._add_error(f"Unexpected character '!' at line {self.line}, column {col}")
                self.tokens.append(Token(TokenType.ERROR, '!', self.line, col))
            elif char == '.':
                self.advance()
                self.tokens.append(Token(TokenType.DOT, '.', self.line, col))
            elif char in self.SINGLE_CHAR_TOKENS:
                token_type = self.SINGLE_CHAR_TOKENS[char]
                self.advance()
                self.tokens.append(Token(token_type, char, self.line, col))
                
                if char in '([{':
                    self.nesting_level += 1
                elif char in ')]}':
                    if self.nesting_level > 0:
                        self.nesting_level -= 1
            else:
                self.advance()
                self._add_error(f"Unknown character '{char}' at line {self.line}, column {col}")
                self.tokens.append(Token(TokenType.ERROR, char, self.line, col))
        while len(self.indent_stack) > 1:
            self.indent_stack.pop()
            self.tokens.append(Token(TokenType.DEDENT, None, self.line, 0))
        self.tokens.append(Token(TokenType.EOF, None, self.line, self.column))
        return self.tokens

def tokenize(source: str, filename: str = "<stdin>") -> List[Token]:
    lexer = Lexer(source, filename)
    return lexer.tokenize()

def tokenize_file(filepath: str) -> List[Token]:
    path = Path(filepath)
    source = path.read_text(encoding='utf-8')
    return tokenize(source, str(path))
