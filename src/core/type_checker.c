/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#define _POSIX_C_SOURCE 200809L
#include "type_checker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

static char *_tc_dup(const char *s){
    if(!s) return NULL;
    size_t l=strlen(s)+1;
    char *r=malloc(l);
    if(r){ memcpy(r,s,l); }
    return r;
}

char *type_to_str(const char *t){
    if(!t||strcmp(t,"void")==0) return _tc_dup("void");
    return _tc_dup(t);
}

bool type_is_int_family(const char *t){
    if(!t) return false;
    return strcmp(t,"int")==0||strcmp(t,"i8")==0||strcmp(t,"i16")==0||
           strcmp(t,"i32")==0||strcmp(t,"i64")==0||strcmp(t,"int32")==0||
           strcmp(t,"int64")==0||strcmp(t,"u8")==0||strcmp(t,"u16")==0||
           strcmp(t,"u32")==0||strcmp(t,"u64")==0;
}

static unsigned _sym_hash(const char *s){
    unsigned h=5381; while(*s){ h=h*33^(unsigned char)*s; s++; } return h;
}

void symtab_init(SymTab *s){ memset(s,0,sizeof(SymTab)); }
void symtab_free(SymTab *s){
    for(int i=0;i<SYMTAB_CAP;i++){
        if(s->entries[i].key){ free(s->entries[i].key); free(s->entries[i].val); }
    }
    memset(s,0,sizeof(SymTab));
}
void symtab_set(SymTab *s, const char *key, const char *val){
    if(!key) return;
    unsigned h=_sym_hash(key)%SYMTAB_CAP;
    for(int i=0;i<SYMTAB_CAP;i++){
        int idx=(h+i)%SYMTAB_CAP;
        if(!s->entries[idx].key||strcmp(s->entries[idx].key,key)==0){
            if(!s->entries[idx].key){ s->entries[idx].key=_tc_dup(key); s->count++; }
            else{ free(s->entries[idx].val); }
            s->entries[idx].val=_tc_dup(val?val:"void");
            return;
        }
    }
}
const char *symtab_get(SymTab *s, const char *key){
    if(!key) return NULL;
    unsigned h=_sym_hash(key)%SYMTAB_CAP;
    for(int i=0;i<SYMTAB_CAP;i++){
        int idx=(h+i)%SYMTAB_CAP;
        if(!s->entries[idx].key) return NULL;
        if(strcmp(s->entries[idx].key,key)==0) return s->entries[idx].val;
    }
    return NULL;
}
void symtab_copy(SymTab *dst, const SymTab *src){
    symtab_free(dst); symtab_init(dst);
    for(int i=0;i<SYMTAB_CAP;i++)
        if(src->entries[i].key)
            symtab_set(dst,src->entries[i].key,src->entries[i].val);
}

void funcsig_init(FuncSigTab *t){ memset(t,0,sizeof(FuncSigTab)); }
void funcsig_free(FuncSigTab *t){
    for(int i=0;i<t->count;i++){
        FuncSig *s=&t->entries[i];
        free(s->name); free(s->return_type);
        for(int j=0;j<s->n_params;j++) free(s->param_types[j]);
        free(s->param_types);
    }
    memset(t,0,sizeof(FuncSigTab));
}
void funcsig_add(FuncSigTab *t, const char *name,
                  char **params, int n_params,
                  const char *ret, bool is_vararg){
    if(t->count>=FUNCSIG_CAP) return;
    for(int i=0;i<t->count;i++){
        if(strcmp(t->entries[i].name,name)==0){
            return;
        }
    }
    FuncSig *s=&t->entries[t->count++];
    s->name=_tc_dup(name);
    s->return_type=_tc_dup(ret?ret:"void");
    s->is_vararg=is_vararg;
    s->n_params=n_params;
    s->param_types=calloc((size_t)(n_params+1),sizeof(char*));
    for(int i=0;i<n_params;i++) s->param_types[i]=_tc_dup(params[i]);
}
FuncSig *funcsig_get(FuncSigTab *t, const char *name){
    for(int i=0;i<t->count;i++)
        if(strcmp(t->entries[i].name,name)==0) return &t->entries[i];
    return NULL;
}

