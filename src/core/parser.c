/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
static inline char *_cbl_strdup(const char *s) {
    if(!s) return NULL;
    size_t l = strlen(s)+1; char *r = malloc(l);
    if(r) { memcpy(r,s,l); }
    return r;
}
#define strdup _cbl_strdup

typedef struct {
    Token *toks;
    int    count;
    int    pos;
} Parser;

static Token *cur(Parser *p)          { return p->pos<p->count ? &p->toks[p->pos] : NULL; }
static Token *peek(Parser *p, int o)  { int i=p->pos+o; return i<p->count?&p->toks[i]:NULL; }
static Token *adv(Parser *p)          { if(p->pos<p->count)return &p->toks[p->pos++]; return NULL; }

static void skip_newlines(Parser *p) {
    while(cur(p) && cur(p)->kind==TK_NEWLINE) adv(p);
}

static Token *expect(Parser *p, TokKind k, const char *msg) {
    Token *t = cur(p);
    if(!t || t->kind != k) {
        fprintf(stderr,"[ERROR] %s (got %s) at line %d\n",
                msg, t?tok_kind_name(t->kind):"EOF", t?t->line:0);
        return NULL;
    }
    return adv(p);
}

/* JUST PARSER!!!! */

static char *parse_type(Parser *p);
static char *parse_type(Parser *p) {
    Token *t = cur(p);
    if(!t) return strdup("void");

    if(t->kind == TK_MUL) {
        adv(p);
        char *inner = parse_type(p);
        char *r = malloc(strlen(inner)+2);
        r[0]='*'; strcpy(r+1,inner); free(inner);
        return r;
    }

    static const TokKind type_toks[] = {
        TK_NAME,TK_INT,TK_STR,TK_BOOL,TK_FLOAT,TK_VOID,
        TK_U8,TK_U16,TK_U32,TK_U64,TK_I8,TK_I16,TK_I32,TK_I64
    };
    bool is_type = false;
    for(int i=0;i<(int)(sizeof(type_toks)/sizeof(type_toks[0]));i++)
        if(t->kind==type_toks[i]){is_type=true;break;}

    if(!is_type) return strdup("int");

    const char *name = t->val ? t->val : tok_kind_name(t->kind);
    char base[256]; strncpy(base, name, 255); base[255]=0;
    adv(p);

    if(cur(p) && cur(p)->kind==TK_LT) {
        adv(p);
        char buf[512]; snprintf(buf,sizeof(buf),"%s<",base);
        bool first=true;
        while(cur(p) && cur(p)->kind!=TK_GT) {
            if(!first) strncat(buf,",",sizeof(buf)-strlen(buf)-1);
            first=false;
            char *arg=parse_type(p);
            strncat(buf,arg,sizeof(buf)-strlen(buf)-1);
            free(arg);
            if(cur(p)&&cur(p)->kind==TK_COMMA) adv(p);
        }
        if(cur(p)&&cur(p)->kind==TK_GT) adv(p);
        strncat(buf,">",sizeof(buf)-strlen(buf)-1);
        return strdup(buf);
    }
    return strdup(base);
}

static AstNode *parse_expr(Parser *p);
static AstNode *parse_stmt(Parser *p);
static AstNode *parse_atom(Parser *p);

static AstNode *parse_call(Parser *p, AstNode *func, int line) {
    adv(p);
    NodeVec args = {0};
    while(cur(p) && cur(p)->kind!=TK_RPAREN) {
        AstNode *a = parse_expr(p);
        if(a) VEC_PUSH(&args, a);
        if(cur(p)&&cur(p)->kind==TK_COMMA) adv(p);
    }
    if(cur(p)&&cur(p)->kind==TK_RPAREN) adv(p);
    AstNode *n = node_new(ND_CALL, line);
    n->call.func = func;
    n->call.args = args;
    return n;
}

static AstNode *parse_postfix(Parser *p, AstNode *expr) {
    while(cur(p)) {
        if(cur(p)->kind==TK_DOT) {
            adv(p);
            Token *field = expect(p, TK_NAME, "Expected field name");
            AstNode *n = node_new(ND_FIELD, expr?expr->line:0);
            n->fld.obj   = expr;
            n->fld.field = field ? strdup(field->val) : strdup("?");
            expr = n;
        } else if(cur(p)->kind==TK_LBRACKET) {
            int line = cur(p)->line; adv(p);
            AstNode *idx = parse_expr(p);
            if(cur(p)&&cur(p)->kind==TK_RBRACKET) adv(p);
            AstNode *n = node_new(ND_INDEX, line);
            n->idx.arr = expr; n->idx.idx = idx;
            expr = n;
        } else if(cur(p)->kind==TK_AS) {
            adv(p);
            char *t = parse_type(p);
            AstNode *n = node_new(ND_CAST, expr?expr->line:0);
            n->cast.expr=expr; n->cast.target=t;
            expr=n;
        } else if(cur(p)->kind==TK_LPAREN) {
            int line=cur(p)->line;
            expr = parse_call(p, expr, line);
        } else break;
    }
    return expr;
}

