#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#define _POSIX_C_SOURCE 200809L
/* strdup shim for strict C11 */
static inline char *_cbl_strdup(const char *s) {
    if(!s) return NULL;
    size_t l = strlen(s)+1;
    char *r = malloc(l);
    if(r) memcpy(r,s,l);
    return r;
}
#define strdup _cbl_strdup

/* ── string helpers ─────────────────────────────────────────────── */
CblStr cblstr(const char *s)     { CblStr r; r.s=(char*)s; r.len=s?(int)strlen(s):0; return r; }
CblStr cblstr_dup(const char *s) { CblStr r; r.s=s?strdup(s):NULL; r.len=s?(int)strlen(s):0; return r; }

/* ── node alloc ─────────────────────────────────────────────────── */
AstNode *node_new(NodeKind k, int line) {
    AstNode *n = calloc(1, sizeof(AstNode));
    n->kind = k; n->line = line;
    return n;
}
void node_free(AstNode *n) { if(n) free(n); }
Program *program_new(void) { return calloc(1, sizeof(Program)); }
void program_free(Program *p) { if(p) free(p); }

/* ── keyword table ──────────────────────────────────────────────── */
typedef struct { const char *word; TokKind kind; } KW;
static const KW KEYWORDS[] = {
    {"def",TK_DEF},{"fn",TK_DEF},{"let",TK_LET},{"return",TK_RETURN},
    {"if",TK_IF},{"else",TK_ELSE},{"extern",TK_EXTERN},
    {"while",TK_WHILE},{"for",TK_FOR},{"break",TK_BREAK},{"continue",TK_CONTINUE},
    {"struct",TK_STRUCT},{"const",TK_CONST},{"and",TK_AND},{"or",TK_OR},{"not",TK_NOT},
    {"int",TK_INT},{"str",TK_STR},{"bool",TK_BOOL},{"float",TK_FLOAT},{"void",TK_VOID},
    {"endofcode",TK_ENDOFCODE},
    {"u8",TK_U8},{"u16",TK_U16},{"u32",TK_U32},{"u64",TK_U64},
    {"i8",TK_I8},{"i16",TK_I16},{"i32",TK_I32},{"i64",TK_I64},
    {"asm",TK_ASM},{"inline",TK_INLINE},{"comptime",TK_COMPTIME},
    {"as",TK_AS},{"packed",TK_PACKED},{"import",TK_IMPORT},{"from",TK_FROM},
    {"module",TK_MODULE},{"match",TK_MATCH},{"case",TK_CASE},{"default",TK_DEFAULT},
    {"enum",TK_ENUM},{"sizeof",TK_SIZEOF},{"in",TK_IN},
    {NULL,TK_EOF}
};

static TokKind keyword_lookup(const char *w) {
    for(int i=0; KEYWORDS[i].word; i++)
        if(strcmp(w, KEYWORDS[i].word)==0) return KEYWORDS[i].kind;
    return TK_NAME;
}

/* ── lexer helpers ──────────────────────────────────────────────── */
static char lex_cur(Lexer *l)  { return l->pos < l->src_len ? l->src[l->pos] : 0; }
static char lex_peek(Lexer *l, int off) {
    int p = l->pos+off; return p < l->src_len ? l->src[p] : 0;
}
static char lex_adv(Lexer *l) {
    if(l->pos >= l->src_len) return 0;
    char c = l->src[l->pos++];
    if(c=='\n'){ l->line++; l->col=0; } else { l->col++; }
    return c;
}
static void emit(Lexer *l, TokKind k, char *val, int line, int col) {
    if(l->tok_len >= l->tok_cap) {
        l->tok_cap = l->tok_cap ? l->tok_cap*2 : 256;
        l->tokens = realloc(l->tokens, sizeof(Token)*(size_t)l->tok_cap);
    }
    l->tokens[l->tok_len++] = (Token){k, val, line, col};
}

/* ── read helpers ───────────────────────────────────────────────── */
static char *read_ident(Lexer *l, int *out_line, int *out_col) {
    *out_line=l->line; *out_col=l->col;
    int start=l->pos;
    while(isalnum((unsigned char)lex_cur(l)) || lex_cur(l)=='_') lex_adv(l);
    int len = l->pos - start;
    char *s = malloc((size_t)len+1);
    memcpy(s, l->src+start, (size_t)len); s[len]=0;
    return s;
}