void sftab_init(StructFieldTab *t){ memset(t,0,sizeof(StructFieldTab)); }
void sftab_free(StructFieldTab *t){
    for(int i=0;i<t->count;i++){
        StructFields *sf=&t->entries[i];
        free(sf->struct_name);
        for(int j=0;j<sf->n_fields;j++){
            free(sf->field_names[j]); free(sf->field_types[j]);
        }
        free(sf->field_names); free(sf->field_types);
    }
    memset(t,0,sizeof(StructFieldTab));
}
void sftab_add(StructFieldTab *t, const char *sname,
                char **fnames, char **ftypes, int n){
    if(t->count>=STRUCTFIELDS_CAP) return;
    StructFields *sf=&t->entries[t->count++];
    sf->struct_name=_tc_dup(sname);
    sf->n_fields=n;
    sf->field_names=calloc((size_t)(n+1),sizeof(char*));
    sf->field_types=calloc((size_t)(n+1),sizeof(char*));
    for(int i=0;i<n;i++){
        sf->field_names[i]=_tc_dup(fnames[i]);
        sf->field_types[i]=_tc_dup(ftypes[i]);
    }
}
StructFields *sftab_get(StructFieldTab *t, const char *sname){
    for(int i=0;i<t->count;i++)
        if(strcmp(t->entries[i].struct_name,sname)==0) return &t->entries[i];
    return NULL;
}

static const char *RESERVED_FUNCS[] = {
    "printf","malloc","free","exit","memcpy","memset",
    "puts","putchar","scanf","calloc","realloc","memmove","memcmp",
    "strlen","strcmp","strncmp","sprintf","snprintf","fopen","fclose",
    "fread","fwrite","fseek","ftell","rewind","fflush",
    NULL
};
static bool is_reserved(const char *name){
    for(int i=0;RESERVED_FUNCS[i];i++)
        if(strcmp(name,RESERVED_FUNCS[i])==0) return true;
    return false;
}

void tc_init(TypeChecker *tc, GameDebugger *dbg){
    memset(tc,0,sizeof(TypeChecker));
    tc->debugger=dbg?dbg:debugger_get();
    sftab_init(&tc->struct_fields);
    funcsig_init(&tc->functions);
    symtab_init(&tc->globals);
    symtab_set(&tc->globals,"true", "bool");
    symtab_set(&tc->globals,"false","bool");
    symtab_set(&tc->globals,"NULL", "*void");
    tc->had_error=false;
    tc->current_return_type=NULL;
}
void tc_free(TypeChecker *tc){
    sftab_free(&tc->struct_fields);
    funcsig_free(&tc->functions);
    symtab_free(&tc->globals);
    free(tc->current_return_type);
    tc->current_return_type=NULL;
}

static bool tc_error(TypeChecker *tc, const char *fmt, ...){
    va_list ap; va_start(ap,fmt);
    vsnprintf(tc->error_msg,sizeof(tc->error_msg),fmt,ap);
    va_end(ap);
    debugger_log_error(tc->debugger,"%s",tc->error_msg);
    tc->had_error=true;
    return false;
}

typedef struct { char *names[1024]; int count; } CallSet;
static void callset_add(CallSet *cs, const char *name){
    for(int i=0;i<cs->count;i++) if(strcmp(cs->names[i],name)==0) return;
    if(cs->count<1024) cs->names[cs->count++]=_tc_dup(name);
}
static void callset_free(CallSet *cs){
    for(int i=0;i<cs->count;i++) free(cs->names[i]);
    cs->count=0;
}

