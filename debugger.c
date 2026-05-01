#include "debugger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ── ANSI colors (disabled on Windows console unless VT enabled) ─── */
#ifdef _WIN32
static bool g_color = false;
#else
static bool g_color = true;
#endif

void dbg_init(bool color) { g_color = color; }

static const char *C_RED    = "\033[31m";
static const char *C_YELLOW = "\033[33m";
static const char *C_CYAN   = "\033[36m";
static const char *C_RESET  = "\033[0m";

static void print_col(FILE *f, const char *col, const char *msg) {
    if(g_color) fprintf(f, "%s%s%s", col, msg, C_RESET);
    else fprintf(f, "%s", msg);
}

void dbg_info(const char *fmt, ...) {
    char buf[2048]; va_list ap; va_start(ap,fmt); vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap);
    print_col(stdout, C_CYAN, "[Info] "); printf("%s\n", buf);
}
void dbg_warn(const char *fmt, ...) {
    char buf[2048]; va_list ap; va_start(ap,fmt); vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap);
    print_col(stderr, C_YELLOW, "[Warn] "); fprintf(stderr, "%s\n", buf);
}
void dbg_error(const char *fmt, ...) {
    char buf[2048]; va_list ap; va_start(ap,fmt); vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap);
    print_col(stderr, C_RED, "[ERROR] "); fprintf(stderr, "%s\n", buf);
}

void dbg_syntax_error(const char *msg, const char *source,
                      int lineno, int col_start, int col_end) {
    dbg_error("%s", msg);
    if(!source || lineno<=0) return;
    /* find the line in source */
    const char *p=source; int cur_line=1;
    while(*p && cur_line<lineno) { if(*p=='\n') cur_line++; p++; }
    /* print the line */
    const char *end=p; while(*end && *end!='\n') end++;
    int len=(int)(end-p);
    char *line_buf=malloc((size_t)len+2);
    memcpy(line_buf,p,(size_t)len); line_buf[len]=0;
    fprintf(stderr, "%s\n", line_buf);
    free(line_buf);
    /* caret */
    if(col_start>=0) {
        int cs=col_start<len?col_start:len;
        int ce=col_end>cs?col_end:cs+1;
        if(ce>len) ce=len;
        for(int i=0;i<cs;i++) fputc(' ',stderr);
        if(g_color) fprintf(stderr,"%s",C_CYAN);
        for(int i=cs;i<ce;i++) fputc('^',stderr);
        if(g_color) fprintf(stderr,"%s",C_RESET);
        fputc('\n',stderr);
    }
    /* suggestion */
    const char *sug=dbg_suggest(msg);
    if(sug) {
        if(g_color) fprintf(stderr,"%s[HINT]%s Did you mean: \"%s\"?\n",C_YELLOW,C_RESET,sug);
        else fprintf(stderr,"[HINT] Did you mean: \"%s\"?\n",sug);
    }
}

/* ── Levenshtein suggestion ─────────────────────────────────────── */
static const char *KNOWN[] = {
    "def","return","endofcode","if","else","elif","while","for","break","continue",
    "struct","enum","match","case","default","import","from","as",
    "int","str","bool","float","void","u8","u16","u32","u64","i8","i16","i32","i64",
    "and","or","not","in","asm","let","const","extern","sizeof","print",
    "printf","malloc","calloc","free","memcpy","memset","strlen","strcmp","exit",NULL
};

static int lev(const char *a, const char *b) {
    int la=(int)strlen(a),lb=(int)strlen(b);
    if(la<lb){const char*tmp=a;a=b;b=tmp;int t=la;la=lb;lb=t;}
    int *prev=calloc((size_t)lb+1,sizeof(int)),*curr=calloc((size_t)lb+1,sizeof(int));
    for(int j=0;j<=lb;j++) prev[j]=j;
    for(int i=1;i<=la;i++) {
        curr[0]=i;
        for(int j=1;j<=lb;j++) {
            int cost=a[i-1]!=b[j-1];
            int ins=prev[j]+1,del=curr[j-1]+1,sub=prev[j-1]+cost;
            curr[j]=ins<del?(ins<sub?ins:sub):(del<sub?del:sub);
        }
        int *tmp=prev;prev=curr;curr=tmp;
    }
    int r=prev[lb]; free(prev); free(curr); return r;
}

const char *dbg_suggest(const char *msg) {
    /* extract last word from error message */
    const char *p=msg+strlen(msg);
    while(p>msg && *p!='\''&&*p!='"'&&*p!=' ') p--;
    const char *end=p;
    while(p>msg && *p!='\''&&*p!='"'&&*p!=' ') p--;
    if(p==end) return NULL;
    char word[64]; int wlen=(int)(end-p); if(wlen>63)wlen=63;
    memcpy(word,p,wlen); word[wlen]=0;
    if(!word[0]) return NULL;

    const char *best=NULL; int best_d=3;
    for(int i=0;KNOWN[i];i++) {
        int d=lev(word,KNOWN[i]);
        if(d<best_d){best_d=d;best=KNOWN[i];}
    }
    return best;
}