static AstNode *parse_atom(Parser *p) {
    Token *t = cur(p);
    if(!t) return NULL;
    int line = t->line;

    if(t->kind==TK_STRING) {
        adv(p);
        AstNode *n = node_new(ND_LITERAL_STR, line);
        n->sval = strdup(t->val?t->val:"");
        return parse_postfix(p,n);
    }
    if(t->kind==TK_NUMBER) {
        adv(p);
        const char *nv = t->val?t->val:"0";
        if(strchr(nv,'.')) {
            AstNode *n=node_new(ND_LITERAL_FLOAT,line);
            n->fval=atof(nv); return parse_postfix(p,n);
        } else {
            AstNode *n=node_new(ND_LITERAL_INT,line);
            n->ival=strtoll(nv,NULL,0); return parse_postfix(p,n);
        }
    }
    if(t->kind==TK_NAME) {
        char *name=strdup(t->val?t->val:""); adv(p);
        if(cur(p)&&cur(p)->kind==TK_LPAREN) {
            int lline=cur(p)->line;
            AstNode *fn=node_new(ND_VARIABLE,line);
            fn->sval=name;
            return parse_postfix(p, parse_call(p,fn,lline));
        }
        AstNode *n=node_new(ND_VARIABLE,line);
        n->sval=name;
        return parse_postfix(p,n);
    }
    if(t->kind==TK_LPAREN) {
        adv(p);
        AstNode *e=parse_expr(p);
        if(cur(p)&&cur(p)->kind==TK_RPAREN) adv(p);
        return parse_postfix(p,e);
    }
    if(t->kind==TK_LBRACKET||t->kind==TK_LBRACE) {
        bool is_struct = (t->kind==TK_LBRACE);
        TokKind end = is_struct ? TK_RBRACE : TK_RBRACKET;
        adv(p);
        NodeVec elems={0};
        while(cur(p)&&cur(p)->kind!=end) {
            AstNode *e=parse_expr(p);
            if(e) VEC_PUSH(&elems,e);
            if(cur(p)&&cur(p)->kind==TK_COMMA) adv(p);
        }
        if(cur(p)&&cur(p)->kind==end) adv(p);
        AstNode *n=node_new(ND_ARRAY_LIT,line);
        n->arlit.elems=elems;
        n->arlit.is_struct_init=is_struct;
        return parse_postfix(p,n);
    }
    adv(p);
    AstNode *n=node_new(ND_LITERAL_INT,line); n->ival=0;
    return n;
}

static AstNode *parse_unary(Parser *p) {
    Token *t=cur(p); if(!t) return NULL;
    int line=t->line;
    if(t->kind==TK_MUL) { adv(p); AstNode *inner=parse_unary(p); AstNode *n=node_new(ND_DEREF,line); n->inner=inner; return n; }
    if(t->kind==TK_AMP) { adv(p); AstNode *inner=parse_unary(p); AstNode *n=node_new(ND_ADDR,line); n->inner=inner; return n; }
    if(t->kind==TK_MINUS) {
        adv(p); AstNode *inner=parse_unary(p);
        AstNode *zero=node_new(ND_LITERAL_INT,line); zero->ival=0;
        AstNode *n=node_new(ND_BINARY,line); n->binop.op=strdup("-"); n->binop.left=zero; n->binop.right=inner;
        return n;
    }
    if(t->kind==TK_SIZEOF) {
        adv(p);
        expect(p,TK_LPAREN,"Expected '(' after sizeof");
        AstNode *n=node_new(ND_SIZEOF,line);
        n->szof.is_type=true;
        n->szof.type_s=parse_type(p);
        n->szof.expr=NULL;
        expect(p,TK_RPAREN,"Expected ')' after sizeof type");
        return n;
    }
    return parse_atom(p);
}

static AstNode *parse_power(Parser *p) {
    AstNode *l=parse_unary(p);
    while(cur(p)&&cur(p)->kind==TK_POW) {
        int line=cur(p)->line; adv(p);
        AstNode *r=parse_unary(p);
        AstNode *n=node_new(ND_BINARY,line); n->binop.op=strdup("**"); n->binop.left=l; n->binop.right=r;
        l=n;
    }
    return l;
}

static AstNode *parse_mul(Parser *p) {
    AstNode *l=parse_power(p);
    while(cur(p)&&(cur(p)->kind==TK_MUL||cur(p)->kind==TK_DIV||cur(p)->kind==TK_MOD)) {
        int line=cur(p)->line; char *op=strdup(cur(p)->val?cur(p)->val:"*"); adv(p);
        AstNode *r=parse_power(p);
        AstNode *n=node_new(ND_BINARY,line); n->binop.op=op; n->binop.left=l; n->binop.right=r;
        l=n;
    }
    return l;
}