static void _collect_calls_node(AstNode *n, CallSet *cs){
    if(!n) return;
    if(n->kind==ND_CALL){
        AstNode *fn=n->call.func;
        if(fn&&fn->kind==ND_VARIABLE&&fn->sval)
            callset_add(cs,fn->sval);
        else if(fn&&fn->kind==ND_CALL)
            _collect_calls_node(fn,cs);
        for(int i=0;i<n->call.args.len;i++)
            _collect_calls_node(n->call.args.data[i],cs);
        return;
    }
    switch(n->kind){
    case ND_BINARY: case ND_COMPARE: case ND_LOGICAL: case ND_WALRUS:
        _collect_calls_node(n->binop.left,cs);
        _collect_calls_node(n->binop.right,cs);
        break;
    case ND_ASSIGN:
        _collect_calls_node(n->assign.target,cs);
        _collect_calls_node(n->assign.value,cs);
        break;
    case ND_RETURN:
        _collect_calls_node(n->ret.value,cs);
        break;
    case ND_IF:
        _collect_calls_node(n->ifst.cond,cs);
        for(int i=0;i<n->ifst.then.len;i++) _collect_calls_node(n->ifst.then.data[i],cs);
        for(int i=0;i<n->ifst.els.len;i++)  _collect_calls_node(n->ifst.els.data[i],cs);
        break;
    case ND_WHILE:
        _collect_calls_node(n->whl.cond,cs);
        for(int i=0;i<n->whl.body.len;i++) _collect_calls_node(n->whl.body.data[i],cs);
        break;
    case ND_FOR:
        _collect_calls_node(n->forl.init,cs);
        _collect_calls_node(n->forl.cond,cs);
        _collect_calls_node(n->forl.post,cs);
        _collect_calls_node(n->forl.iter_expr,cs);
        for(int i=0;i<n->forl.body.len;i++) _collect_calls_node(n->forl.body.data[i],cs);
        break;
    case ND_FIELD:  _collect_calls_node(n->fld.obj,cs); break;
    case ND_INDEX:  _collect_calls_node(n->idx.arr,cs); _collect_calls_node(n->idx.idx,cs); break;
    case ND_DEREF: case ND_ADDR: _collect_calls_node(n->inner,cs); break;
    case ND_CAST:  _collect_calls_node(n->cast.expr,cs); break;
    case ND_ARRAY_LIT:
        for(int i=0;i<n->arlit.elems.len;i++) _collect_calls_node(n->arlit.elems.data[i],cs);
        break;
    case ND_MATCH:
        _collect_calls_node(n->match.expr,cs);
        for(int i=0;i<n->match.cases.len;i++){
            MatchCase *mc=&n->match.cases.data[i];
            for(int j=0;j<mc->vals.len;j++) _collect_calls_node(mc->vals.data[j],cs);
            for(int j=0;j<mc->body.len;j++) _collect_calls_node(mc->body.data[j],cs);
        }
        break;
    default: break;
    }
}