static char *read_number(Lexer *l, int *out_line, int *out_col) {
    *out_line=l->line; *out_col=l->col;
    int start=l->pos;
    if(lex_cur(l)=='0' && (lex_peek(l,1)=='x'||lex_peek(l,1)=='X')) {
        lex_adv(l); lex_adv(l);
        while(isxdigit((unsigned char)lex_cur(l))) lex_adv(l);
    } else if(lex_cur(l)=='0' && (lex_peek(l,1)=='b'||lex_peek(l,1)=='B')) {
        lex_adv(l); lex_adv(l);
        while(lex_cur(l)=='0'||lex_cur(l)=='1') lex_adv(l);
    } else {
        while(isdigit((unsigned char)lex_cur(l))) lex_adv(l);
        if(lex_cur(l)=='.' && isdigit((unsigned char)lex_peek(l,1))) {
            lex_adv(l);
            while(isdigit((unsigned char)lex_cur(l))) lex_adv(l);
        }
        if(lex_cur(l)=='e'||lex_cur(l)=='E') {
            lex_adv(l);
            if(lex_cur(l)=='+'||lex_cur(l)=='-') lex_adv(l);
            while(isdigit((unsigned char)lex_cur(l))) lex_adv(l);
        }
    }
    int len=l->pos-start;
    char *s=malloc((size_t)len+1);
    memcpy(s,l->src+start,(size_t)len); s[len]=0;
    return s;
}

static char *read_string(Lexer *l, char quote, int *out_line, int *out_col) {
    *out_line=l->line; *out_col=l->col;
    lex_adv(l); /* skip opening quote */
    char buf[65536]; int bi=0;
    while(l->pos < l->src_len && lex_cur(l) != quote) {
        if(lex_cur(l)=='\\') {
            lex_adv(l);
            char nc=lex_adv(l);
            if(nc=='n')  buf[bi++]='\n';
            else if(nc=='t')  buf[bi++]='\t';
            else if(nc=='r')  buf[bi++]='\r';
            else if(nc=='0')  buf[bi++]='\0';
            else              buf[bi++]=nc;
        } else if(lex_cur(l)=='\n') {
            break;
        } else {
            buf[bi++]=lex_adv(l);
        }
        if(bi>=(int)sizeof(buf)-4) break;
    }
    if(lex_cur(l)==quote) lex_adv(l);
    buf[bi]=0;
    return strdup(buf);
}

static void process_indent(Lexer *l, int level, int line) {
    int cur = l->indent_stack[l->indent_top];
    if(level > cur) {
        if(l->indent_top < 255) l->indent_stack[++l->indent_top]=level;
        emit(l, TK_INDENT, NULL, line, 0);
    } else if(level < cur) {
        while(l->indent_top>0 && l->indent_stack[l->indent_top]>level) {
            l->indent_top--;
            emit(l, TK_DEDENT, NULL, line, 0);
        }
    }
}