static AstNode *parse_add(Parser *p) {
    AstNode *l=parse_mul(p);
    while(cur(p)&&(cur(p)->kind==TK_PLUS||cur(p)->kind==TK_MINUS)) {
        int line=cur(p)->line; char *op=strdup(cur(p)->val?cur(p)->val:"+"); adv(p);
        AstNode *r=parse_mul(p);
        AstNode *n=node_new(ND_BINARY,line); n->binop.op=op; n->binop.left=l; n->binop.right=r;
        l=n;
    }
    return l;
}

static AstNode *parse_compare(Parser *p) {
    AstNode *l=parse_add(p);
    static const TokKind cmp_toks[]={TK_EQ,TK_NE,TK_LT,TK_GT,TK_LE,TK_GE};
    while(cur(p)) {
        bool is_cmp=false;
        for(int i=0;i<6;i++) if(cur(p)->kind==cmp_toks[i]){is_cmp=true;break;}
        if(!is_cmp) break;
        int line=cur(p)->line; char *op=strdup(cur(p)->val?cur(p)->val:"=="); adv(p);
        AstNode *r=parse_add(p);
        AstNode *n=node_new(ND_COMPARE,line); n->binop.op=op; n->binop.left=l; n->binop.right=r;
        l=n;
    }
    return l;
}

static AstNode *parse_not(Parser *p) {
    if(cur(p)&&cur(p)->kind==TK_NOT) {
        int line=cur(p)->line; adv(p);
        AstNode *inner=parse_not(p);
        AstNode *n=node_new(ND_LOGICAL,line); n->binop.op=strdup("not"); n->binop.left=inner; n->binop.right=NULL;
        return n;
    }
    return parse_compare(p);
}

static AstNode *parse_and(Parser *p) {
    AstNode *l=parse_not(p);
    while(cur(p)&&cur(p)->kind==TK_AND) {
        int line=cur(p)->line; adv(p);
        AstNode *r=parse_not(p);
        AstNode *n=node_new(ND_LOGICAL,line); n->binop.op=strdup("and"); n->binop.left=l; n->binop.right=r;
        l=n;
    }
    return l;
}

static AstNode *parse_or(Parser *p) {
    AstNode *l=parse_and(p);
    while(cur(p)&&cur(p)->kind==TK_OR) {
        int line=cur(p)->line; adv(p);
        AstNode *r=parse_and(p);
        AstNode *n=node_new(ND_LOGICAL,line); n->binop.op=strdup("or"); n->binop.left=l; n->binop.right=r;
        l=n;
    }
    return l;
}

static AstNode *parse_expr(Parser *p) {
    AstNode *e=parse_or(p);
    if(cur(p)&&cur(p)->kind==TK_WALRUS) {
        int line=cur(p)->line; adv(p);
        AstNode *val=parse_expr(p);
        AstNode *n=node_new(ND_WALRUS,line); n->binop.left=e; n->binop.right=val; n->binop.op=strdup(":=");
        return n;
    }
    return e;
}

static NodeVec parse_block(Parser *p);

static NodeVec parse_block(Parser *p) {
    NodeVec body={0};
    skip_newlines(p);
    if(cur(p)&&cur(p)->kind==TK_INDENT) { adv(p); } else { return body; }
    while(cur(p)&&cur(p)->kind!=TK_DEDENT) {
        skip_newlines(p);
        if(cur(p)&&cur(p)->kind==TK_DEDENT) break;
        AstNode *s=parse_stmt(p);
        if(s) VEC_PUSH(&body,s);
        skip_newlines(p);
    }
    if(cur(p)&&cur(p)->kind==TK_DEDENT) adv(p);
    return body;
}

static AstNode *parse_if(Parser *p) {
    int line=cur(p)->line; adv(p);
    AstNode *cond=parse_expr(p);
    expect(p,TK_COLON,"Expected ':' after if condition");
    NodeVec then=parse_block(p);
    NodeVec els={0};
    if(cur(p)&&cur(p)->kind==TK_ELSE) {
        adv(p);
        expect(p,TK_COLON,"Expected ':' after else");
        els=parse_block(p);
    }
    AstNode *n=node_new(ND_IF,line);
    n->ifst.cond=cond; n->ifst.then=then; n->ifst.els=els;
    return n;
}

static AstNode *parse_while(Parser *p) {
    int line=cur(p)->line; adv(p);
    AstNode *cond=parse_expr(p);
    expect(p,TK_COLON,"Expected ':' after while");
    NodeVec body=parse_block(p);
    AstNode *n=node_new(ND_WHILE,line);
    n->whl.cond=cond; n->whl.body=body;
    return n;
}