bool tc_check_expr(TypeChecker *tc, AstNode *expr,
                    SymTab *symbols, OriginTab *origins,
                    char *out_type, int out_sz){
    if(!expr){ snprintf(out_type,out_sz,"void"); return true; }

    switch(expr->kind){
    case ND_LITERAL_INT:
        snprintf(out_type,out_sz,"int");  return true;
    case ND_LITERAL_FLOAT:
        snprintf(out_type,out_sz,"float"); return true;
    case ND_LITERAL_STR:
        snprintf(out_type,out_sz,"str");  return true;
    case ND_LITERAL_BOOL:
        snprintf(out_type,out_sz,"bool"); return true;

    case ND_VARIABLE: {
        const char *v=symtab_get(symbols,expr->sval);
        if(!v) v=symtab_get(&tc->globals,expr->sval);
        if(!v){
            FuncSig *fs=funcsig_get(&tc->functions,expr->sval);
            if(fs){
                char fns[512]="fn("; bool first=true;
                for(int i=0;i<fs->n_params;i++){
                    if(!first){ strncat(fns,",",sizeof(fns)-strlen(fns)-1); }
                    first=false;
                    strncat(fns,fs->param_types[i],sizeof(fns)-strlen(fns)-1);
                }
                strncat(fns,")->",sizeof(fns)-strlen(fns)-1);
                strncat(fns,fs->return_type,sizeof(fns)-strlen(fns)-1);
                snprintf(out_type,out_sz,"%s",fns);
                return true;
            }
            return tc_error(tc,"Undefined variable '%s'",expr->sval);
        }
        snprintf(out_type,out_sz,"%s",v);
        return true;
    }

    case ND_FIELD: {
        char obj_t[256]={0};
        if(!tc_check_expr(tc,expr->fld.obj,symbols,origins,obj_t,sizeof(obj_t))) return false;
        if(strcmp(obj_t,"str")==0){
            if(strcmp(expr->fld.field,"data")==0)  { snprintf(out_type,out_sz,"*void"); return true; }
            if(strcmp(expr->fld.field,"length")==0){ snprintf(out_type,out_sz,"i32");   return true; }
        }
        char sname[256]; strncpy(sname,obj_t,sizeof(sname)-1); sname[sizeof(sname)-1]=0;
        if(sname[0]=='*') memmove(sname,sname+1,strlen(sname));
        if(strncmp(sname,"struct ",7)==0) memmove(sname,sname+7,strlen(sname)-6);
        StructFields *sf=sftab_get(&tc->struct_fields,sname);
        if(!sf) return tc_error(tc,"Field access on non-struct type: %s",obj_t);
        for(int i=0;i<sf->n_fields;i++){
            if(strcmp(sf->field_names[i],expr->fld.field)==0){
                snprintf(out_type,out_sz,"%s",sf->field_types[i]);
                return true;
            }
        }
        return tc_error(tc,"Field '%s' not found in struct '%s'",expr->fld.field,sname);
    }

    case ND_INDEX: {
        char arr_t[256]={0}; char idx_t[256]={0};
        if(!tc_check_expr(tc,expr->idx.arr,symbols,origins,arr_t,sizeof(arr_t))) return false;
        if(!tc_check_expr(tc,expr->idx.idx,symbols,origins,idx_t,sizeof(idx_t))) return false;
        if(arr_t[0]=='*'){ snprintf(out_type,out_sz,"%s",arr_t+1); return true; }
        if(strncmp(arr_t,"array<",6)==0){
            char inner[256]; strncpy(inner,arr_t+6,sizeof(inner)-1); inner[sizeof(inner)-1]=0;
            char *gt=strrchr(inner,'>'); if(gt)*gt=0;
            snprintf(out_type,out_sz,"%s",inner); return true;
        }
        snprintf(out_type,out_sz,"void"); return true;
    }

    case ND_CALL: {
        AstNode *fn=expr->call.func;
        const char *fname=NULL;
        if(fn&&fn->kind==ND_VARIABLE) fname=fn->sval;

        if(fname&&strcmp(fname,"print")==0){
            for(int i=0;i<expr->call.args.len;i++){
                char at[256]={0};
                tc_check_expr(tc,expr->call.args.data[i],symbols,origins,at,sizeof(at));
            }
            snprintf(out_type,out_sz,"void"); return true;
        }
        if(fname&&strcmp(fname,"range")==0){
            for(int i=0;i<expr->call.args.len;i++){
                char at[256]={0};
                tc_check_expr(tc,expr->call.args.data[i],symbols,origins,at,sizeof(at));
            }
            snprintf(out_type,out_sz,"range"); return true;
        }
        if(fname&&strcmp(fname,"len")==0){
            if(expr->call.args.len!=1)
                return tc_error(tc,"len() expects a single argument");
            char at[256]={0};
            tc_check_expr(tc,expr->call.args.data[0],symbols,origins,at,sizeof(at));
            snprintf(out_type,out_sz,"int"); return true;
        }

        if(fname){
            FuncSig *sig=funcsig_get(&tc->functions,fname);
            if(!sig) return tc_error(tc,"Call to unknown function '%s'",fname);

            for(int i=0;i<expr->call.args.len;i++){
                char at[256]={0};
                tc_check_expr(tc,expr->call.args.data[i],symbols,origins,at,sizeof(at));
                if(i<sig->n_params && sig->param_types[i]){
                    const char *expected=sig->param_types[i];
                    if(strcmp(at,"str")==0&&expected[0]=='*'){
                        AstNode *a=expr->call.args.data[i];
                        bool ok=(a->kind==ND_FIELD&&strcmp(a->fld.field,"data")==0)||
                                (a->kind==ND_CAST&&a->cast.expr&&
                                 a->cast.expr->kind==ND_FIELD&&
                                 strcmp(a->cast.expr->fld.field,"data")==0);
                        if(!ok)
                            return tc_error(tc,
                                "Passing `str` directly to '%s'; use `<str>.data as *void`",fname);
                    }
                    if(strcmp(at,expected)!=0){
                        if(!(type_is_int_family(at)&&type_is_int_family(expected)))
                            tc_error(tc,"Arg type mismatch in '%s': got %s, want %s",
                                     fname,at,expected);
                    }
                }
            }
            snprintf(out_type,out_sz,"%s",sig->return_type?sig->return_type:"void");
            return true;
        }

        char callee_t[256]={0};
        if(!tc_check_expr(tc,fn,symbols,origins,callee_t,sizeof(callee_t))) return false;
        if(strncmp(callee_t,"fn(",3)==0||strncmp(callee_t,"*fn(",4)==0){
            const char *p=callee_t[0]=='*'?callee_t+1:callee_t;
            const char *arrow=strstr(p,"->");
            snprintf(out_type,out_sz,"%s",arrow?arrow+2:"void");
            return true;
        }
        return tc_error(tc,"Call on non-function type: %s",callee_t);
    }

    case ND_ARRAY_LIT: {
        if(expr->arlit.elems.len==0){
            snprintf(out_type,out_sz,"array<void>"); return true;
        }
        char elem_t[256]={0};
        tc_check_expr(tc,expr->arlit.elems.data[0],symbols,origins,elem_t,sizeof(elem_t));
        for(int i=1;i<expr->arlit.elems.len;i++){
            char t[256]={0};
            tc_check_expr(tc,expr->arlit.elems.data[i],symbols,origins,t,sizeof(t));
            if(strcmp(t,elem_t)!=0&&!type_is_int_family(t)&&!type_is_int_family(elem_t))
                tc_error(tc,"Array literal element type mismatch: %s != %s",t,elem_t);
        }
        snprintf(out_type,out_sz,"array<%s>",elem_t);
        return true;
    }

    case ND_CAST: {
        char inner_t[256]={0};
        tc_check_expr(tc,expr->cast.expr,symbols,origins,inner_t,sizeof(inner_t));
        snprintf(out_type,out_sz,"%s",expr->cast.target?expr->cast.target:"void");
        return true;
    }

    case ND_ADDR: {
        char inner_t[256]={0};
        tc_check_expr(tc,expr->inner,symbols,origins,inner_t,sizeof(inner_t));
        snprintf(out_type,out_sz,"*%s",inner_t);
        return true;
    }

    case ND_DEREF: {
        char inner_t[256]={0};
        tc_check_expr(tc,expr->inner,symbols,origins,inner_t,sizeof(inner_t));
        if(inner_t[0]=='*') snprintf(out_type,out_sz,"%s",inner_t+1);
        else                 snprintf(out_type,out_sz,"%s",inner_t);
        return true;
    }

    case ND_SIZEOF:
        snprintf(out_type,out_sz,"int"); return true;

    case ND_WALRUS: {
        char rt[256]={0};
        tc_check_expr(tc,expr->binop.right,symbols,origins,rt,sizeof(rt));
        snprintf(out_type,out_sz,"%s",rt); return true;
    }

    case ND_BINARY: {
        char lt[256]={0}, rt[256]={0};
        tc_check_expr(tc,expr->binop.left, symbols,origins,lt,sizeof(lt));
        tc_check_expr(tc,expr->binop.right,symbols,origins,rt,sizeof(rt));
        if(strcmp(lt,rt)!=0&&!type_is_int_family(lt)&&!type_is_int_family(rt))
            tc_error(tc,"Binary op type mismatch: %s %s %s",lt,expr->binop.op,rt);
        if(strcmp(expr->binop.op,"+")==0&&strcmp(lt,"str")==0&&strcmp(rt,"str")==0){
            snprintf(out_type,out_sz,"str"); return true;
        }
        snprintf(out_type,out_sz,"%s",lt[0]?lt:"int");
        return true;
    }
    case ND_COMPARE: {
        char lt[256]={0}, rt[256]={0};
        tc_check_expr(tc,expr->binop.left, symbols,origins,lt,sizeof(lt));
        tc_check_expr(tc,expr->binop.right,symbols,origins,rt,sizeof(rt));
        if((strncmp(lt,"f",1)==0||strncmp(rt,"f",1)==0)&&
           strcmp(lt,"false")!=0&&strcmp(rt,"false")!=0&&
           strcmp(lt,"float")!=0&&strcmp(rt,"float")!=0)
            tc_error(tc,"Direct float comparison not supported; convert to int");
        snprintf(out_type,out_sz,"bool"); return true;
    }
    case ND_LOGICAL: {
        if(strcmp(expr->binop.op,"not")==0){
            char lt[256]={0};
            tc_check_expr(tc,expr->binop.left,symbols,origins,lt,sizeof(lt));
            if(strcmp(lt,"int")!=0&&strcmp(lt,"bool")!=0)
                tc_error(tc,"'not' requires bool/int, got %s",lt);
            snprintf(out_type,out_sz,"bool"); return true;
        }
        char lt[256]={0}, rt[256]={0};
        tc_check_expr(tc,expr->binop.left, symbols,origins,lt,sizeof(lt));
        tc_check_expr(tc,expr->binop.right,symbols,origins,rt,sizeof(rt));
        if((strcmp(lt,"int")!=0&&strcmp(lt,"bool")!=0)||
           (strcmp(rt,"int")!=0&&strcmp(rt,"bool")!=0))
            tc_error(tc,"'%s' requires bool/int operands",expr->binop.op);
        snprintf(out_type,out_sz,"bool"); return true;
    }

    case ND_INLINE_ASM:
        snprintf(out_type,out_sz,"void"); return true;

    default:
        snprintf(out_type,out_sz,"void"); return true;
    }
}

