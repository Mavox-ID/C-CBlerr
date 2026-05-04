/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#define _POSIX_C_SOURCE 200809L
#include "monomorphizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wstringop-truncation"

static char *_mdup(const char *s){
    if(!s)return NULL; size_t l=strlen(s)+1;
    char *r=malloc(l); if(r)memcpy(r,s,l); return r;
}

char *mono_stringify_type(const char *t){
    if(!t) return _mdup("void");
    size_t l=strlen(t);
    char *out=malloc(l+1); if(!out)return _mdup("void");
    for(size_t i=0;i<l;i++){
        char c=t[i];
        if(c=='<'||c=='>') out[i]='_';
        else                out[i]=c;
    }
    out[l]=0;
    return out;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static char *_type_c(const char *t){ return t?_mdup(t):_mdup("void"); }
#pragma GCC diagnostic pop

void mono_collect_placeholders_type(const char *t,
                                     char **acc, int *n_acc, int max_acc){
    if(!t||!acc||!n_acc) return;
    char buf[512]; strncpy(buf,t,sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    char *p=buf;
    while(*p){
        while(*p&&(*p=='*'||*p=='<'||*p=='>'||*p==','||*p==' ')) p++;
        if(!*p) break;
        /* I add a token */
        char tok[128]; int ti=0;
        while(*p&&*p!='*'&&*p!='<'&&*p!='>'&&*p!=','&&*p!=' '&&ti<127)
            tok[ti++]=*p++;
        tok[ti]=0;
        if(!tok[0]) continue;
        bool all_upper=true;
        for(int i=0;tok[i];i++) if(!isupper((unsigned char)tok[i])){all_upper=false;break;}
        if(!all_upper) continue;
        bool dup=false;
        for(int i=0;i<*n_acc;i++) if(strcmp(acc[i],tok)==0){dup=true;break;}
        if(!dup && *n_acc<max_acc){
            acc[*n_acc]=_mdup(tok);
            (*n_acc)++;
        }
    }
}

void mono_collect_placeholders_func(FuncDef *f,
                                     char **acc, int *n_acc, int max_acc){
    if(!f||!acc||!n_acc) return;
    for(int i=0;i<f->params.len;i++)
        mono_collect_placeholders_type(f->params.data[i].type,acc,n_acc,max_acc);
    mono_collect_placeholders_type(f->return_type,acc,n_acc,max_acc);
}

static char *_replace_type_in_str(const char *t,
                                    const char **keys,
                                    const char **vals,
                                    int          n){
    if(!t) return _mdup("void");
    char result[1024]; strncpy(result,t,sizeof(result)-1); result[sizeof(result)-1]=0;

    for(int i=0;i<n;i++){
        const char *k=keys[i];
        const char *v=vals[i];
        if(!k||!v) continue;
        char tmp[1024]; tmp[0]=0;
        const char *src=result;
        while(*src){
            const char *found=strstr(src,k);
            if(!found){ strncat(tmp,src,sizeof(tmp)-strlen(tmp)-1); break; }
            bool left_ok  = (found==result)||(!isalnum((unsigned char)*(found-1))&&*(found-1)!='_');
            bool right_ok = !isalnum((unsigned char)*(found+strlen(k)))&&*(found+strlen(k))!='_';
            if(left_ok&&right_ok){
                int pre=(int)(found-src);
                char prebuf[256]; if(pre>255)pre=255;
                memcpy(prebuf,src,(size_t)pre); prebuf[pre]=0;
                { size_t _tl=strlen(tmp),_pl=strlen(prebuf),_vl=strlen(v);
              if(_tl+_pl<sizeof(tmp)-1){memcpy(tmp+_tl,prebuf,_pl);tmp[_tl+_pl]=0;}
              _tl=strlen(tmp);
              if(_tl+_vl<sizeof(tmp)-1){memcpy(tmp+_tl,v,_vl);tmp[_tl+_vl]=0;} }
                src=found+strlen(k);
            } else {
                char onec[2]={(char)*src,0}; strncat(tmp,onec,sizeof(tmp)-strlen(tmp)-1);
                src++;
            }
        }
        result[sizeof(result)-1]=0;
        snprintf(result,sizeof(result),"%s",tmp);
    }
    return _mdup(result);
}

void mono_replace_types(AstNode *n,
                          const char **keys,
                          const char **vals,
                          int          nm){
    if(!n||nm==0) return;
    switch(n->kind){
    case ND_ASSIGN:
        if(n->assign.var_type){
            char *nt=_replace_type_in_str(n->assign.var_type,keys,vals,nm);
            free(n->assign.var_type); n->assign.var_type=nt;
        }
        mono_replace_types(n->assign.target,keys,vals,nm);
        mono_replace_types(n->assign.value, keys,vals,nm);
        break;
    case ND_RETURN:
        mono_replace_types(n->ret.value,keys,vals,nm); break;
    case ND_IF:
        mono_replace_types(n->ifst.cond,keys,vals,nm);
        for(int i=0;i<n->ifst.then.len;i++) mono_replace_types(n->ifst.then.data[i],keys,vals,nm);
        for(int i=0;i<n->ifst.els.len;i++)  mono_replace_types(n->ifst.els.data[i], keys,vals,nm);
        break;
    case ND_WHILE:
        mono_replace_types(n->whl.cond,keys,vals,nm);
        for(int i=0;i<n->whl.body.len;i++) mono_replace_types(n->whl.body.data[i],keys,vals,nm);
        break;
    case ND_FOR:
        mono_replace_types(n->forl.init,     keys,vals,nm);
        mono_replace_types(n->forl.cond,     keys,vals,nm);
        mono_replace_types(n->forl.post,     keys,vals,nm);
        mono_replace_types(n->forl.iter_expr,keys,vals,nm);
        for(int i=0;i<n->forl.body.len;i++) mono_replace_types(n->forl.body.data[i],keys,vals,nm);
        break;
    case ND_CALL:
        mono_replace_types(n->call.func,keys,vals,nm);
        for(int i=0;i<n->call.args.len;i++) mono_replace_types(n->call.args.data[i],keys,vals,nm);
        break;
    case ND_BINARY: case ND_COMPARE: case ND_LOGICAL: case ND_WALRUS:
        mono_replace_types(n->binop.left, keys,vals,nm);
        mono_replace_types(n->binop.right,keys,vals,nm);
        break;
    case ND_FIELD:
        mono_replace_types(n->fld.obj,keys,vals,nm); break;
    case ND_INDEX:
        mono_replace_types(n->idx.arr,keys,vals,nm);
        mono_replace_types(n->idx.idx,keys,vals,nm);
        break;
    case ND_DEREF: case ND_ADDR:
        mono_replace_types(n->inner,keys,vals,nm); break;
    case ND_CAST:
        mono_replace_types(n->cast.expr,keys,vals,nm);
        if(n->cast.target){
            char *nt=_replace_type_in_str(n->cast.target,keys,vals,nm);
            free(n->cast.target); n->cast.target=nt;
        }
        break;
    case ND_SIZEOF:
        if(n->szof.is_type&&n->szof.type_s){
            char *nt=_replace_type_in_str(n->szof.type_s,keys,vals,nm);
            free(n->szof.type_s); n->szof.type_s=nt;
        }
        break;
    case ND_ARRAY_LIT:
        if(n->arlit.elem_type){
            char *nt=_replace_type_in_str(n->arlit.elem_type,keys,vals,nm);
            free(n->arlit.elem_type); n->arlit.elem_type=nt;
        }
        for(int i=0;i<n->arlit.elems.len;i++)
            mono_replace_types(n->arlit.elems.data[i],keys,vals,nm);
        break;
    case ND_MATCH:
        mono_replace_types(n->match.expr,keys,vals,nm);
        for(int i=0;i<n->match.cases.len;i++){
            MatchCase *mc=&n->match.cases.data[i];
            for(int j=0;j<mc->vals.len;j++) mono_replace_types(mc->vals.data[j],keys,vals,nm);
            for(int j=0;j<mc->body.len;j++) mono_replace_types(mc->body.data[j],keys,vals,nm);
        }
        break;
    default: break;
    }
}

void mono_replace_types_body(NodeVec *body,
                               const char **keys,
                               const char **vals,
                               int          n){
    if(!body) return;
    for(int i=0;i<body->len;i++) mono_replace_types(body->data[i],keys,vals,n);
}

static FuncDef _clone_func(FuncDef *src,
                             const char *mangled_name,
                             const char **keys,
                             const char **vals,
                             int          nm){
    FuncDef f; memset(&f,0,sizeof(f));
    f.name         = _mdup(mangled_name);
    f.is_extern    = src->is_extern;
    f.is_vararg    = src->is_vararg;
    f.return_type  = _replace_type_in_str(src->return_type?src->return_type:"void",keys,vals,nm);
    f.decorators   = src->decorators;

    f.params.data = calloc((size_t)(src->params.len+1),sizeof(Param));
    f.params.cap  = src->params.len+1;
    for(int i=0;i<src->params.len;i++){
        Param *sp=&src->params.data[i];
        Param  dp;
        dp.name = _mdup(sp->name);
        dp.type = _replace_type_in_str(sp->type?sp->type:"int",keys,vals,nm);
        f.params.data[f.params.len++]=dp;
    }

    f.body.data = calloc((size_t)(src->body.len+1),sizeof(AstNode*));
    f.body.cap  = src->body.len+1;
    for(int i=0;i<src->body.len;i++)
        f.body.data[f.body.len++]=src->body.data[i];

    mono_replace_types_body(&f.body,keys,vals,nm);
    return f;
}

static StructDef _clone_struct(StructDef *src,
                                 const char *mangled_name,
                                 const char **keys,
                                 const char **vals,
                                 int          nm){
    StructDef sd; memset(&sd,0,sizeof(sd));
    sd.name       = _mdup(mangled_name);
    sd.is_enum    = src->is_enum;
    sd.decorators = src->decorators;

    sd.fields.data = calloc((size_t)(src->fields.len+1),sizeof(StructField));
    sd.fields.cap  = src->fields.len+1;
    for(int i=0;i<src->fields.len;i++){
        StructField *sf=&src->fields.data[i];
        StructField  df;
        df.name  = _mdup(sf->name);
        df.field = _mdup(sf->field);
        df.type  = _replace_type_in_str(sf->type?sf->type:"int",keys,vals,nm);
        df.value = _mdup(sf->value);
        sd.fields.data[sd.fields.len++]=df;
    }
    return sd;
}

static void _collect_type_args(AstNode *call_node,
                                 char **out_types,
                                 int   *n_out,
                                 int    max_out){
    *n_out=0;
    if(!call_node||call_node->kind!=ND_CALL) return;
    AstNode *fn=call_node->call.func;
    if(!fn||fn->kind!=ND_VARIABLE||!fn->sval) return;

    const char *lt=strchr(fn->sval,'<');
    if(!lt) return;
    const char *rt=strrchr(fn->sval,'>');
    if(!rt||rt<lt) return;

    int len=(int)(rt-lt-1);
    char inner[256]; if(len>255)len=255;
    memcpy(inner,lt+1,(size_t)len); inner[len]=0;

    char *tok=strtok(inner,",");
    while(tok&&*n_out<max_out){
        while(*tok==' ')tok++;
        char *end=tok+strlen(tok)-1;
        while(end>tok&&*end==' ')*end--=0;
        if(*tok) out_types[(*n_out)++]=_mdup(tok);
        tok=strtok(NULL,",");
    }
}

static void _rewrite_call_name(AstNode *call_node, const char *mangled){
    if(!call_node||call_node->kind!=ND_CALL) return;
    AstNode *fn=call_node->call.func;
    if(!fn||fn->kind!=ND_VARIABLE) return;
    free(fn->sval);
    fn->sval=_mdup(mangled);
}

static void _walk_rewrite(AstNode *n, Program *prog,
                            char **done_names, int *n_done);

static void _maybe_instantiate(AstNode *call_node, Program *prog,
                                 char **done_names, int *n_done){
    if(!call_node||call_node->kind!=ND_CALL) return;
    AstNode *fn=call_node->call.func;
    if(!fn||fn->kind!=ND_VARIABLE||!fn->sval) return;

    const char *lt=strchr(fn->sval,'<');
    if(!lt) return;

    char base[256]; int bl=(int)(lt-fn->sval); if(bl>255)bl=255;
    memcpy(base,fn->sval,(size_t)bl); base[bl]=0;

    char *type_args[32]; int n_args=0;
    _collect_type_args(call_node,type_args,&n_args,32);
    if(n_args==0){ /* clean up < > anyway ^-^ */
        free(fn->sval); fn->sval=_mdup(base); return;
    }

    FuncDef *src_func=NULL;
    for(int i=0;i<prog->funcs.len;i++){
        if(strcmp(prog->funcs.data[i].name,base)==0){
            src_func=&prog->funcs.data[i]; break;
        }
    }

    char *placeholders[32]; int n_ph=0;
    if(src_func)
        mono_collect_placeholders_func(src_func,placeholders,&n_ph,32);

    StructDef *src_struct=NULL;
    if(!src_func){
        for(int i=0;i<prog->structs.len;i++){
            char *sp_arr[32]; int sp_n=0;
            StructDef *sd=&prog->structs.data[i];
            if(strcmp(sd->name,base)!=0) continue;
            for(int j=0;j<sd->fields.len;j++)
                mono_collect_placeholders_type(sd->fields.data[j].type,sp_arr,&sp_n,32);
            if(sp_n>0){src_struct=sd; for(int k=0;k<sp_n;k++) placeholders[n_ph++]=sp_arr[k]; break;}
        }
    }

    if(n_ph==0||n_args==0){
        /* no generic params, just strip */
        free(fn->sval); fn->sval=_mdup(base);
        for(int i=0;i<n_args;i++) free(type_args[i]);
        return;
    }

    char mangled[512]; strncpy(mangled,base,sizeof(mangled)-1); mangled[sizeof(mangled)-1]=0;
    for(int i=0;i<n_args&&i<n_ph;i++){
        char *ms=mono_stringify_type(type_args[i]);
        strncat(mangled,"_",sizeof(mangled)-strlen(mangled)-1);
        strncat(mangled,ms,sizeof(mangled)-strlen(mangled)-1);
        free(ms);
    }

    bool already=false;
    for(int i=0;i<*n_done;i++) if(strcmp(done_names[i],mangled)==0){already=true;break;}

    if(!already){
        int map_n=n_args<n_ph?n_args:n_ph;
        const char **mkeys=(map_n>0)?calloc((size_t)map_n,sizeof(char*)):NULL;
        const char **mvals=(map_n>0)?calloc((size_t)map_n,sizeof(char*)):NULL;
        for(int i=0;i<map_n;i++){mkeys[i]=placeholders[i]; mvals[i]=type_args[i];}

        if(src_func){
            FuncDef inst=_clone_func(src_func,mangled,mkeys,mvals,map_n);
            VEC_PUSH(&prog->funcs,inst);
        }
        if(src_struct){
            StructDef inst=_clone_struct(src_struct,mangled,mkeys,mvals,map_n);
            VEC_PUSH(&prog->structs,inst);
        }

        free(mkeys); free(mvals);

        if(*n_done<4096) done_names[(*n_done)++]=_mdup(mangled);
    }

    _rewrite_call_name(call_node,mangled);

    for(int i=0;i<n_args;i++) free(type_args[i]);
}

static void _walk_rewrite(AstNode *n, Program *prog,
                            char **done_names, int *n_done){
    if(!n) return;
    if(n->kind==ND_CALL){
        _maybe_instantiate(n,prog,done_names,n_done);
        for(int i=0;i<n->call.args.len;i++)
            _walk_rewrite(n->call.args.data[i],prog,done_names,n_done);
        return;
    }
    switch(n->kind){
    case ND_BINARY: case ND_COMPARE: case ND_LOGICAL: case ND_WALRUS:
        _walk_rewrite(n->binop.left, prog,done_names,n_done);
        _walk_rewrite(n->binop.right,prog,done_names,n_done);
        break;
    case ND_ASSIGN:
        _walk_rewrite(n->assign.target,prog,done_names,n_done);
        _walk_rewrite(n->assign.value, prog,done_names,n_done);
        break;
    case ND_RETURN:
        _walk_rewrite(n->ret.value,prog,done_names,n_done); break;
    case ND_IF:
        _walk_rewrite(n->ifst.cond,prog,done_names,n_done);
        for(int i=0;i<n->ifst.then.len;i++) _walk_rewrite(n->ifst.then.data[i],prog,done_names,n_done);
        for(int i=0;i<n->ifst.els.len;i++)  _walk_rewrite(n->ifst.els.data[i], prog,done_names,n_done);
        break;
    case ND_WHILE:
        _walk_rewrite(n->whl.cond,prog,done_names,n_done);
        for(int i=0;i<n->whl.body.len;i++) _walk_rewrite(n->whl.body.data[i],prog,done_names,n_done);
        break;
    case ND_FOR:
        _walk_rewrite(n->forl.init,     prog,done_names,n_done);
        _walk_rewrite(n->forl.cond,     prog,done_names,n_done);
        _walk_rewrite(n->forl.post,     prog,done_names,n_done);
        _walk_rewrite(n->forl.iter_expr,prog,done_names,n_done);
        for(int i=0;i<n->forl.body.len;i++) _walk_rewrite(n->forl.body.data[i],prog,done_names,n_done);
        break;
    case ND_FIELD:
        _walk_rewrite(n->fld.obj,prog,done_names,n_done); break;
    case ND_INDEX:
        _walk_rewrite(n->idx.arr,prog,done_names,n_done);
        _walk_rewrite(n->idx.idx,prog,done_names,n_done);
        break;
    case ND_DEREF: case ND_ADDR:
        _walk_rewrite(n->inner,prog,done_names,n_done); break;
    case ND_CAST:
        _walk_rewrite(n->cast.expr,prog,done_names,n_done); break;
    case ND_ARRAY_LIT:
        for(int i=0;i<n->arlit.elems.len;i++)
            _walk_rewrite(n->arlit.elems.data[i],prog,done_names,n_done);
        break;
    case ND_MATCH:
        _walk_rewrite(n->match.expr,prog,done_names,n_done);
        for(int i=0;i<n->match.cases.len;i++){
            MatchCase *mc=&n->match.cases.data[i];
            for(int j=0;j<mc->vals.len;j++) _walk_rewrite(mc->vals.data[j],prog,done_names,n_done);
            for(int j=0;j<mc->body.len;j++) _walk_rewrite(mc->body.data[j],prog,done_names,n_done);
        }
        break;
    default: break;
    }
}

void monomorphize(Program *prog){
    if(!prog) return;

    char **done_names=calloc(4096,sizeof(char*));
    int    n_done=0;

    int orig_len=prog->funcs.len;
    for(int fi=0;fi<orig_len;fi++){
        FuncDef *f=&prog->funcs.data[fi];
        for(int i=0;i<f->body.len;i++)
            _walk_rewrite(f->body.data[i],prog,done_names,&n_done);
    }

    for(int i=0;i<prog->globals.len;i++)
        _walk_rewrite(prog->globals.data[i].value,prog,done_names,&n_done);

    int new_len=0;
    for(int i=0;i<prog->funcs.len;i++){
        FuncDef *f=&prog->funcs.data[i];
        char *ph[32]; int n_ph=0;
        mono_collect_placeholders_func(f,ph,&n_ph,32);
        bool keep=true;
        if(n_ph>0){
            bool has_instance=false;
            for(int j=0;j<n_done;j++){
                size_t nl=strlen(f->name?f->name:"");
                if(nl>0&&done_names[j]&&
                   strncmp(done_names[j],f->name,nl)==0&&
                   done_names[j][nl]=='_'){
                    has_instance=true; break;
                }
            }
            if(has_instance) keep=false;
            for(int j=0;j<n_ph;j++) free(ph[j]);
        }
        if(keep) prog->funcs.data[new_len++]=*f; /* Remove it ... */
    }
    prog->funcs.len=new_len;

    int new_slen=0;
    for(int i=0;i<prog->structs.len;i++){
        StructDef *sd=&prog->structs.data[i];
        char *ph[32]; int n_ph=0;
        for(int j=0;j<sd->fields.len;j++)
            mono_collect_placeholders_type(sd->fields.data[j].type,ph,&n_ph,32);
        bool keep=true;
        if(n_ph>0){
            bool has_instance=false;
            size_t nl=strlen(sd->name?sd->name:"");
            for(int j=0;j<n_done;j++){
                if(nl>0&&done_names[j]&&
                   strncmp(done_names[j],sd->name,nl)==0&&
                   done_names[j][nl]=='_'){
                    has_instance=true; break;
                }
            }
            if(has_instance) keep=false;
            for(int j=0;j<n_ph;j++) free(ph[j]);
        }
        if(keep) prog->structs.data[new_slen++]=*sd;
    }
    prog->structs.len=new_slen;

    /* Cln */
    for(int i=0;i<n_done;i++) free(done_names[i]);
    free(done_names);
}

FuncDef *mono_find_function(Program *prog, const char *name) {
    if(!prog || !name) return NULL;
    for(int i=0; i<prog->funcs.len; i++)
        if(prog->funcs.data[i].name && strcmp(prog->funcs.data[i].name,name)==0)
            return &prog->funcs.data[i];
    return NULL;
}

StructDef *mono_find_struct(Program *prog, const char *name) {
    if(!prog || !name) return NULL;
    for(int i=0; i<prog->structs.len; i++)
        if(prog->structs.data[i].name && strcmp(prog->structs.data[i].name,name)==0)
            return &prog->structs.data[i];
    return NULL;
}

char *mono_resolve_type(const char *t, const char **keys, const char **vals, int n) {
    if(!t) return _mdup("void");
    return _replace_type_in_str(t, keys, vals, n);
}

void mono_visit(Program *prog, AstNode *node,
                char **done_names, int *n_done) {
    if(!node || !prog) return;
    _walk_rewrite(node, prog, done_names, n_done);
}

void mono_visit_function(Program *prog, FuncDef *f,
                          char **done_names, int *n_done) {
    if(!f || !prog) return;
    for(int i = 0; i < f->body.len; i++)
        mono_visit(prog, f->body.data[i], done_names, n_done);
}

void mono_visit_program(Program *prog) {
    if(!prog) return;
    char **done_names = calloc(4096, sizeof(char*));
    int    n_done     = 0;
    int    orig_len   = prog->funcs.len;
    for(int i = 0; i < orig_len; i++)
        mono_visit_function(prog, &prog->funcs.data[i], done_names, &n_done);
    for(int i = 0; i < n_done; i++) free(done_names[i]);
    free(done_names);
}