static AstNode *parse_for(Parser *p) {
    int line=cur(p)->line; adv(p);
    AstNode *n=node_new(ND_FOR,line);
    memset(&n->forl,0,sizeof(n->forl));

    if(cur(p)&&cur(p)->kind==TK_LPAREN) {
        adv(p);
        AstNode *init=NULL;
        if(cur(p)&&cur(p)->kind==TK_SEMICOLON) { /* empty :> */ }
        else { init=parse_stmt(p); }
        expect(p,TK_SEMICOLON,"Expected ';' in for");
        AstNode *cond=NULL;
        if(cur(p)&&cur(p)->kind!=TK_SEMICOLON) cond=parse_expr(p);
        expect(p,TK_SEMICOLON,"Expected ';' in for");
        AstNode *post=NULL;
        if(cur(p)&&cur(p)->kind!=TK_RPAREN) post=parse_expr(p);
        expect(p,TK_RPAREN,"Expected ')' after for header");
        expect(p,TK_COLON,"Expected ':' after for header");
        NodeVec body=parse_block(p);
        n->forl.init=init; n->forl.cond=cond; n->forl.post=post;
        n->forl.body=body;
        return n;
    }

    if(cur(p)&&cur(p)->kind==TK_NAME) {
        char *var=strdup(cur(p)->val?cur(p)->val:""); adv(p);
        expect(p,TK_IN,"Expected 'in'");
        if(cur(p)&&cur(p)->kind==TK_NUMBER&&peek(p,1)&&peek(p,1)->kind==TK_RANGE) {
            Token *st=adv(p); adv(p);
            Token *en=expect(p,TK_NUMBER,"Expected end of range");
            AstNode *sfn=node_new(ND_VARIABLE,line); sfn->sval=strdup("range");
            AstNode *as=node_new(ND_LITERAL_INT,line); as->ival=strtoll(st->val,NULL,0);
            AstNode *ae=node_new(ND_LITERAL_INT,line); ae->ival=en?strtoll(en->val,NULL,0):0;
            NodeVec args={0}; VEC_PUSH(&args,as); VEC_PUSH(&args,ae);
            AstNode *call=node_new(ND_CALL,line); call->call.func=sfn; call->call.args=args;
            n->forl.iter_var=var; n->forl.iter_expr=call;
        } else {
            n->forl.iter_var=var;
            n->forl.iter_expr=parse_expr(p);
        }
        expect(p,TK_COLON,"Expected ':' after for header");
        n->forl.body=parse_block(p);
        return n;
    }
    return n;
}

static AstNode *parse_match(Parser *p) {
    int line=cur(p)->line; adv(p);
    AstNode *expr=parse_expr(p);
    expect(p,TK_COLON,"Expected ':' after match");
    skip_newlines(p);
    expect(p,TK_INDENT,"Expected indent for match");
    CaseVec cases={0};
    while(cur(p)&&cur(p)->kind!=TK_DEDENT) {
        skip_newlines(p);
        if(!cur(p)||cur(p)->kind==TK_DEDENT) break;
        MatchCase mc; memset(&mc,0,sizeof(mc));
        if(cur(p)->kind==TK_CASE) {
            adv(p);
            while(1) {
                AstNode *v=parse_expr(p); VEC_PUSH(&mc.vals,v);
                if(cur(p)&&cur(p)->kind==TK_COMMA){adv(p);continue;} break;
            }
        } else if(cur(p)->kind==TK_DEFAULT) {
            adv(p); /* no values = default */
        }
        expect(p,TK_COLON,"Expected ':' after case");
        mc.body=parse_block(p);
        VEC_PUSH(&cases,mc);
        skip_newlines(p);
    }
    if(cur(p)&&cur(p)->kind==TK_DEDENT) adv(p);
    AstNode *n=node_new(ND_MATCH,line);
    n->match.expr=expr; n->match.cases=cases;
    return n;
}

