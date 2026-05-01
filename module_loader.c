#define _POSIX_C_SOURCE 200809L
#include "module_loader.h"
#include "lexer.h"
#include "parser.h"
#include "debugger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

/* ── path helpers ───────────────────────────────────────────────── */
static void dir_of(const char *path, char *out, int out_sz) {
    strncpy(out, path, (size_t)out_sz-1); out[out_sz-1]=0;
    char *last=NULL;
    for(char *p=out;*p;p++) if(*p=='/'||*p=='\\') last=p;
    if(last) *last=0;
    else { out[0]='.'; out[1]=0; }
}

static void join_path(const char *dir, const char *name, char *out, int out_sz) {
    snprintf(out, out_sz, "%s%c%s", dir, PATH_SEP, name);
}

#ifndef _WIN32
#include <stdlib.h>   /* realpath on Linux */
#endif

static void resolve_path(const char *p, char *out, int out_sz) {
#ifdef _WIN32
    DWORD r = GetFullPathNameA(p, out_sz, out, NULL);
    if(!r) strncpy(out, p, out_sz-1);
#else
    if(!realpath(p, out)) strncpy(out, p, (size_t)(out_sz-1));
#endif
    out[out_sz-1]=0;
}

/* ── included-set (simple linear list of resolved paths) ────────── */
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

/* ── read whole file ────────────────────────────────────────────── */
static char *read_file(const char *path) {
    FILE *f=fopen(path,"rb");
    if(!f) return NULL;
    fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
    char *buf=malloc((size_t)sz+1);
    { size_t _r = fread(buf,1,(size_t)sz,f); (void)_r; } buf[sz]=0;
    fclose(f); return buf;
}

/* ── merge imported program into host ───────────────────────────── */
static void merge(Program *dst, Program *src) {
    /* functions */
    for(int i=0;i<src->funcs.len;i++) {
        FuncDef f=src->funcs.data[i];
        /* check duplicate */
        bool dup=false;
        for(int j=0;j<dst->funcs.len;j++)
            if(dst->funcs.data[j].name&&f.name&&strcmp(dst->funcs.data[j].name,f.name)==0){dup=true;break;}
        if(!dup) VEC_PUSH(&dst->funcs,f);
    }
    /* structs */
    for(int i=0;i<src->structs.len;i++) {
        StructDef s=src->structs.data[i];
        bool dup=false;
        for(int j=0;j<dst->structs.len;j++)
            if(dst->structs.data[j].name&&s.name&&strcmp(dst->structs.data[j].name,s.name)==0){dup=true;break;}
        if(!dup) VEC_PUSH(&dst->structs,s);
    }
    /* globals */
    for(int i=0;i<src->globals.len;i++) {
        GlobalVar g=src->globals.data[i];
        bool dup=false;
        for(int j=0;j<dst->globals.len;j++)
            if(dst->globals.data[j].name&&g.name&&strcmp(dst->globals.data[j].name,g.name)==0){dup=true;break;}
        if(!dup) VEC_PUSH(&dst->globals,g);
    }
}

/* ── recursive import resolver ──────────────────────────────────── */
static int resolve_imports(Program *prog, const char *base_dir) {
    /* collect imports to process (prog->imports may grow) */
    int n = prog->imports.len;
    for(int ii=0;ii<n;ii++) {
        Import *imp=&prog->imports.data[ii];
        if(!imp->module_name) continue;

        /* try to find the file */
        char candidate[512];
        char resolved[512];

        /* if already has extension, try directly */
        join_path(base_dir, imp->module_name, candidate, sizeof(candidate));
        resolve_path(candidate, resolved, sizeof(resolved));

        /* if not found, try adding .cbl */
        FILE *test=fopen(resolved,"rb");
        if(!test) {
            char with_ext[768];
            snprintf(with_ext,sizeof(with_ext),"%s.cbl",candidate);
            resolve_path(with_ext, resolved, sizeof(resolved));
            test=fopen(resolved,"rb");
        }
        if(!test) {
            dbg_warn("Import not found: %s", imp->module_name);
            continue;
        }
        fclose(test);

        if(already_included(resolved)) continue;
        mark_included(resolved);

        char *src=read_file(resolved);
        if(!src) { dbg_warn("Cannot read import: %s", resolved); continue; }

        int tok_count=0;
        Token *toks=tokenize(src, &tok_count);
        free(src);
        Program *imported=parse(toks, tok_count);
        tokens_free(toks, tok_count);

        /* recursively resolve imports of the imported file */
        char imp_dir[512]; dir_of(resolved, imp_dir, sizeof(imp_dir));
        resolve_imports(imported, imp_dir);

        merge(prog, imported);
    }
    prog->imports.len=0; /* clear imports after processing */
    return 0;
}

int module_inline_imports(Program *prog, const char *source_path) {
    n_included=0;
    /* mark the source file itself as included */
    char resolved[512]; resolve_path(source_path, resolved, sizeof(resolved));
    mark_included(resolved);

    char base_dir[512]; dir_of(source_path, base_dir, sizeof(base_dir));
    return resolve_imports(prog, base_dir);
}
