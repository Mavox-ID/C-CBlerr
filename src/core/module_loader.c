/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#include "module_loader.h"
#include "lexer.h"
#include "parser.h"
#include "debugger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32
#  include <limits.h>
#  include <unistd.h>
extern char *realpath(const char * restrict, char * restrict);
#endif

static inline char *_cbl_strdup(const char *s) {
    if(!s) return NULL;
    size_t l = strlen(s)+1; char *r = malloc(l);
    if(r) { memcpy(r,s,l); }
    return r;
}
#define strdup _cbl_strdup

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP '\\'
#else
#include <limits.h>
#define PATH_SEP '/'
#endif

static void dir_of(const char *path, char *out, int out_sz) {
    snprintf(out, (size_t)out_sz, "%s", path);
    char *last=NULL;
    for(char *p=out;*p;p++) if(*p=='/'||*p=='\\') last=p;
    if(last) *last=0;
    else { out[0]='.'; out[1]=0; }
}

static void join_path(const char *dir, const char *name, char *out, int out_sz) {
    snprintf(out, out_sz, "%s%c%s", dir, PATH_SEP, name);
}

#ifndef _WIN32
#include <stdlib.h>
#endif

static void resolve_path(const char *p, char *out, int out_sz) {
#ifdef _WIN32
    DWORD r = GetFullPathNameA(p, out_sz, out, NULL);
    if(!r){ snprintf(out, (size_t)out_sz, "%s", p); }
#else
    if(!realpath(p, out)){ snprintf(out, (size_t)out_sz, "%s", p); }
#endif
    out[out_sz-1]=0;
}

#define MAX_INCLUDED 256
static char included[MAX_INCLUDED][512];
static int  n_included = 0;

static bool already_included(const char *path) {
    for(int i=0;i<n_included;i++) if(strcmp(included[i],path)==0) return true;
    return false;
}
static void mark_included(const char *path) {
    if(n_included < MAX_INCLUDED) { snprintf(included[n_included], 512, "%s", path); n_included++; }
}

static char *read_file(const char *path) {
    FILE *f=fopen(path,"rb");
    if(!f) return NULL;
    fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
    char *buf=malloc((size_t)sz+1);
    { size_t _r = fread(buf,1,(size_t)sz,f); (void)_r; } buf[sz]=0;
    fclose(f); return buf;
}

static void merge(Program *dst, Program *src) { /* Just merge? OK */
    for(int i=0;i<src->funcs.len;i++) {
        FuncDef f=src->funcs.data[i];
        bool dup=false;
        for(int j=0;j<dst->funcs.len;j++)
            if(dst->funcs.data[j].name&&f.name&&strcmp(dst->funcs.data[j].name,f.name)==0){dup=true;break;}
        if(!dup) VEC_PUSH(&dst->funcs,f);
    }
    for(int i=0;i<src->structs.len;i++) {
        StructDef s=src->structs.data[i];
        bool dup=false;
        for(int j=0;j<dst->structs.len;j++)
            if(dst->structs.data[j].name&&s.name&&strcmp(dst->structs.data[j].name,s.name)==0){dup=true;break;}
        if(!dup) VEC_PUSH(&dst->structs,s);
    }
    for(int i=0;i<src->globals.len;i++) {
        GlobalVar g=src->globals.data[i];
        bool dup=false;
        for(int j=0;j<dst->globals.len;j++)
            if(dst->globals.data[j].name&&g.name&&strcmp(dst->globals.data[j].name,g.name)==0){dup=true;break;}
        if(!dup) VEC_PUSH(&dst->globals,g);
    }
}

static int resolve_imports(Program *prog, const char *base_dir) {
    int n = prog->imports.len;
    for(int ii=0;ii<n;ii++) {
        Import *imp=&prog->imports.data[ii];
        if(!imp->module_name) continue;

        char candidate[512];
        char resolved[512];

        join_path(base_dir, imp->module_name, candidate, sizeof(candidate));
        resolve_path(candidate, resolved, sizeof(resolved));

        FILE *test=fopen(resolved,"rb");
        if(!test) {
            char with_ext[768];
            snprintf(with_ext,sizeof(with_ext),"%s.cbl",candidate);
            resolve_path(with_ext, resolved, sizeof(resolved));
            test=fopen(resolved,"rb");
        }
        if(!test) {
            DBG_WARN("Import not found: %s", imp->module_name);
            continue;
        }
        fclose(test);

        if(already_included(resolved)) continue;
        mark_included(resolved);

        char *src=read_file(resolved);
        if(!src) { DBG_WARN("Cannot read import: %s", resolved); continue; }

        int tok_count=0;
        Token *toks=tokenize(src, &tok_count);
        free(src);
        Program *imported=parse(toks, tok_count);
        tokens_free(toks, tok_count);

        char imp_dir[512]; dir_of(resolved, imp_dir, sizeof(imp_dir));
        resolve_imports(imported, imp_dir);

        merge(prog, imported);
    }
    prog->imports.len=0;
    return 0;
}

int module_inline_imports(Program *prog, const char *source_path) {
    n_included=0;
    char resolved[512]; resolve_path(source_path, resolved, sizeof(resolved));
    mark_included(resolved);

    char base_dir[512]; dir_of(source_path, base_dir, sizeof(base_dir));
    return resolve_imports(prog, base_dir);
}

bool module_resolve_path(const char *module_name, const char *base_dir,
                          char *out_path, int out_sz) {
    if(!module_name || !out_path) return false;

    char candidate[512];

    snprintf(candidate,sizeof(candidate),"%s%c%s",
             base_dir, PATH_SEP, module_name);
    if(access(candidate,4)==0){ snprintf(out_path,out_sz,"%s",candidate); return true; }

    snprintf(candidate,sizeof(candidate),"%s%c%s.cbl", /* Real only cbl files supported? */
             base_dir, PATH_SEP, module_name);
    if(access(candidate,4)==0){ snprintf(out_path,out_sz,"%s",candidate); return true; }

    snprintf(candidate,sizeof(candidate),"%s%c%s%c%s.cbl",
             base_dir, PATH_SEP, module_name, PATH_SEP, module_name);
    if(access(candidate,4)==0){ snprintf(out_path,out_sz,"%s",candidate); return true; }

    snprintf(candidate,sizeof(candidate),".%c%s.cbl",PATH_SEP,module_name);
    if(access(candidate,4)==0){ snprintf(out_path,out_sz,"%s",candidate); return true; }

    const char *env_path = getenv("CBLERR_PATH");
    if(env_path) {
        char ep_buf[2048]; strncpy(ep_buf,env_path,sizeof(ep_buf)-1); ep_buf[sizeof(ep_buf)-1]=0;
        char *tok = strtok(ep_buf,
#ifdef _WIN32
            ";"
#else
            ":"
#endif
        );
        while(tok) {
            snprintf(candidate,sizeof(candidate),"%s%c%s.cbl",tok,PATH_SEP,module_name);
            if(access(candidate,4)==0){
                snprintf(out_path,out_sz,"%s",candidate); return true;
            }
            tok = strtok(NULL,
#ifdef _WIN32
                ";"
#else
                ":"
#endif
            );
        }
    }

    return false;
}