/* ── main tokenize ──────────────────────────────────────────────── */
Token *tokenize(const char *source, int *out_count) {
    Lexer _l = {0}; Lexer *l = &_l;
    l->src     = source;
    l->src_len = (int)strlen(source);
    l->line    = 1;
    l->indent_stack[0] = 0;
    l->indent_top = 0;

    bool at_line_start = true;

    while(l->pos < l->src_len) {
        if(at_line_start) {
            /* measure indentation */
            if(lex_cur(l)=='\n') { lex_adv(l); continue; }
            int indent=0, tmp=l->pos;
            while(tmp < l->src_len && l->src[tmp]==' ') { indent++; tmp++; }
            /* blank/comment line */
            if(tmp >= l->src_len || l->src[tmp]=='\n' || l->src[tmp]=='#') {
                while(l->pos < l->src_len && lex_cur(l)!='\n') lex_adv(l);
                if(lex_cur(l)=='\n') lex_adv(l);
                continue;
            }
            process_indent(l, indent, l->line);
            at_line_start = false;
            /* skip spaces after indent measure */
            while(lex_cur(l)==' '||lex_cur(l)=='\t'||lex_cur(l)=='\r') lex_adv(l);
        }

        char c = lex_cur(l);
        if(!c) break;

        /* whitespace (non-newline) */
        if(c==' '||c=='\t'||c=='\r') { lex_adv(l); continue; }

        /* newline */
        if(c=='\n') {
            lex_adv(l);
            if(l->nesting==0)
                emit(l, TK_NEWLINE, NULL, l->line-1, l->col);
            at_line_start = true;
            continue;
        }

        /* comment */
        if(c=='#') {
            while(l->pos<l->src_len && lex_cur(l)!='\n') lex_adv(l);
            continue;
        }

        /* string */
        if(c=='"'||c=='\'') {
            int line,col; char *sv=read_string(l,c,&line,&col);
            emit(l, TK_STRING, sv, line, col); continue;
        }

        /* number */
        if(isdigit((unsigned char)c)) {
            int line,col; char *nv=read_number(l,&line,&col);
            emit(l, TK_NUMBER, nv, line, col); continue;
        }

        /* identifier / keyword */
        if(isalpha((unsigned char)c)||c=='_') {
            int line,col; char *id=read_ident(l,&line,&col);
            TokKind k=keyword_lookup(id);
            if(k!=TK_NAME) { free(id); id=NULL; }
            emit(l, k, id, line, col); continue;
        }

        /* multi-char operators */
        int line=l->line, col=l->col;
        if(c=='.'&&lex_peek(l,1)=='.'&&lex_peek(l,2)=='.') { lex_adv(l);lex_adv(l);lex_adv(l); emit(l,TK_ELLIPSIS,strdup("..."),line,col); continue; }
        if(c=='.'&&lex_peek(l,1)=='.') { lex_adv(l);lex_adv(l); emit(l,TK_RANGE,strdup(".."),line,col); continue; }
        if(c=='-'&&lex_peek(l,1)=='>') { lex_adv(l);lex_adv(l); emit(l,TK_ARROW,strdup("->"),line,col); continue; }
        if(c==':'&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_WALRUS,strdup(":="),line,col); continue; }
        if(c=='='&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_EQ,strdup("=="),line,col); continue; }
        if(c=='!'&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_NE,strdup("!="),line,col); continue; }
        if(c=='<'&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_LE,strdup("<="),line,col); continue; }
        if(c=='>'&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_GE,strdup(">="),line,col); continue; }
        if(c=='+'&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_PLUS_ASSIGN,strdup("+="),line,col); continue; }
        if(c=='-'&&lex_peek(l,1)=='=') { lex_adv(l);lex_adv(l); emit(l,TK_MINUS_ASSIGN,strdup("-="),line,col); continue; }
        if(c=='*'&&lex_peek(l,1)=='*') { lex_adv(l);lex_adv(l); emit(l,TK_POW,strdup("**"),line,col); continue; }

        /* single-char */
        lex_adv(l);
        char sv2[2]={(char)c,0};
        switch(c) {
            case '+': emit(l,TK_PLUS, strdup(sv2),line,col); break;
            case '-': emit(l,TK_MINUS,strdup(sv2),line,col); break;
            case '*': emit(l,TK_MUL,  strdup(sv2),line,col); break;
            case '/': emit(l,TK_DIV,  strdup(sv2),line,col); break;
            case '%': emit(l,TK_MOD,  strdup(sv2),line,col); break;
            case '=': emit(l,TK_ASSIGN,strdup(sv2),line,col); break;
            case '<': emit(l,TK_LT,  strdup(sv2),line,col); break;
            case '>': emit(l,TK_GT,  strdup(sv2),line,col); break;
            case '.': emit(l,TK_DOT, strdup(sv2),line,col); break;
            case '(': emit(l,TK_LPAREN,  strdup(sv2),line,col); l->nesting++; break;
            case ')': emit(l,TK_RPAREN,  strdup(sv2),line,col); if(l->nesting>0)l->nesting--; break;
            case '{': emit(l,TK_LBRACE,  strdup(sv2),line,col); l->nesting++; break;
            case '}': emit(l,TK_RBRACE,  strdup(sv2),line,col); if(l->nesting>0)l->nesting--; break;
            case '[': emit(l,TK_LBRACKET,strdup(sv2),line,col); l->nesting++; break;
            case ']': emit(l,TK_RBRACKET,strdup(sv2),line,col); if(l->nesting>0)l->nesting--; break;
            case ':': emit(l,TK_COLON,  strdup(sv2),line,col); break;
            case ',': emit(l,TK_COMMA,  strdup(sv2),line,col); break;
            case '@': emit(l,TK_AT,     strdup(sv2),line,col); break;
            case '&': emit(l,TK_AMP,    strdup(sv2),line,col); break;
            case '|': emit(l,TK_PIPE,   strdup(sv2),line,col); break;
            case '^': emit(l,TK_CARET,  strdup(sv2),line,col); break;
            case '~': emit(l,TK_TILDE,  strdup(sv2),line,col); break;
            case '?': emit(l,TK_QUESTION,strdup(sv2),line,col); break;
            case ';': emit(l,TK_SEMICOLON,strdup(sv2),line,col); break;
            default:  emit(l,TK_ERROR,  strdup(sv2),line,col); break;
        }
    }

    /* flush remaining DEDENT */
    while(l->indent_top>0) {
        l->indent_top--;
        emit(l, TK_DEDENT, NULL, l->line, 0);
    }
    emit(l, TK_EOF, NULL, l->line, l->col);

    *out_count = l->tok_len;
    return l->tokens;
}

void tokens_free(Token *toks, int count) {
    for(int i=0;i<count;i++) free(toks[i].val);
    free(toks);
}

const char *tok_kind_name(TokKind k) {
    static const char *names[] = {
        "def","return","endofcode","if","else","extern",
        "while","for","break","continue","struct","const",
        "import","from","module","and","or","not",
        "int","str","bool","float","void",
        "u8","u16","u32","u64","i8","i16","i32","i64",
        "asm","comptime","as","packed","inline",
        "@","...","NAME","NUMBER","STRING",
        "+","-","*","/","%","**",
        "==","!=","<",">","<=",">=",
        "=","+=","-=",":=",
        "let","(",")","{","}","[","]","<",">",
        ":","->",",",".",";"
        ,"&","|","^","~","?",
        "..","match","case","default","enum",
        "sizeof","in","NEWLINE","INDENT","DEDENT",
        "EOF","ERROR"
    };
    if((int)k < (int)(sizeof(names)/sizeof(names[0]))) return names[k];
    return "?";
}
