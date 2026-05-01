#include "codegen.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
static inline char *_cbl_strdup(const char *s) {
    if(!s) return NULL;
    size_t l = strlen(s)+1; char *r = malloc(l);
    if(r) { memcpy(r,s,l); }
    return r;
}
#define strdup _cbl_strdup

static void cg_ensure(CodeGen *cg, int extra) {
    if(cg->len+extra+1 >= cg->cap) {
        cg->cap = cg->cap ? cg->cap*2+extra : 65536+extra;
        cg->buf = realloc(cg->buf, (size_t)cg->cap);
    }
}
static void cg_raw(CodeGen *cg, const char *s) {
    int l=(int)strlen(s);
    cg_ensure(cg,l);
    memcpy(cg->buf+cg->len,s,l);
    cg->len+=l;
    cg->buf[cg->len]=0;
}
static void cg_line(CodeGen *cg, const char *s) {
    for(int i=0;i<cg->indent;i++) cg_raw(cg,"    ");
    cg_raw(cg,s);
    cg_raw(cg,"\n");
}
static void cg_linef(CodeGen *cg, const char *fmt, ...) {
    char buf[4096]; va_list ap; va_start(ap,fmt); vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap);
    cg_line(cg, buf);
}

static const char *ctype(const char *flux) {
    if(!flux||strcmp(flux,"void")==0) return "void";
    if(strcmp(flux,"int")==0||strcmp(flux,"i32")==0||strcmp(flux,"int32")==0) return "int32_t";
    if(strcmp(flux,"i64")==0||strcmp(flux,"int64")==0) return "int64_t";
    if(strcmp(flux,"i16")==0) return "int16_t";
    if(strcmp(flux,"i8")==0)  return "int8_t";
    if(strcmp(flux,"u8")==0)  return "uint8_t";
    if(strcmp(flux,"u16")==0) return "uint16_t";
    if(strcmp(flux,"u32")==0) return "uint32_t";
    if(strcmp(flux,"u64")==0) return "uint64_t";
    if(strcmp(flux,"float")==0||strcmp(flux,"f32")==0) return "float";
    if(strcmp(flux,"f64")==0||strcmp(flux,"double")==0) return "double";
    if(strcmp(flux,"bool")==0) return "bool";
    if(strcmp(flux,"str")==0) return "flux_string";
    return NULL;
}

static char *ctype_str(const char *flux) {
    if(!flux) return strdup("void");
    if(flux[0]=='*') {
        char *inner=ctype_str(flux+1);
        char *r=malloc(strlen(inner)+2); r[0]=0; strcat(r,inner); strcat(r,"*");
        free(inner); return r;
    }
    if(strncmp(flux,"ptr<",4)==0) {
        char *inner=ctype_str(flux+4);
        size_t l=strlen(inner); if(l>0&&inner[l-1]=='>') inner[l-1]=0;
        char *r=malloc(strlen(inner)+2); strcpy(r,inner); strcat(r,"*");
        free(inner); return r;
    }
    if(strncmp(flux,"array<",6)==0) {
        char tmp[256]; strncpy(tmp,flux+6,sizeof(tmp)-1); tmp[sizeof(tmp)-1]=0;
        size_t l=strlen(tmp); if(l>0&&tmp[l-1]=='>') tmp[l-1]=0;
        char *inner=ctype_str(tmp);
        char *r=malloc(strlen(inner)+2); strcpy(r,inner); strcat(r,"*");
        free(inner); return r;
    }
    const char *m=ctype(flux);
    if(m) return strdup(m);
    char buf[256]; snprintf(buf,sizeof(buf),"struct %s",flux);
    return strdup(buf);
}

static void escape_str(const char *s, char *out, int outmax) {
    int oi=0;
    for(;*s&&oi<outmax-4;s++) {
        unsigned char c=(unsigned char)*s;
        if(c=='\\')      { out[oi++]='\\'; out[oi++]='\\'; }
        else if(c=='"')  { out[oi++]='\\'; out[oi++]='"';  }
        else if(c=='\n') { out[oi++]='\\'; out[oi++]='n';  }
        else if(c=='\r') { out[oi++]='\\'; out[oi++]='r';  }
        else if(c=='\t') { out[oi++]='\\'; out[oi++]='t';  }
        else if(c==0)    { out[oi++]='\\'; out[oi++]='0';  }
        else              out[oi++]=(char)c;
    }
    out[oi]=0;
}