bool tc_check_stmt(TypeChecker *tc, AstNode *stmt,
                    SymTab *symbols, OriginTab *origins,
                    const char *func_ret){
    if(!stmt) return true;
    switch(stmt->kind){
    
    case ND_ASSIGN: {
        char val_t[256]={0};
        if(!tc_check_expr(tc,stmt->assign.value,symbols,origins,val_t,sizeof(val_t)))
            return tc->had_error?false:true;

        const char *declared=stmt->assign.var_type;
        if(declared){
            if(strcmp(declared,val_t)!=0&&
               !(type_is_int_family(declared)&&type_is_int_family(val_t))){
                bool suppress = false;
                if(strncmp(val_t,"array<",6)==0)     suppress=true;
                if(strcmp(val_t,"void")==0)          suppress=true;
                if(declared[0]=='*'&&type_is_int_family(val_t)) suppress=true;
                if(strcmp(val_t,"str")==0&&declared[0]=='*')    suppress=true;
                if(!suppress)
                    debugger_log_warning(tc->debugger,
                        "Type hint mismatch: declared '%s', got '%s'",declared,val_t);
            }
        }
        if(stmt->assign.target&&stmt->assign.target->kind==ND_VARIABLE){
            const char *vname=stmt->assign.target->sval;
            symtab_set(symbols,vname,declared?declared:val_t);
            const char *orig="local";
            if(stmt->assign.value){
                AstNode *v=stmt->assign.value;
                if(v->kind==ND_CALL&&v->call.func&&v->call.func->kind==ND_VARIABLE){
                    const char *fn=v->call.func->sval;
                    if(fn&&(strcmp(fn,"malloc")==0||strcmp(fn,"calloc")==0))
                        orig="heap";
                } else if(v->kind==ND_ARRAY_LIT){
                    orig="stack_array";
                } else if(v->kind==ND_ADDR){
                    orig="stack_ptr";
                }
            }
            symtab_set(origins,vname,orig);
        }
        return true;
    }

    case ND_RETURN: {
        char val_t[256]="void";
        if(stmt->ret.value){
            AstNode *rv=stmt->ret.value;
            if(rv->kind==ND_ADDR){
                AstNode *inner=rv->inner;
                if(inner&&inner->kind==ND_ARRAY_LIT)
                    return tc_error(tc,"Returning address of local array literal is forbidden; use malloc");
                if(inner&&inner->kind==ND_VARIABLE){
                    const char *org=symtab_get(origins,inner->sval);
                    if(org&&strcmp(org,"heap")!=0)
                        return tc_error(tc,"Returning address of local variable '%s' is forbidden; use malloc",inner->sval);
                }
            }
            if(rv->kind==ND_VARIABLE){
                const char *org=symtab_get(origins,rv->sval);
                if(org&&(strcmp(org,"stack_ptr")==0||strcmp(org,"stack_array")==0))
                    return tc_error(tc,"Returning stack-allocated pointer '%s' is forbidden",rv->sval);
            }
            if(!tc_check_expr(tc,stmt->ret.value,symbols,origins,val_t,sizeof(val_t)))
                return false;
        }
        if(func_ret){
            if(strcmp(func_ret,"void")==0){
                if(strcmp(val_t,"void")!=0)
                    tc_error(tc,"void function cannot return a value of type %s",val_t);
            } else {
                if(strcmp(val_t,"void")==0&&!stmt->ret.is_endofcode)
                    tc_error(tc,"Function returns %s but return has no value",func_ret);
                else if(strcmp(val_t,func_ret)!=0&&
                        !(type_is_int_family(val_t)&&type_is_int_family(func_ret)))
                    tc_error(tc,"Return type mismatch: expected %s, got %s",func_ret,val_t);
            }
        }
        return true;
    }

    case ND_IF: {
        char ct[256]={0};
        tc_check_expr(tc,stmt->ifst.cond,symbols,origins,ct,sizeof(ct));
        if(strcmp(ct,"int")!=0&&strcmp(ct,"bool")!=0)
            tc_error(tc,"If condition must be bool/int, got %s",ct);
        for(int i=0;i<stmt->ifst.then.len;i++)
            tc_check_stmt(tc,stmt->ifst.then.data[i],symbols,origins,func_ret);
        for(int i=0;i<stmt->ifst.els.len;i++)
            tc_check_stmt(tc,stmt->ifst.els.data[i],symbols,origins,func_ret);
        return true;
    }

    case ND_WHILE: {
        char ct[256]={0};
        tc_check_expr(tc,stmt->whl.cond,symbols,origins,ct,sizeof(ct));
        if(strcmp(ct,"int")!=0&&strcmp(ct,"bool")!=0)
            tc_error(tc,"While condition must be bool/int, got %s",ct);
        for(int i=0;i<stmt->whl.body.len;i++)
            tc_check_stmt(tc,stmt->whl.body.data[i],symbols,origins,func_ret);
        return true;
    }

    case ND_FOR: {
        SymTab inner_sym; symtab_copy(&inner_sym,symbols);
        OriginTab inner_orig; symtab_copy(&inner_orig,origins);
        if(stmt->forl.init) tc_check_stmt(tc,stmt->forl.init,&inner_sym,&inner_orig,func_ret);
        if(stmt->forl.cond){
            char ct[256]={0};
            tc_check_expr(tc,stmt->forl.cond,&inner_sym,&inner_orig,ct,sizeof(ct));
            if(strcmp(ct,"int")!=0&&strcmp(ct,"bool")!=0)
                tc_error(tc,"For condition must be bool/int, got %s",ct);
        }
        if(stmt->forl.post) tc_check_expr(tc,stmt->forl.post,&inner_sym,&inner_orig,NULL,0);
        if(stmt->forl.iter_var){
            char it[256]="int";
            if(stmt->forl.iter_expr){
                char et[256]={0};
                tc_check_expr(tc,stmt->forl.iter_expr,&inner_sym,&inner_orig,et,sizeof(et));
                if(strncmp(et,"array<",6)==0){
                    char inner[256]; strncpy(inner,et+6,sizeof(inner)-1); inner[sizeof(inner)-1]=0;
                    char *gt=strrchr(inner,'>'); if(gt)*gt=0;
                    snprintf(it,sizeof(it),"%s",inner);
                } else if(et[0]=='*'){ strncpy(it,et+1,sizeof(it)-1); }
            }
            symtab_set(&inner_sym,stmt->forl.iter_var,it);
            symtab_set(&inner_orig,stmt->forl.iter_var,"local");
        }
        for(int i=0;i<stmt->forl.body.len;i++)
            tc_check_stmt(tc,stmt->forl.body.data[i],&inner_sym,&inner_orig,func_ret);
        symtab_free(&inner_sym); symtab_free(&inner_orig);
        return true;
    }

    case ND_MATCH: {
        char et[256]={0};
        tc_check_expr(tc,stmt->match.expr,symbols,origins,et,sizeof(et));
        for(int i=0;i<stmt->match.cases.len;i++){
            MatchCase *mc=&stmt->match.cases.data[i];
            for(int j=0;j<mc->vals.len;j++){
                char vt[256]={0};
                tc_check_expr(tc,mc->vals.data[j],symbols,origins,vt,sizeof(vt));
                if(strcmp(vt,et)!=0&&!type_is_int_family(vt)&&!type_is_int_family(et))
                    tc_error(tc,"Match case type %s != match expression type %s",vt,et);
            }
            for(int j=0;j<mc->body.len;j++)
                tc_check_stmt(tc,mc->body.data[j],symbols,origins,func_ret);
        }
        return true;
    }

    case ND_BREAK: case ND_CONTINUE: return true;

    case ND_INLINE_ASM: return true;

    default: {
        char t[256]={0};
        tc_check_expr(tc,stmt,symbols,origins,t,sizeof(t));
        return true;
    }
    }
}

