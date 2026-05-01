#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── string helper ──────────────────────────────────────────────── */
typedef struct { char *s; size_t len; } CblStr;
CblStr cblstr(const char *s);            /* wraps a C-string */
CblStr cblstr_dup(const char *s);        /* heap-allocates a copy  */

/* ── token types ────────────────────────────────────────────────── */
typedef enum {
    TK_DEF, TK_RETURN, TK_ENDOFCODE, TK_IF, TK_ELSE, TK_EXTERN,
    TK_WHILE, TK_FOR, TK_BREAK, TK_CONTINUE, TK_STRUCT, TK_CONST,
    TK_IMPORT, TK_FROM, TK_MODULE, TK_AND, TK_OR, TK_NOT,
    TK_INT, TK_STR, TK_BOOL, TK_FLOAT, TK_VOID,
    TK_U8, TK_U16, TK_U32, TK_U64,
    TK_I8, TK_I16, TK_I32, TK_I64,
    TK_ASM, TK_COMPTIME, TK_AS, TK_PACKED, TK_INLINE,
    TK_AT, TK_ELLIPSIS, TK_NAME, TK_NUMBER, TK_STRING,
    TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_MOD, TK_POW,
    TK_EQ, TK_NE, TK_LT, TK_GT, TK_LE, TK_GE,
    TK_ASSIGN, TK_PLUS_ASSIGN, TK_MINUS_ASSIGN, TK_WALRUS,
    TK_LET, TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE,
    TK_LBRACKET, TK_RBRACKET, TK_LANGLE, TK_RANGLE,
    TK_COLON, TK_ARROW, TK_COMMA, TK_DOT, TK_SEMICOLON,
    TK_AMP, TK_PIPE, TK_CARET, TK_TILDE, TK_QUESTION,
    TK_RANGE, TK_MATCH, TK_CASE, TK_DEFAULT, TK_ENUM,
    TK_SIZEOF, TK_IN, TK_NEWLINE, TK_INDENT, TK_DEDENT,
    TK_EOF, TK_ERROR
} TokKind;

typedef struct {
    TokKind  kind;
    char    *val;   /* heap-allocated, may be NULL */
    int      line, col;
} Token;

/* ── dynamic list helpers ───────────────────────────────────────── */
#define VEC(T) struct { T *data; int len, cap; }
#define VEC_PUSH(v, item) do { \
    if ((v)->len >= (v)->cap) { \
        (v)->cap = (v)->cap ? (v)->cap*2 : 8; \
        (v)->data = realloc((v)->data, sizeof(*(v)->data)*(size_t)(v)->cap); \
    } \
    (v)->data[(v)->len++] = (item); \
} while(0)
#define VEC_FREE(v) do { free((v)->data); (v)->data=NULL; (v)->len=(v)->cap=0; } while(0)

/* ── type representation ────────────────────────────────────────── */
/* Types are stored as strings: "int","str","*void","array<i32>",etc. */
typedef char* TypeStr;

/* ── AST node kinds ─────────────────────────────────────────────── */
typedef enum {
    ND_LITERAL_INT, ND_LITERAL_FLOAT, ND_LITERAL_STR, ND_LITERAL_BOOL,
    ND_VARIABLE,
    ND_BINARY,    /* +,-,*,/,%,**,&,|,^,<<,>> */
    ND_COMPARE,   /* ==,!=,<,>,<=,>= */
    ND_LOGICAL,   /* and,or,not */
    ND_ASSIGN,
    ND_RETURN,
    ND_IF,
    ND_WHILE,
    ND_FOR,
    ND_BREAK,
    ND_CONTINUE,
    ND_CALL,
    ND_FIELD,     /* obj.field */
    ND_INDEX,     /* arr[idx] */
    ND_ARRAY_LIT,
    ND_DEREF,
    ND_ADDR,
    ND_CAST,
    ND_SIZEOF,
    ND_INLINE_ASM,
    ND_MATCH,
    ND_WALRUS,
} NodeKind;

typedef struct AstNode AstNode;
typedef VEC(AstNode*) NodeVec;

/* ── parameter: (name, type_str) ───────────────────────────────── */
typedef struct { char *name; TypeStr type; } Param;
typedef VEC(Param) ParamVec;

/* ── match case ─────────────────────────────────────────────────── */
typedef struct {
    NodeVec vals;   /* empty = default */
    NodeVec body;
} MatchCase;
typedef VEC(MatchCase) CaseVec;

/* ── AstNode ────────────────────────────────────────────────────── */
struct AstNode {
    NodeKind kind;
    int line;
    union {
        /* literals */
        int64_t  ival;
        double   fval;
        char    *sval;   /* str literal or variable name */
        /* binary / compare / logical / walrus */
        struct { char *op; AstNode *left, *right; } binop;
        /* assign */
        struct { AstNode *target; AstNode *value; TypeStr var_type; } assign;
        /* return */
        struct { AstNode *value; bool is_endofcode; } ret;
        /* if */
        struct { AstNode *cond; NodeVec then, els; } ifst;
        /* while */
        struct { AstNode *cond; NodeVec body; } whl;
        /* for - C-style or iterator */
        struct {
            AstNode *init, *cond, *post;        /* C-style */
            char *iter_var; AstNode *iter_expr; /* for-in */
            NodeVec body;
        } forl;
        /* call */
        struct { AstNode *func; NodeVec args; } call;
        /* field access */
        struct { AstNode *obj; char *field; } fld;
        /* array index */
        struct { AstNode *arr; AstNode *idx; } idx;
        /* array literal */
        struct { NodeVec elems; TypeStr elem_type; bool is_struct_init; } arlit;
        /* deref / addr-of */
        AstNode *inner;
        /* cast */
        struct { AstNode *expr; TypeStr target; } cast;
        /* sizeof */
        struct { bool is_type; TypeStr type_s; AstNode *expr; } szof;
        /* inline asm */
        char *asm_code;
        /* match */
        struct { AstNode *expr; CaseVec cases; } match;
    };
};

/* ── top-level declarations ─────────────────────────────────────── */
typedef struct {
    char *name;
    char *value;   /* field decorator arg if present */
} Decorator;
typedef VEC(Decorator) DecorVec;

typedef struct {
    char *name;
    char *field;
    char *type;
    char *value;   /* optional default / enum value */
} StructField;
typedef VEC(StructField) FieldVec;

typedef struct {
    char *name;
    FieldVec fields;
    DecorVec decorators;
    bool is_enum;                /* true → fields are enum members */
} StructDef;
typedef VEC(StructDef) StructVec;

typedef struct {
    char     *name;
    ParamVec  params;
    TypeStr   return_type;
    NodeVec   body;
    bool      is_extern;
    bool      is_vararg;
    DecorVec  decorators;
} FuncDef;
typedef VEC(FuncDef) FuncVec;

typedef struct {
    char    *name;
    TypeStr  type;
    AstNode *value;
    bool     is_const;
} GlobalVar;
typedef VEC(GlobalVar) GlobVec;

typedef struct {
    char *module_name;
    char **items;    /* NULL = import all */
    int   n_items;
} Import;
typedef VEC(Import) ImportVec;

typedef struct {
    FuncVec   funcs;
    StructVec structs;
    GlobVec   globals;
    ImportVec imports;
} Program;

/* ── forward decls for alloc helpers ───────────────────────────── */
AstNode *node_new(NodeKind k, int line);
void     node_free(AstNode *n);
Program *program_new(void);
void     program_free(Program *p);