static char *gen_expr(CodeGen *cg, AstNode *n);

static char *gen_print(CodeGen *cg, AstNode *n) {
    if(!n->call.args.data||n->call.args.len==0) return strdup("printf(\"\\n\")");
    char fmt[4096]=""; char args_part[4096]=""; bool has_args=false;
    for(int i=0;i<n->call.args.len;i++) {
        AstNode *a=n->call.args.data[i];
        if(a->kind==ND_LITERAL_STR) {
            char esc[2048]; escape_str(a->sval,esc,sizeof(esc));
            strncat(fmt,esc,sizeof(fmt)-strlen(fmt)-1);
        } else {
            strncat(fmt,"%d",sizeof(fmt)-strlen(fmt)-1);
            char *ac=gen_expr(cg,a);
            if(has_args) strncat(args_part,", ",sizeof(args_part)-strlen(args_part)-1);
            strncat(args_part,ac,sizeof(args_part)-strlen(args_part)-1);
            free(ac); has_args=true;
        }
    }
    if(fmt[0]==0||fmt[strlen(fmt)-1]!='n') strncat(fmt,"\\n",sizeof(fmt)-strlen(fmt)-1);
    char buf[8192];
    if(has_args) snprintf(buf,sizeof(buf),"printf(\"%s\", %s)",fmt,args_part);
    else snprintf(buf,sizeof(buf),"printf(\"%s\")",fmt);
    return strdup(buf);
}