static bool tc_check_function(TypeChecker *tc, FuncDef *f){
    SymTab symbols; symtab_init(&symbols);
    OriginTab origins; symtab_init(&origins);

    for(int i=0;i<f->params.len;i++){
        Param *p=&f->params.data[i];
        symtab_set(&symbols,p->name,p->type?p->type:"int");
        symtab_set(&origins,p->name,"param");
    }

    const char *ret=f->return_type?f->return_type:"void";

    for(int i=0;i<f->body.len;i++)
        tc_check_stmt(tc,f->body.data[i],&symbols,&origins,ret);

    symtab_free(&symbols); symtab_free(&origins);
    return !tc->had_error;
}

bool tc_check(TypeChecker *tc, Program *prog){
    for(int i=0;i<prog->structs.len;i++){
        StructDef *sd=&prog->structs.data[i];
        if(sd->is_enum) continue;
        char **fnames=calloc((size_t)(sd->fields.len+1),sizeof(char*));
        char **ftypes=calloc((size_t)(sd->fields.len+1),sizeof(char*));
        for(int j=0;j<sd->fields.len;j++){
            fnames[j]=sd->fields.data[j].name;
            ftypes[j]=sd->fields.data[j].type?sd->fields.data[j].type:"int";
        }
        sftab_add(&tc->struct_fields,sd->name,fnames,ftypes,sd->fields.len);
        free(fnames); free(ftypes);
    }

    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *f=&prog->funcs.data[i];
        char **ptypes=calloc((size_t)(f->params.len+1),sizeof(char*));
        for(int j=0;j<f->params.len;j++)
            ptypes[j]=f->params.data[j].type?f->params.data[j].type:"int";
        funcsig_add(&tc->functions,f->name,ptypes,f->params.len,
                    f->return_type?f->return_type:"void",f->is_vararg);
        free(ptypes);
    }

    for(int i=0;i<prog->globals.len;i++){
        GlobalVar *g=&prog->globals.data[i];
        symtab_set(&tc->globals,g->name,g->type?g->type:"int");
        if(g->value){
            AstNode *v=g->value;
            bool is_simple=(v->kind==ND_LITERAL_INT||v->kind==ND_LITERAL_FLOAT||
                            v->kind==ND_LITERAL_STR ||v->kind==ND_LITERAL_BOOL||
                            v->kind==ND_ARRAY_LIT   ||v->kind==ND_VARIABLE);
            if(!is_simple)
                debugger_log_warning(tc->debugger,
                    "Global '%s': complex initializer will be moved to CblerrInitGlobals()",
                    g->name);
        }
    }

    bool found_non_extern=false;
    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *f=&prog->funcs.data[i];
        if(f->is_extern&&found_non_extern){
            tc_error(tc,"extern declarations must be placed at the top of the file");
            break;
        }
        if(!f->is_extern) found_non_extern=true;
    }

    {
        int main_idx=-1, last_non_extern=-1;
        for(int i=0;i<prog->funcs.len;i++){
            FuncDef *f=&prog->funcs.data[i];
            if(!f->is_extern){
                last_non_extern=i;
                if(strcmp(f->name,"main")==0) main_idx=i;
            }
        }
        if(main_idx>=0&&main_idx!=last_non_extern)
            tc_error(tc,"main() MUST ALWAYS be the last function in the file");
    }

    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *caller=&prog->funcs.data[i];
        if(caller->is_extern) continue;
        CallSet cs; memset(&cs,0,sizeof(cs));
        for(int j=0;j<caller->body.len;j++)
            _collect_calls_node(caller->body.data[j],&cs);
        for(int ci=0;ci<cs.count;ci++){
            const char *callee_name=cs.names[ci];
            for(int k=0;k<prog->funcs.len;k++){
                if(strcmp(prog->funcs.data[k].name,callee_name)==0&&k>i&&!prog->funcs.data[k].is_extern){
                    tc_error(tc,"Function '%s' must be defined before it is called by '%s' (Top-Down Rule)",
                             callee_name,caller->name);
                }
            }
        }
        callset_free(&cs);
    }

    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *f=&prog->funcs.data[i];
        if(!f->is_extern&&is_reserved(f->name))
            tc_error(tc,"Redefinition of reserved function '%s'; declare as extern",f->name);
    }

    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *f=&prog->funcs.data[i];
        if(f->is_extern) continue;
        const char *rt=f->return_type?f->return_type:"void";
        if(type_is_int_family(rt)){
            bool has_return=false;
            if(f->body.len>0){
                AstNode *last=f->body.data[f->body.len-1];
                if(last&&last->kind==ND_RETURN) has_return=true;
            }
            if(!has_return)
                debugger_log_warning(tc->debugger,
                    "Function '%s' returns '%s' but may not have explicit return",f->name,rt);
        }
    }
    
    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *f=&prog->funcs.data[i];
        if(f->is_extern) continue;
        tc_check_function(tc,f);
    }

    return !tc->had_error;
}