static AstNode *parse_stmt(Parser *p) {
    Token *t=cur(p);
    if(!t||t->kind==TK_EOF) return NULL;
    int line=t->line;

    if(t->kind==TK_NEWLINE) { adv(p); return NULL; }

    if(t->kind==TK_ASM) {
        adv(p);
        expect(p,TK_LPAREN,"Expected '(' after asm");
        Token *s=expect(p,TK_STRING,"Expected asm string");
        expect(p,TK_RPAREN,"Expected ')'");
        AstNode *n=node_new(ND_INLINE_ASM,line);
        n->asm_code=strdup(s?s->val:"");
        return n;
    }
    if(t->kind==TK_RETURN) {
        adv(p);
        AstNode *v=NULL;
        if(cur(p)&&cur(p)->kind!=TK_NEWLINE&&cur(p)->kind!=TK_DEDENT&&cur(p)->kind!=TK_EOF)
            v=parse_expr(p);
        AstNode *n=node_new(ND_RETURN,line); n->ret.value=v; n->ret.is_endofcode=false;
        return n;
    }
    if(t->kind==TK_ENDOFCODE) {
        adv(p);
        AstNode *zero=node_new(ND_LITERAL_INT,line); zero->ival=0;
        AstNode *n=node_new(ND_RETURN,line); n->ret.value=zero; n->ret.is_endofcode=true;
        return n;
    }
    if(t->kind==TK_IF)    return parse_if(p);
    if(t->kind==TK_WHILE) return parse_while(p);
    if(t->kind==TK_FOR)   return parse_for(p);
    if(t->kind==TK_MATCH) return parse_match(p);
    if(t->kind==TK_BREAK) { adv(p); return node_new(ND_BREAK,line); }
    if(t->kind==TK_CONTINUE){adv(p); return node_new(ND_CONTINUE,line);}

    if(t->kind==TK_LET) {
        adv(p);
        Token *name=expect(p,TK_NAME,"Expected variable name after let");
        expect(p,TK_ASSIGN,"Expected '=' in let");
        AstNode *val=parse_expr(p);
        AstNode *n=node_new(ND_ASSIGN,line);
        AstNode *var=node_new(ND_VARIABLE,line); var->sval=strdup(name?name->val:"?");
        n->assign.target=var; n->assign.value=val; n->assign.var_type=NULL;
        return n;
    }

    if(t->kind==TK_NAME && peek(p,1) && peek(p,1)->kind==TK_COLON) {
        char *name=strdup(t->val?t->val:""); adv(p); adv(p);
        char *type=parse_type(p);
        AstNode *val=NULL;
        if(cur(p)&&cur(p)->kind==TK_ASSIGN) { adv(p); val=parse_expr(p); }
        else { val=node_new(ND_LITERAL_INT,line); val->ival=0; }
        AstNode *n=node_new(ND_ASSIGN,line);
        AstNode *var=node_new(ND_VARIABLE,line); var->sval=name;
        n->assign.target=var; n->assign.value=val; n->assign.var_type=type;
        return n;
    }

    AstNode *left=parse_expr(p);
    if(!left) return NULL;

    if(cur(p)&&(cur(p)->kind==TK_ASSIGN||cur(p)->kind==TK_PLUS_ASSIGN||cur(p)->kind==TK_MINUS_ASSIGN)) {
        TokKind op=cur(p)->kind; adv(p);
        AstNode *val=parse_expr(p);
        if(op==TK_PLUS_ASSIGN)  { AstNode *cp=left; val=node_new(ND_BINARY,line); val->binop.op=strdup("+"); val->binop.left=cp; val->binop.right=parse_expr(p); }
        if(op==TK_PLUS_ASSIGN||op==TK_MINUS_ASSIGN) {
            AstNode *lhs_copy=left;
            AstNode *bin=node_new(ND_BINARY,line);
            bin->binop.op=strdup(op==TK_PLUS_ASSIGN?"+":"-");
            bin->binop.left=lhs_copy;
            bin->binop.right=val;
            val=bin;
        }
        AstNode *n=node_new(ND_ASSIGN,line);
        n->assign.target=left; n->assign.value=val; n->assign.var_type=NULL;
        return n;
    }
    return left;
}

static DecorVec parse_decorators(Parser *p) {
    DecorVec decs={0};
    while(cur(p)&&cur(p)->kind==TK_AT) {
        adv(p);
        Token *name=expect(p,TK_NAME,"Expected decorator name");
        Decorator d={0}; d.name=strdup(name?name->val:"?");
        if(cur(p)&&cur(p)->kind==TK_LPAREN) {
            adv(p);
            if(cur(p)&&cur(p)->kind==TK_STRING) { d.value=strdup(cur(p)->val?cur(p)->val:""); adv(p); }
            if(cur(p)&&cur(p)->kind==TK_RPAREN) adv(p);
        }
        VEC_PUSH(&decs,d);
        skip_newlines(p);
    }
    return decs;
}

static void parse_function(Parser *p, Program *prog, DecorVec decs) {
    bool is_extern=false;
    if(cur(p)&&cur(p)->kind==TK_EXTERN) { is_extern=true; adv(p); }
    expect(p,TK_DEF,"Expected 'def'/'fn'");
    Token *name=expect(p,TK_NAME,"Expected function name");

    FuncDef fd={0};
    fd.name=strdup(name?name->val:"?");
    fd.is_extern=is_extern;
    fd.decorators=decs;

    expect(p,TK_LPAREN,"Expected '('");
    while(cur(p)&&cur(p)->kind!=TK_RPAREN) {
        if(cur(p)->kind==TK_ELLIPSIS) { fd.is_vararg=true; adv(p); break; }
        Token *pn=expect(p,TK_NAME,"Expected param name");
        expect(p,TK_COLON,"Expected ':' after param name");
        char *pt=parse_type(p);
        Param param; param.name=strdup(pn?pn->val:"?"); param.type=pt;
        VEC_PUSH(&fd.params,param);
        if(cur(p)&&cur(p)->kind==TK_COMMA) adv(p);
    }
    expect(p,TK_RPAREN,"Expected ')'");

    if(cur(p)&&cur(p)->kind==TK_ARROW) { adv(p); fd.return_type=parse_type(p); }

    if(is_extern) { skip_newlines(p); VEC_PUSH(&prog->funcs,fd); return; }

    expect(p,TK_COLON,"Expected ':' after function signature");
    fd.body=parse_block(p);
    VEC_PUSH(&prog->funcs,fd);
}