static char *gen_expr(CodeGen *cg, AstNode *n) {
    if(!n) return strdup("0");
    char tmp[8192]; char *l=NULL,*r=NULL;
    switch(n->kind) {
    case ND_LITERAL_INT:
        snprintf(tmp,sizeof(tmp),"%lld",(long long)n->ival);
        return strdup(tmp);
    case ND_LITERAL_FLOAT:
        snprintf(tmp,sizeof(tmp),"%g",n->fval);
        return strdup(tmp);
    case ND_LITERAL_STR: {
        char esc[4096]; escape_str(n->sval,esc,sizeof(esc));
        snprintf(tmp,sizeof(tmp),"(flux_string){\"%s\", %d}",esc,(int)strlen(n->sval));
        return strdup(tmp); }
    case ND_LITERAL_BOOL:
        return strdup(n->ival?"true":"false");
    case ND_VARIABLE:
        return strdup(n->sval?n->sval:"?");
    case ND_BINARY:
        l=gen_expr(cg,n->binop.left); r=gen_expr(cg,n->binop.right);
        if(strcmp(n->binop.op,"**")==0) { snprintf(tmp,sizeof(tmp),"pow(%s, %s)",l,r); }
        else snprintf(tmp,sizeof(tmp),"(%s %s %s)",l,n->binop.op,r);
        free(l);free(r); return strdup(tmp);
    case ND_COMPARE:
        l=gen_expr(cg,n->binop.left); r=gen_expr(cg,n->binop.right);
        snprintf(tmp,sizeof(tmp),"(%s %s %s)",l,n->binop.op,r);
        free(l);free(r); return strdup(tmp);
    case ND_LOGICAL:
        if(strcmp(n->binop.op,"not")==0) {
            l=gen_expr(cg,n->binop.left); snprintf(tmp,sizeof(tmp),"(!%s)",l); free(l);
        } else {
            l=gen_expr(cg,n->binop.left); r=gen_expr(cg,n->binop.right);
            const char *op=strcmp(n->binop.op,"and")==0?"&&":"||";
            snprintf(tmp,sizeof(tmp),"(%s %s %s)",l,op,r);
            free(l);free(r);
        }
        return strdup(tmp);
    case ND_CALL: {
        /* special built-ins */
        AstNode *fn=n->call.func;
        const char *fname=NULL;
        if(fn&&fn->kind==ND_VARIABLE) fname=fn->sval;
        if(fname&&strcmp(fname,"print")==0) return gen_print(cg,n);
        if(fname&&strcmp(fname,"len")==0&&n->call.args.len==1) {
            char *a=gen_expr(cg,n->call.args.data[0]);
            snprintf(tmp,sizeof(tmp),"(sizeof(%s)/sizeof(%s[0]))",a,a);
            free(a); return strdup(tmp);
        }
        char *fn_s=gen_expr(cg,fn);
        char args_buf[4096]="";
        for(int i=0;i<n->call.args.len;i++) {
            if(i>0) strncat(args_buf,", ",sizeof(args_buf)-strlen(args_buf)-1);
            char *a=gen_expr(cg,n->call.args.data[i]);
            strncat(args_buf,a,sizeof(args_buf)-strlen(args_buf)-1);
            free(a);
        }
        snprintf(tmp,sizeof(tmp),"%s(%s)",fn_s,args_buf);
        free(fn_s); return strdup(tmp);
    }
    case ND_FIELD:
        l=gen_expr(cg,n->fld.obj);
        snprintf(tmp,sizeof(tmp),"%s.%s",l,n->fld.field);
        free(l); return strdup(tmp);
    case ND_INDEX:
        l=gen_expr(cg,n->idx.arr); r=gen_expr(cg,n->idx.idx);
        snprintf(tmp,sizeof(tmp),"%s[%s]",l,r);
        free(l);free(r); return strdup(tmp);
    case ND_ARRAY_LIT: {
        char buf[4096]; buf[0]=0;
        for(int i=0;i<n->arlit.elems.len;i++) {
            if(i>0) strncat(buf,", ",sizeof(buf)-strlen(buf)-1);
            char *e=gen_expr(cg,n->arlit.elems.data[i]);
            strncat(buf,e,sizeof(buf)-strlen(buf)-1); free(e);
        }
        if(n->arlit.is_struct_init) snprintf(tmp,sizeof(tmp),"{%s}",buf);
        else {
            const char *et=n->arlit.elem_type?n->arlit.elem_type:"int32_t";
            char *ets=ctype_str(et);
            snprintf(tmp,sizeof(tmp),"(%s[]){%s}",ets,buf); free(ets);
        }
        return strdup(tmp);
    }
    case ND_DEREF:
        l=gen_expr(cg,n->inner); snprintf(tmp,sizeof(tmp),"(*%s)",l); free(l); return strdup(tmp);
    case ND_ADDR:
        l=gen_expr(cg,n->inner); snprintf(tmp,sizeof(tmp),"(&%s)",l); free(l); return strdup(tmp);
    case ND_CAST: {
        char *ct=ctype_str(n->cast.target); l=gen_expr(cg,n->cast.expr);
        snprintf(tmp,sizeof(tmp),"((%s)%s)",ct,l);
        free(ct);free(l); return strdup(tmp);
    }
    case ND_SIZEOF:
        if(n->szof.is_type) {
            char *ct=ctype_str(n->szof.type_s);
            snprintf(tmp,sizeof(tmp),"sizeof(%s)",ct); free(ct);
        } else {
            l=gen_expr(cg,n->szof.expr);
            snprintf(tmp,sizeof(tmp),"sizeof(%s)",l); free(l);
        }
        return strdup(tmp);
    case ND_WALRUS:
        l=gen_expr(cg,n->binop.left); r=gen_expr(cg,n->binop.right);
        snprintf(tmp,sizeof(tmp),"(%s = %s)",l,r);
        free(l);free(r); return strdup(tmp);
    default: return strdup("0");
    }
}

static void gen_stmt(CodeGen *cg, AstNode *n);

