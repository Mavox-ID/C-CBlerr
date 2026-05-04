/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

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

Token *tokenize(const char *source, int *out_count);
void   tokens_free(Token *toks, int count);

const char *tok_kind_name(TokKind k);
Token *tokenize_file(const char *filepath, int *out_count);
void tok_get_position(const Token *t, int *out_line, int *out_col);
void tok_to_str(const Token *t, char *out, int sz);
bool tok_is_error(const Token *t);