static void parse_struct(Parser *p, Program *prog, DecorVec decs, bool is_enum) {
    adv(p);
    Token *name=expect(p,TK_NAME,"Expected struct/enum name");
    expect(p,TK_COLON,"Expected ':' after struct name");
    skip_newlines(p);
    expect(p,TK_INDENT,"Expected indent after struct ':'");

    StructDef sd={0};
    sd.name=strdup(name?name->val:"?");
    sd.decorators=decs;
    sd.is_enum=is_enum;

    while(cur(p)&&cur(p)->kind!=TK_DEDENT) {
        skip_newlines(p);
        if(!cur(p)||cur(p)->kind==TK_DEDENT) break;
        Token *fname=expect(p,TK_NAME,"Expected field name");
        char *ftype=NULL;
        char *fval=NULL;
        if(!is_enum) {
            expect(p,TK_COLON,"Expected ':' after field");
            ftype=parse_type(p);
        } else {
            ftype=strdup("int");
            if(cur(p)&&cur(p)->kind==TK_ASSIGN) {
                adv(p);
                AstNode *ev=parse_expr(p);
                if(ev) {
                    char buf[64];
                    if(ev->kind==ND_LITERAL_INT) snprintf(buf,sizeof(buf),"%lld",(long long)ev->ival);
                    else snprintf(buf,sizeof(buf),"0");
                    fval=strdup(buf);
                }
            }
        }
        StructField sf;
        sf.name=strdup(fname?fname->val:"?"); sf.type=ftype; sf.value=fval;
        VEC_PUSH(&sd.fields,sf);
        skip_newlines(p);
    }
    if(cur(p)&&cur(p)->kind==TK_DEDENT) adv(p);
    VEC_PUSH(&prog->structs,sd);
}

static Import parse_import(Parser *p) {
    adv(p);
    Import imp={0};
    Token *t=cur(p);
    if(t&&(t->kind==TK_STRING||t->kind==TK_NAME)) {
        imp.module_name=strdup(t->val?t->val:""); adv(p);
    }
    return imp;
}

static Import parse_from_import(Parser *p) {
    adv(p);
    Import imp={0};
    Token *mod=cur(p);
    if(mod&&(mod->kind==TK_STRING||mod->kind==TK_NAME)) {
        imp.module_name=strdup(mod->val?mod->val:""); adv(p);
    }
    expect(p,TK_IMPORT,"Expected 'import' after module name");
    int cap=8; imp.items=malloc(sizeof(char*)*cap); imp.n_items=0;
    while(1) {
        Token *it=expect(p,TK_NAME,"Expected import name");
        if(imp.n_items>=cap) { cap*=2; imp.items=realloc(imp.items,sizeof(char*)*cap); }
        imp.items[imp.n_items++]=strdup(it?it->val:"?");
        if(cur(p)&&cur(p)->kind==TK_COMMA){adv(p);continue;} break;
    }
    return imp;
}

Program *parse(Token *tokens, int count) {
    Parser _p={tokens,count,0}; Parser *p=&_p;
    Program *prog=program_new();
    skip_newlines(p);

    while(cur(p)&&(cur(p)->kind==TK_IMPORT||cur(p)->kind==TK_FROM)) {
        Import imp;
        if(cur(p)->kind==TK_IMPORT) imp=parse_import(p);
        else imp=parse_from_import(p);
        VEC_PUSH(&prog->imports,imp);
        skip_newlines(p);
    }

    while(cur(p)&&cur(p)->kind!=TK_EOF) {
        skip_newlines(p);
        if(!cur(p)||cur(p)->kind==TK_EOF) break;
        if(cur(p)->kind==TK_IMPORT){Import im=parse_import(p);VEC_PUSH(&prog->imports,im);continue;}
        if(cur(p)->kind==TK_FROM){Import im=parse_from_import(p);VEC_PUSH(&prog->imports,im);continue;}
        if(cur(p)->kind==TK_COMPTIME){
            adv(p); skip_newlines(p);
            if(cur(p)&&cur(p)->kind==TK_INDENT){adv(p);while(cur(p)&&cur(p)->kind!=TK_DEDENT)adv(p);if(cur(p))adv(p);}
            continue;
        }

        DecorVec decs={0};
        if(cur(p)&&cur(p)->kind==TK_AT) decs=parse_decorators(p);

        if(cur(p)&&cur(p)->kind==TK_STRUCT) { parse_struct(p,prog,decs,false); continue; }
        if(cur(p)&&cur(p)->kind==TK_ENUM)   { parse_struct(p,prog,decs,true);  continue; }
        if(cur(p)&&(cur(p)->kind==TK_DEF||cur(p)->kind==TK_EXTERN)) {
            parse_function(p,prog,decs); continue;
        }
        if(cur(p)&&(cur(p)->kind==TK_CONST||
            (cur(p)->kind==TK_NAME&&peek(p,1)&&peek(p,1)->kind==TK_COLON))) {
            bool is_const=false;
            if(cur(p)->kind==TK_CONST){is_const=true;adv(p);}
            Token *name=expect(p,TK_NAME,"Expected global var name");
            expect(p,TK_COLON,"Expected ':'");
            char *type=parse_type(p);
            AstNode *val=NULL;
            if(cur(p)&&cur(p)->kind==TK_ASSIGN){adv(p);val=parse_expr(p);}
            GlobalVar gv; gv.name=strdup(name?name->val:"?"); gv.type=type; gv.value=val; gv.is_const=is_const;
            VEC_PUSH(&prog->globals,gv);
            skip_newlines(p);
            continue;
        }
        skip_newlines(p);
        if(!cur(p)||cur(p)->kind==TK_EOF) break;
        adv(p);
    }
    return prog;
}