static void gen_stmt(CodeGen *cg, AstNode *n) {
    if(!n) return;
    char *e=NULL;
    switch(n->kind) {
    case ND_RETURN:
        if(n->ret.value) {
            e=gen_expr(cg,n->ret.value);
            cg_linef(cg,"return %s;",e); free(e);
        } else cg_line(cg,"return;");
        break;
    case ND_ASSIGN: {
        AstNode *tgt=n->assign.target;
        char *val=gen_expr(cg,n->assign.value);
        if(n->assign.var_type) {
            char *ct=ctype_str(n->assign.var_type);
            if(tgt&&tgt->kind==ND_VARIABLE)
                cg_linef(cg,"%s %s = %s;",ct,tgt->sval?tgt->sval:"?",val);
            else { char *ts=gen_expr(cg,tgt); cg_linef(cg,"%s %s = %s;",ct,ts,val); free(ts); }
            free(ct);
        } else {
            char *ts=gen_expr(cg,tgt);
            cg_linef(cg,"%s = %s;",ts,val); free(ts);
        }
        free(val); break;
    }
    case ND_IF: {
        char *cond=gen_expr(cg,n->ifst.cond);
        cg_linef(cg,"if (%s) {",cond); free(cond);
        cg->indent++;
        for(int i=0;i<n->ifst.then.len;i++) gen_stmt(cg,n->ifst.then.data[i]);
        cg->indent--;
        if(n->ifst.els.len>0) {
            cg_line(cg,"} else {");
            cg->indent++;
            for(int i=0;i<n->ifst.els.len;i++) gen_stmt(cg,n->ifst.els.data[i]);
            cg->indent--;
        }
        cg_line(cg,"}"); break;
    }
    case ND_WHILE: {
        char *cond=gen_expr(cg,n->whl.cond);
        cg_linef(cg,"while (%s) {",cond); free(cond);
        cg->indent++;
        for(int i=0;i<n->whl.body.len;i++) gen_stmt(cg,n->whl.body.data[i]);
        cg->indent--;
        cg_line(cg,"}"); break;
    }
    case ND_FOR: {
        if(n->forl.init||n->forl.cond||n->forl.post) {
            char init_buf[256]="", cond_buf[256]="1", post_buf[256]="";
            if(n->forl.init) {
                AstNode *ini=n->forl.init;
                if(ini->kind==ND_ASSIGN&&ini->assign.var_type) {
                    char *ct=ctype_str(ini->assign.var_type);
                    char *vn=gen_expr(cg,ini->assign.target);
                    char *vv=gen_expr(cg,ini->assign.value);
                    snprintf(init_buf,sizeof(init_buf),"%s %s = %s",ct,vn,vv);
                    free(ct);free(vn);free(vv);
                } else { char *ie=gen_expr(cg,ini); snprintf(init_buf,sizeof(init_buf),"%s",ie); free(ie); }
            }
            if(n->forl.cond) { char *ce=gen_expr(cg,n->forl.cond); snprintf(cond_buf,sizeof(cond_buf),"%s",ce); free(ce); }
            if(n->forl.post) { char *pe=gen_expr(cg,n->forl.post); snprintf(post_buf,sizeof(post_buf),"%s",pe); free(pe); }
            cg_linef(cg,"for (%s; %s; %s) {",init_buf,cond_buf,post_buf);
        } else if(n->forl.iter_var) {
            AstNode *ie=n->forl.iter_expr;
            if(ie&&ie->kind==ND_CALL&&ie->call.func&&ie->call.func->kind==ND_VARIABLE
               &&strcmp(ie->call.func->sval,"range")==0&&ie->call.args.len>=2) {
                char *s=gen_expr(cg,ie->call.args.data[0]);
                char *e2=gen_expr(cg,ie->call.args.data[1]);
                cg_linef(cg,"for (int32_t %s = %s; %s < %s; ++%s) {",
                         n->forl.iter_var,s,n->forl.iter_var,e2,n->forl.iter_var);
                free(s);free(e2);
            } else {
                char *arr=gen_expr(cg,ie);
                cg_linef(cg,"for (int __idx_%s=0; __idx_%s<(int)(sizeof(%s)/sizeof(%s[0])); ++__idx_%s) {",
                         n->forl.iter_var,n->forl.iter_var,arr,arr,n->forl.iter_var);
                cg_linef(cg,"    int32_t %s = %s[__idx_%s];",n->forl.iter_var,arr,n->forl.iter_var);
                free(arr);
            }
        } else {
            cg_line(cg,"while(1) {");
        }
        cg->indent++;
        for(int i=0;i<n->forl.body.len;i++) gen_stmt(cg,n->forl.body.data[i]);
        cg->indent--;
        cg_line(cg,"}"); break;
    }
    case ND_BREAK:    cg_line(cg,"break;"); break;
    case ND_CONTINUE: cg_line(cg,"continue;"); break;
    case ND_INLINE_ASM:
        cg_linef(cg,"__asm__(\"%s\");",n->asm_code?n->asm_code:""); break;
    case ND_MATCH: {
        char *me=gen_expr(cg,n->match.expr);
        bool first=true;
        for(int i=0;i<n->match.cases.len;i++) {
            MatchCase *mc=&n->match.cases.data[i];
            if(mc->vals.len==0) {
                if(first) cg_line(cg,"if (1) {"); else cg_line(cg,"else {");
            } else {
                char cond_buf[4096]="";
                for(int j=0;j<mc->vals.len;j++) {
                    if(j>0) strncat(cond_buf," || ",sizeof(cond_buf)-strlen(cond_buf)-1);
                    char *v=gen_expr(cg,mc->vals.data[j]);
                    char part[512]; snprintf(part,sizeof(part),"(%s == %s)",me,v);
                    strncat(cond_buf,part,sizeof(cond_buf)-strlen(cond_buf)-1); free(v);
                }
                if(first) cg_linef(cg,"if (%s) {",cond_buf);
                else cg_linef(cg,"else if (%s) {",cond_buf);
            }
            cg->indent++;
            for(int j=0;j<mc->body.len;j++) gen_stmt(cg,mc->body.data[j]);
            cg->indent--;
            cg_line(cg,"}"); first=false;
        }
        free(me); break;
    }
    default: {
        char *ex=gen_expr(cg,n);
        if(ex&&strcmp(ex,"0")!=0) cg_linef(cg,"%s;",ex);
        free(ex); break;
    }
    }
}

