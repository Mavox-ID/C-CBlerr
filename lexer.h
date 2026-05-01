#pragma once
#include "ast.h"

typedef struct {
    const char *src;
    int         src_len;
    int         pos;
    int         line, col;
    int         nesting;
    int         indent_stack[256];
    int         indent_top;
    Token      *tokens;
    int         tok_len, tok_cap;
} Lexer;

/* Main public API */
Token *tokenize(const char *source, int *out_count);
void   tokens_free(Token *toks, int count);

const char *tok_kind_name(TokKind k);