bool scan_postfix_for_assignment(Token *tokens, int count, int start_pos, int *out_end) {
    int pos = start_pos;
    if(out_end) *out_end = start_pos;
    while(pos < count) {
        Token *t = &tokens[pos];
        if(t->kind == TK_DOT && pos+1 < count && tokens[pos+1].kind == TK_NAME) {
            pos += 2; continue;
        }
        if(t->kind == TK_LBRACKET) {
            int depth = 1; pos++;
            while(pos < count && depth > 0) {
                if(tokens[pos].kind == TK_LBRACKET) depth++;
                else if(tokens[pos].kind == TK_RBRACKET) depth--;
                pos++;
            }
            continue;
        }
        break;
    }
    if(out_end) *out_end = pos;
    if(pos < count) {
        TokKind k = tokens[pos].kind;
        return k == TK_ASSIGN || k == TK_PLUS_ASSIGN || k == TK_MINUS_ASSIGN || k == TK_WALRUS;
    }
    return false;
}

AstNode *parse_atom_or_access_simple(Token *tokens, int count, int *pos) {
    if(!tokens || !pos || *pos >= count) return NULL;
    Parser p; p.toks = tokens; p.count = count; p.pos = *pos;
    AstNode *n = parse_atom(&p);
    *pos = p.pos;
    return n;
}

char *parse_type_str(Token *tokens, int count, int *pos) {
    if(!tokens || !pos || *pos >= count) return _cbl_strdup("void");
    Parser p; p.toks = tokens; p.count = count; p.pos = *pos;
    char *t = parse_type(&p);
    *pos = p.pos;
    return t;
}

Token *parser_current_token(Parser *p) { return cur(p); }

Token *parser_peek_token(Parser *p, int offset) { return peek(p, offset); }

Token *parser_advance(Parser *p) { return adv(p); }

GlobalVar *parser_parse_global_var(Parser *p, bool is_const) {
    if(is_const && cur(p) && cur(p)->kind == TK_CONST) adv(p);
    Token *name = expect(p, TK_NAME, "Expected global variable name");
    if(!expect(p, TK_COLON, "Expected ':' after global var name")) return NULL;
    char *type = parse_type(p);
    AstNode *val = NULL;
    if(cur(p) && cur(p)->kind == TK_ASSIGN) {
        adv(p);
        val = parse_expr(p);
    }
    GlobalVar *gv = calloc(1, sizeof(GlobalVar));
    if(!gv) { free(type); return NULL; }
    gv->name     = _cbl_strdup(name ? name->val : "?");
    gv->type     = type;
    gv->value    = val;
    gv->is_const = is_const;
    return gv;
}

StructDef *parser_parse_struct_def(Parser *p) {
    if(cur(p) && cur(p)->kind == TK_STRUCT) adv(p);
    Token *name = expect(p, TK_NAME, "Expected struct name");
    if(!expect(p, TK_COLON, "Expected ':' after struct name")) return NULL;
    skip_newlines(p);
    if(!expect(p, TK_INDENT, "Expected indent after struct ':'")) return NULL;

    StructDef *sd = calloc(1, sizeof(StructDef));
    if(!sd) return NULL;
    sd->name    = _cbl_strdup(name ? name->val : "?");
    sd->is_enum = false;

    while(cur(p) && cur(p)->kind != TK_DEDENT) {
        skip_newlines(p);
        if(!cur(p) || cur(p)->kind == TK_DEDENT) break;
        Token *fname = expect(p, TK_NAME, "Expected field name");
        if(!expect(p, TK_COLON, "Expected ':' after field name")) break;
        char *ftype = parse_type(p);
        StructField sf = {0};
        sf.name  = _cbl_strdup(fname ? fname->val : "?");
        sf.type  = ftype;
        sf.value = NULL;
        VEC_PUSH(&sd->fields, sf);
        skip_newlines(p);
    }
    if(cur(p) && cur(p)->kind == TK_DEDENT) adv(p);
    return sd;
}