static void gen_func_sig(CodeGen *cg, FuncDef *f, char *out, int out_sz) {
    char *rt=ctype_str(f->return_type?f->return_type:"void");
    char params[2048]="";
    if(f->params.len==0&&!f->is_vararg) strncpy(params,"void",sizeof(params)-1);
    for(int i=0;i<f->params.len;i++) {
        if(i>0) strncat(params,", ",sizeof(params)-strlen(params)-1);
        char *pt=ctype_str(f->params.data[i].type?f->params.data[i].type:"int");
        char part[256]; snprintf(part,sizeof(part),"%s %s",pt,f->params.data[i].name?f->params.data[i].name:"_");
        strncat(params,part,sizeof(params)-strlen(params)-1); free(pt);
    }
    if(f->is_vararg) {
        if(f->params.len>0) strncat(params,", ...",sizeof(params)-strlen(params)-1);
        else strncpy(params,"...",sizeof(params)-1);
    }

    const char *call_conv="";
#ifdef _WIN32
    if(f->is_extern) {
        static const char *crt[]={
            "malloc","calloc","realloc","free","memset","memcpy","memmove",
            "printf","sprintf","puts","putchar","scanf","exit","fopen",
            "fgetc","feof","fclose","fputc","system","wsprintfA","memmove",NULL};
        bool is_crt=false;
        for(int i=0;crt[i];i++) if(f->name&&strcmp(f->name,crt[i])==0){is_crt=true;break;}
        if(!is_crt) call_conv="__stdcall ";
    }
    if(f->name&&(strcmp(f->name,"WinMain")==0||strcmp(f->name,"WindowProc")==0))
        call_conv="__stdcall ";
#endif
    snprintf(out,out_sz,"%s %s%s(%s)",rt,call_conv,f->name?f->name:"?",params);
    free(rt);
}