StructDef *parser_parse_enum_def(Parser *p) {
    if(cur(p) && cur(p)->kind == TK_ENUM) adv(p);
    Token *name = expect(p, TK_NAME, "Expected enum name");
    if(!expect(p, TK_COLON, "Expected ':' after enum name")) return NULL;
    skip_newlines(p);
    if(!expect(p, TK_INDENT, "Expected indent after enum ':'")) return NULL;

    StructDef *sd = calloc(1, sizeof(StructDef));
    if(!sd) return NULL;
    sd->name    = _cbl_strdup(name ? name->val : "?");
    sd->is_enum = true;

    while(cur(p) && cur(p)->kind != TK_DEDENT) {
        skip_newlines(p);
        if(!cur(p) || cur(p)->kind == TK_DEDENT) break;
        Token *fname = expect(p, TK_NAME, "Expected enum member");
        char *val = NULL;
        if(cur(p) && cur(p)->kind == TK_ASSIGN) {
            adv(p);
            AstNode *ev = parse_expr(p);
            if(ev && ev->kind == ND_LITERAL_INT) {
                char buf[32]; snprintf(buf, sizeof(buf), "%lld", (long long)ev->ival);
                val = _cbl_strdup(buf);
            }
        }
        StructField sf = {0};
        sf.name  = _cbl_strdup(fname ? fname->val : "?");
        sf.type  = _cbl_strdup("int");
        sf.value = val;
        VEC_PUSH(&sd->fields, sf);
        skip_newlines(p);
    }
    if(cur(p) && cur(p)->kind == TK_DEDENT) adv(p);
    return sd;
}

void parser_parse_comptime(Parser *p) {
    if(cur(p) && cur(p)->kind == TK_COMPTIME) adv(p);
    if(cur(p) && cur(p)->kind == TK_COLON)    adv(p);
    skip_newlines(p);
    if(cur(p) && cur(p)->kind == TK_INDENT) {
        adv(p);
        while(cur(p) && cur(p)->kind != TK_DEDENT) adv(p);
        if(cur(p) && cur(p)->kind == TK_DEDENT) adv(p);
    }
}

AstNode *parser_parse_statement(Parser *p) { return parse_stmt(p); }

AstNode *parser_parse_return(Parser *p) {
    int line = cur(p) ? cur(p)->line : 0;
    if(cur(p) && cur(p)->kind == TK_RETURN) adv(p);
    AstNode *v = NULL;
    if(cur(p) && cur(p)->kind != TK_NEWLINE &&
       cur(p)->kind != TK_DEDENT && cur(p)->kind != TK_EOF)
        v = parse_expr(p);
    AstNode *n = node_new(ND_RETURN, line);
    n->ret.value = v; n->ret.is_endofcode = false;
    return n;
}

AstNode *parser_parse_var_decl(Parser *p) {
    int line = cur(p) ? cur(p)->line : 0;
    bool has_let = (cur(p) && cur(p)->kind == TK_LET);
    if(has_let) adv(p);
    Token *name = expect(p, TK_NAME, "Expected variable name");
    char *type = NULL;
    if(!has_let && cur(p) && cur(p)->kind == TK_COLON) {
        adv(p); type = parse_type(p);
    }
    AstNode *val = NULL;
    if(cur(p) && cur(p)->kind == TK_ASSIGN) { adv(p); val = parse_expr(p); }
    else { val = node_new(ND_LITERAL_INT, line); val->ival = 0; }
    AstNode *n = node_new(ND_ASSIGN, line);
    AstNode *var = node_new(ND_VARIABLE, line);
    var->sval = _cbl_strdup(name ? name->val : "?");
    n->assign.target   = var;
    n->assign.value    = val;
    n->assign.var_type = type;
    return n;
}

AstNode *parser_parse_if_stmt(Parser *p) {
    if(cur(p) && cur(p)->kind == TK_IF) adv(p);
    AstNode *cond = parse_expr(p);
    expect(p, TK_COLON, "Expected ':' after if condition");
    NodeVec then = parse_block(p);
    NodeVec els  = {0};
    if(cur(p) && cur(p)->kind == TK_ELSE) {
        adv(p);
        expect(p, TK_COLON, "Expected ':' after else");
        els = parse_block(p);
    }
    AstNode *n = node_new(ND_IF, cond ? cond->line : 0);
    n->ifst.cond = cond; n->ifst.then = then; n->ifst.els = els;
    return n;
}

AstNode *parser_parse_while_stmt(Parser *p) {
    int line = cur(p) ? cur(p)->line : 0;
    if(cur(p) && cur(p)->kind == TK_WHILE) adv(p);
    AstNode *cond = parse_expr(p);
    expect(p, TK_COLON, "Expected ':' after while");
    NodeVec body = parse_block(p);
    AstNode *n = node_new(ND_WHILE, line);
    n->whl.cond = cond; n->whl.body = body;
    return n;
}

AstNode *parser_parse_for_stmt(Parser *p) {
    if(cur(p) && cur(p)->kind == TK_FOR) adv(p);
    return parse_for(p);
}

AstNode *parser_parse_match_stmt(Parser *p) {

    return parse_match(p);
}

AstNode *parser_parse_expression(Parser *p) { return parse_expr(p); }
AstNode *parser_parse_logical_or(Parser *p) { return parse_or(p); }
AstNode *parser_parse_logical_and(Parser *p) { return parse_and(p); }
AstNode *parser_parse_logical_not(Parser *p) { return parse_not(p); }
AstNode *parser_parse_comparison(Parser *p) { return parse_compare(p); }
AstNode *parser_parse_additive(Parser *p) { return parse_add(p); }
AstNode *parser_parse_multiplicative(Parser *p) { return parse_mul(p); }