char *codegen_generate(Program *prog) {
    CodeGen _cg={0}; CodeGen *cg=&_cg;

    cg_line(cg,"typedef signed char int8_t;");
    cg_line(cg,"typedef short int16_t;");
    cg_line(cg,"typedef int int32_t;");
    cg_line(cg,"typedef long long int64_t;");
    cg_line(cg,"typedef unsigned char uint8_t;");
    cg_line(cg,"typedef unsigned short uint16_t;");
    cg_line(cg,"typedef unsigned int uint32_t;");
    cg_line(cg,"typedef unsigned long long uint64_t;");
    cg_line(cg,"#if defined(__GNUC__)||defined(__clang__)");
    cg_line(cg,"typedef __SIZE_TYPE__ size_t;");
    cg_line(cg,"#elif defined(_WIN64)");
    cg_line(cg,"typedef unsigned long long size_t;");
    cg_line(cg,"#else");
    cg_line(cg,"typedef unsigned int size_t;");
    cg_line(cg,"#endif");
    cg_line(cg,"#define bool _Bool");
    cg_line(cg,"#define true 1");
    cg_line(cg,"#define false 0");
    cg_line(cg,"#define NULL ((void*)0)");
    cg_line(cg,"typedef struct { const char* data; int64_t length; } flux_string;");
    cg_line(cg,"extern int memcmp(const void*, const void*, size_t);");
    cg_line(cg,"static inline bool flux_string_eq(flux_string a, flux_string b) {");
    cg_line(cg,"    if (a.length != b.length) return false;");
    cg_line(cg,"    if (a.length == 0) return true;");
    cg_line(cg,"    return memcmp(a.data, b.data, (size_t)a.length) == 0;");
    cg_line(cg,"}");
    cg_line(cg,"");

    cg_line(cg,"#if defined(_WIN32)||defined(__WIN32__)");
    cg_line(cg,"extern void* __stdcall LoadLibraryA(const void*);");
    cg_line(cg,"extern void* __stdcall GetProcAddress(void*, const void*);");
    cg_line(cg,"extern void* __stdcall GetModuleHandleA(const void*);");
    cg_line(cg,"extern void __stdcall ExitProcess(uint32_t);");
    cg_line(cg,"int _fltused = 0;");
    cg_line(cg,"void __main(void) {}");
    cg_line(cg,"#endif");
    cg_line(cg,"");

    static const char *extern_decls[] = {
        "extern void* malloc(size_t);",
        "extern void* calloc(size_t, size_t);",
        "extern void* realloc(void*, size_t);",
        "extern void free(void*);",
        "extern void* memset(void*, int, size_t);",
        "extern void* memcpy(void*, const void*, size_t);",
        "extern void* memmove(void*, const void*, size_t);",
        "extern int strcmp(const char*, const char*);",
        "extern int printf(const char*, ...);",
        "extern int sprintf(char*, const char*, ...);",
        "extern int puts(const char*);",
        "extern int putchar(int);",
        "extern int scanf(const char*, ...);",
        "extern void exit(int);",
        "extern void* fopen(const char*, const char*);",
        "extern int fclose(void*);",
        "extern int fgetc(void*);",
        "extern int fputc(int, void*);",
        "extern int feof(void*);",
        "extern int system(const char*);",
        "extern double pow(double, double);",
        NULL
    };
    for(int i=0;extern_decls[i];i++) cg_line(cg,extern_decls[i]);
    cg_line(cg,"");

    for(int i=0;i<prog->structs.len;i++) {
        if(!prog->structs.data[i].is_enum)
            cg_linef(cg,"struct %s;",prog->structs.data[i].name);
    }
    cg_line(cg,"");

    for(int i=0;i<prog->structs.len;i++) {
        StructDef *s=&prog->structs.data[i];
        if(s->is_enum) {
            cg_linef(cg,"typedef enum {");
            cg->indent++;
            for(int j=0;j<s->fields.len;j++) {
                StructField *f=&s->fields.data[j];
                if(f->value) cg_linef(cg,"%s = %s,",f->name,f->value);
                else cg_linef(cg,"%s,",f->name);
            }
            cg->indent--;
            cg_linef(cg,"} %s;",s->name);
        } else {
            cg_linef(cg,"struct %s {",s->name);
            cg->indent++;
            for(int j=0;j<s->fields.len;j++) {
                char *ct=ctype_str(s->fields.data[j].type?s->fields.data[j].type:"int");
                cg_linef(cg,"%s %s;",ct,s->fields.data[j].name?s->fields.data[j].name:"_");
                free(ct);
            }
            cg->indent--;
            cg_line(cg,"};");
        }
    }
    cg_line(cg,"");

    static const char *skip_std[]={
        "malloc","calloc","realloc","free","memset","memcpy","memmove",
        "printf","sprintf","puts","putchar","scanf","exit","fopen",
        "fgetc","feof","fclose","fputc","system",
        "LoadLibraryA","GetProcAddress","GetModuleHandleA","ExitProcess",NULL};

    for(int i=0;i<prog->globals.len;i++) {
        GlobalVar *g=&prog->globals.data[i];
        char *ct=ctype_str(g->type?g->type:"int");
        if(g->value) {
            char *val=gen_expr(cg,g->value);
            if(g->value->kind==ND_LITERAL_INT||g->value->kind==ND_LITERAL_FLOAT
               ||g->value->kind==ND_LITERAL_STR||g->value->kind==ND_LITERAL_BOOL) {
                cg_linef(cg,"%s %s = %s;",ct,g->name,val);
            } else {
                cg_linef(cg,"%s %s;",ct,g->name);
                if(cg->dyn_count>=cg->dyn_cap) {
                    cg->dyn_cap=cg->dyn_cap?cg->dyn_cap*2:16;
                    cg->dyn_names=realloc(cg->dyn_names,sizeof(char*)*(size_t)cg->dyn_cap);
                    cg->dyn_vals =realloc(cg->dyn_vals, sizeof(char*)*(size_t)cg->dyn_cap);
                }
                cg->dyn_names[cg->dyn_count]=strdup(g->name);
                cg->dyn_vals [cg->dyn_count]=strdup(val);
                cg->dyn_count++;
            }
            free(val);
        } else {
            cg_linef(cg,"%s %s;",ct,g->name);
        }
        free(ct);
    }
    cg_line(cg,"");

    for(int i=0;i<prog->funcs.len;i++) {
        FuncDef *f=&prog->funcs.data[i];
        if(!f->is_extern) continue;
        bool skip=false;
        for(int j=0;skip_std[j];j++) if(f->name&&strcmp(f->name,skip_std[j])==0){skip=true;break;}
        if(skip) continue;
        char sig[1024]; gen_func_sig(cg,f,sig,sizeof(sig));
        cg_linef(cg,"%s;",sig);
    }
    for(int i=0;i<prog->funcs.len;i++) {
        FuncDef *f=&prog->funcs.data[i];
        if(f->is_extern) continue;
        char sig[1024]; gen_func_sig(cg,f,sig,sizeof(sig));
        cg_linef(cg,"%s;",sig);
    }
    cg_line(cg,"");

    for(int i=0;i<prog->funcs.len;i++) {
        FuncDef *f=&prog->funcs.data[i];
        if(f->is_extern) continue;
        char sig[1024]; gen_func_sig(cg,f,sig,sizeof(sig));
        cg_linef(cg,"%s {",sig);
        cg->indent++;
        for(int j=0;j<f->body.len;j++) gen_stmt(cg,f->body.data[j]);
        if(f->return_type&&strcmp(f->return_type,"void")!=0) {
            bool has_ret=false;
            if(f->body.len>0&&f->body.data[f->body.len-1]&&
               f->body.data[f->body.len-1]->kind==ND_RETURN) has_ret=true;
            if(!has_ret) cg_line(cg,"return 0;");
        }
        cg->indent--;
        cg_line(cg,"}");
        cg_line(cg,"");
    }

    cg_line(cg,"void CblerrInitGlobals(void) {");
    cg->indent++;
    for(int i=0;i<cg->dyn_count;i++)
        cg_linef(cg,"%s = %s;",cg->dyn_names[i],cg->dyn_vals[i]);
    cg->indent--;
    cg_line(cg,"}");

    cg_line(cg,"#if defined(_WIN32)||defined(__WIN32__)");
    cg_line(cg,"void CblerrStartup(void) { CblerrInitGlobals(); main(); ExitProcess(0); }");
    cg_line(cg,"#endif");

    for(int i=0;i<cg->dyn_count;i++){free(cg->dyn_names[i]);free(cg->dyn_vals[i]);}
    free(cg->dyn_names); free(cg->dyn_vals);
    return cg->buf;
}
