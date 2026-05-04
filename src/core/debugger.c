/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#define _POSIX_C_SOURCE 200809L
#include "debugger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#ifdef _WIN32
#  include <windows.h>
#  ifndef _WIN64
typedef unsigned int uintptr_t;
#  else
typedef unsigned long long uintptr_t;
#  endif
#else
#  include <unistd.h>
#  include <stdint.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static char *_dbg_strdup(const char *s){
    if(!s) return NULL;
    size_t l=strlen(s)+1;
    char *r=malloc(l);
    if(r){ memcpy(r,s,l); }
    return r;
}
#pragma GCC diagnostic pop

static const char *g_known_keywords[] = {
    "def","return","endofcode","if","else","elif","while","for","break","continue",
    "struct","enum","match","case","default","import","from","module","as",
    "int","str","bool","float","void","u8","u16","u32","u64","i8","i16","i32","i64","f64",
    "and","or","not","in","asm","comptime","let","const","extern","packed","inline",
    "true","false","null","sizeof","print",
    "printf","sprintf","snprintf","scanf","fscanf","fprintf",
    "malloc","calloc","realloc","free",
    "memcpy","memmove","memset","memcmp","memchr",
    "strlen","strcpy","strncpy","strcat","strncat","strcmp","strncmp",
    "strstr","strchr","strrchr","strtok","atoi","atof","strtol","strtod",
    "puts","putchar","getchar","exit","abort","system","qsort","bsearch",
    "rand","srand","sin","cos","tan","sqrt","pow","abs","fabs","floor","ceil",
    "fmod","log","exp","atan2",
    "fopen","fclose","fread","fwrite","fseek","ftell","rewind","fflush",
    "remove","rename","tmpfile","time","clock","difftime","mktime",
    "getenv","atexit","signal","raise",
    "PostQuitMessage","CreateWindowExA","GetDC","ReleaseDC",
    "ShowWindow","GetConsoleWindow","SetConsoleMode","GetConsoleMode",
    "GetStdHandle","WriteConsole","ReadConsole",
    "GetAsyncKeyState","Sleep","GetCursorPos","ScreenToClient",
    "GetMessageA","DispatchMessageA","PeekMessageA","TranslateMessage",
    "MessageBoxA","WinMain","wWinMain","DllMain",
    "ExitProcess","GetModuleHandleA","GetProcAddress","LoadLibraryA","FreeLibrary",
    "Beep","PlaySoundA","sndPlaySoundA","timeGetTime","GetTickCount",
    "CreateFileA","ReadFile","WriteFile","CloseHandle",
    "HeapAlloc","HeapFree","GetProcessHeap",NULL
};

static int g_color_detected = -1;

static int _detect_color_support(void){
    if(getenv("NO_COLOR")) return 0;
#ifdef _WIN32
    HANDLE hout=GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode=0;
    if(!GetConsoleMode(hout,&mode)) return 0;
    SetConsoleMode(hout,mode|0x0004);
    return 1;
#else
    if(isatty(fileno(stdout))||isatty(fileno(stderr))) return 1;
    return 0;
#endif
}

bool dbg_color_supported(void){
    if(g_color_detected==-1) g_color_detected=_detect_color_support();
    return (bool)g_color_detected;
}
void dbg_init(bool color){ g_color_detected=color?1:0; }

const char *dbg_color_red   (void){return dbg_color_supported()?"\033[31m":"";}
const char *dbg_color_yellow(void){return dbg_color_supported()?"\033[33m":"";}
const char *dbg_color_blue  (void){return dbg_color_supported()?"\033[34m":"";}
const char *dbg_color_cyan  (void){return dbg_color_supported()?"\033[36m":"";}
const char *dbg_color_reset (void){return dbg_color_supported()?"\033[0m": "";}

int dbg_levenshtein(const char *a, const char *b){
    int la=(int)strlen(a), lb=(int)strlen(b);
    if(la<lb){const char *t=a;a=b;b=t;int tt=la;la=lb;lb=tt;}
    if(lb==0) return la;
    int *prev=calloc((size_t)(lb+1),sizeof(int));
    int *curr=calloc((size_t)(lb+1),sizeof(int));
    for(int j=0;j<=lb;j++) prev[j]=j;
    for(int i=1;i<=la;i++){
        curr[0]=i;
        for(int j=1;j<=lb;j++){
            int ins=prev[j]+1, del=curr[j-1]+1, sub=prev[j-1]+(a[i-1]!=b[j-1]?1:0);
            curr[j]=ins<del?(ins<sub?ins:sub):(del<sub?del:sub);
        }
        int *t=prev;prev=curr;curr=t;
    }
    int r=prev[lb]; free(prev);free(curr); return r;
}

double dbg_token_similarity(const char *unknown, const char *candidate){
    char u[256],c[256]; int i=0,j=0;
    while(unknown[i]&&i<255){u[i]=(char)tolower((unsigned char)unknown[i]);i++;} u[i]=0;
    while(candidate[j]&&j<255){c[j]=(char)tolower((unsigned char)candidate[j]);j++;} c[j]=0;
    if(strcmp(u,c)==0) return 0.0;
    int dist=dbg_levenshtein(u,c);
    int max_len=(int)(strlen(u)>strlen(c)?strlen(u):strlen(c));
    if(max_len==0) return 1e9;
    double base_sim=(double)dist/(double)max_len;
    int len_diff=(int)strlen(u)-(int)strlen(c); if(len_diff<0)len_diff=-len_diff;
    double len_penalty=(double)len_diff*0.1;
    if(u[0]==c[0]) base_sim*=0.85;
    int min_len=(int)(strlen(u)<strlen(c)?strlen(u):strlen(c));
    int cpfx=0;
    for(int k=0;k<min_len;k++){if(u[k]==c[k])cpfx++;else break;}
    if(cpfx>0) base_sim*=(1.0-(double)cpfx/(double)max_len*0.3);
    return base_sim+len_penalty;
}

const char *dbg_find_closest_match(const char *unknown, int max_distance){
    if(!unknown||!unknown[0]) return NULL;
    char u[256]; int ui=0;
    while(unknown[ui]&&ui<255){u[ui]=(char)tolower((unsigned char)unknown[ui]);ui++;} u[ui]=0;
    typedef struct{double score;int dist;int len_diff;int idx;}Cand;
    Cand best[512]; int nb=0;
    for(int i=0;g_known_keywords[i];i++){
        const char *kw=g_known_keywords[i];
        char kl[256]; int ki=0;
        while(kw[ki]&&ki<255){kl[ki]=(char)tolower((unsigned char)kw[ki]);ki++;} kl[ki]=0;
        int d=dbg_levenshtein(u,kl);
        if(d<=max_distance&&nb<512){
            int ld=(int)strlen(u)-(int)strlen(kw); if(ld<0)ld=-ld;
            double sc=dbg_token_similarity(u,kl);
            best[nb++]=(Cand){sc,d,ld,i};
        }
    }
    if(nb==0) return NULL;
    for(int i=0;i<nb-1;i++)
        for(int j=i+1;j<nb;j++)
            if(best[j].dist<best[i].dist||(best[j].dist==best[i].dist&&best[j].score<best[i].score)){
                Cand t=best[i];best[i]=best[j];best[j]=t;
            }
    return g_known_keywords[best[0].idx];
}

static void _rotate_log(GameDebugger *d){
    if(!d->log_file[0]) return;
    FILE *f=fopen(d->log_file,"rb"); if(!f) return;
    fseek(f,0,SEEK_END); long sz=ftell(f); fclose(f);
    if(sz<d->max_log_size) return;
    time_t now=time(NULL); struct tm *tm_i=localtime(&now);
    char ts[32]; strftime(ts,sizeof(ts),"%Y%m%d_%H%M%S",tm_i);
    char stem[DEBUGGER_MAX_LOGPATH]; strncpy(stem,d->log_file,sizeof(stem)-1); stem[sizeof(stem)-1]=0;
    char *dot=strrchr(stem,'.'); if(dot)*dot=0;
    char newname[DEBUGGER_MAX_LOGPATH+64];
    snprintf(newname,sizeof(newname),"%s_%s.log",stem,ts);
    rename(d->log_file,newname);
}
static void _write_log(GameDebugger *d, const char *msg){
    if(!d->log_file[0]) return;
    _rotate_log(d);
    FILE *f=fopen(d->log_file,"a"); if(!f) return;
    fprintf(f,"%s\n",msg); fclose(f);
}

static void _format_message(GameDebugger *d, DebugLevel level, const char *msg,
                              char *con, int csz, char *fil, int fsz){
    time_t now=time(NULL); struct tm *tm_i=localtime(&now);
    char ts[32]; strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M:%S",tm_i);
    double elapsed=(double)clock()/(double)CLOCKS_PER_SEC - d->start_time;
    static const char *lnames[]={"NONE ","Error","Warn ","INFO ","VERB ","TRACE"};
    static const char *lcolors[]={"\033[0m","\033[31m","\033[33m","\033[32m","\033[36m","\033[90m"};
    const char *lname =(level>=0&&level<=5)?lnames [level]:"UNKN ";
    const char *lcolor=(d->use_colors&&level>=0&&level<=5)?lcolors[level]:"";
    const char *rst   = d->use_colors?"\033[0m":"";
    snprintf(fil,fsz,"[%s] [%s] [%7.3fs] %s",ts,lname,elapsed,msg);
    snprintf(con,csz,"%s[%s] [%s] [%7.3fs]%s %s",lcolor,ts,lname,elapsed,rst,msg);
}

static void _do_log(GameDebugger *d, DebugLevel lv, FILE *out, const char *fmt, va_list ap){
    if((int)d->level<(int)lv) return;
    char raw[4096]; vsnprintf(raw,sizeof(raw),fmt,ap);
    char con[4352],fil[4352];
    _format_message(d,lv,raw,con,sizeof(con),fil,sizeof(fil));
    fprintf(out,"%s\n",con);
    _write_log(d,fil);
}
void debugger_log_error  (GameDebugger *d,const char *fmt,...){va_list ap;va_start(ap,fmt);_do_log(d,DBG_ERROR,  stderr,fmt,ap);va_end(ap);d->error_count++;}
void debugger_log_warning(GameDebugger *d,const char *fmt,...){va_list ap;va_start(ap,fmt);_do_log(d,DBG_WARNING,stderr,fmt,ap);va_end(ap);d->warning_count++;}
void debugger_log_info   (GameDebugger *d,const char *fmt,...){va_list ap;va_start(ap,fmt);_do_log(d,DBG_INFO,   stdout,fmt,ap);va_end(ap);}
void debugger_log_verbose(GameDebugger *d,const char *fmt,...){va_list ap;va_start(ap,fmt);_do_log(d,DBG_VERBOSE,stdout,fmt,ap);va_end(ap);}
void debugger_log_trace  (GameDebugger *d,const char *fmt,...){va_list ap;va_start(ap,fmt);_do_log(d,DBG_TRACE,  stdout,fmt,ap);va_end(ap);}

void debugger_watch_memory(GameDebugger *d, void *address){
    debugger_log_verbose(d,"Memory watch added: 0x%llx",(unsigned long long)(size_t)address);
}

CrashContext debugger_capture_context(GameDebugger *d, const char *exc_type,
                                       const char *exc_msg,
                                       StackFrame *frames, int n_frames){
    CrashContext ctx; memset(&ctx,0,sizeof(ctx));
    time_t now=time(NULL); struct tm *tm_i=localtime(&now);
    strftime(ctx.timestamp,sizeof(ctx.timestamp),"%Y-%m-%dT%H:%M:%S",tm_i);
    strncpy(ctx.exception_type,   exc_type?exc_type:"UnknownError",  sizeof(ctx.exception_type)-1);
    strncpy(ctx.exception_message,exc_msg ?exc_msg :"",              sizeof(ctx.exception_message)-1);
    ctx.elapsed_time=(double)clock()/(double)CLOCKS_PER_SEC - d->start_time;
    ctx.n_frames=n_frames<CRASH_MAX_FRAMES?n_frames:CRASH_MAX_FRAMES;
    if(frames&&ctx.n_frames>0) memcpy(ctx.stack_frames,frames,sizeof(StackFrame)*(size_t)ctx.n_frames);
    ctx.mem_rss_kb=-1;
#ifndef _WIN32
    FILE *mf=fopen("/proc/self/status","r");
    if(mf){ char ln[256];
        while(fgets(ln,sizeof(ln),mf))
            if(strncmp(ln,"VmRSS:",6)==0){ctx.mem_rss_kb=atol(ln+6);break;}
        fclose(mf); }
#endif
    return ctx;
}

void debugger_critical_dump(GameDebugger *d, const char *exc_type, const char *exc_msg){
    StackFrame nf[1]; memset(nf,0,sizeof(nf));
    CrashContext ctx=debugger_capture_context(d,exc_type,exc_msg,nf,0);
    debugger_log_error(d,"Critical Error!");
    debugger_log_error(d,"Exception Type: %s",    ctx.exception_type);
    debugger_log_error(d,"Exception Message: %s", ctx.exception_message);
    debugger_log_error(d,"Timestamp: %s",          ctx.timestamp);
    debugger_log_error(d,"Elapsed Time: %.3fs",    ctx.elapsed_time);
    if(ctx.n_frames>0){
        debugger_log_error(d,"Stack call:");
        for(int i=0;i<ctx.n_frames;i++){
            StackFrame *f=&ctx.stack_frames[i];
            debugger_log_error(d,"  Frame %d: %s (%s:%d)",i+1,f->function,f->filename,f->lineno);
            if(f->code[0]) debugger_log_error(d,"    Code: %s",f->code);
        }
    }
    if(ctx.mem_rss_kb>=0) debugger_log_error(d,"Memory (RSS): %ld KB",ctx.mem_rss_kb);
    debugger_log_error(d,"================================================");
    debugger_log_error(d,"Total errors:   %d",d->error_count);
    debugger_log_error(d,"Total warnings: %d",d->warning_count);
    if(d->log_file[0]) debugger_log_error(d,"Log file: %s",d->log_file);
}

void debugger_print_summary(GameDebugger *d){
    double elapsed=(double)clock()/(double)CLOCKS_PER_SEC - d->start_time;
    printf("\n|              Debug Report              |\n");
    printf("  Elapsed time: %.3fs\n",elapsed);
    printf("  Errors:       %d\n",d->error_count);
    printf("  Warnings:     %d\n",d->warning_count);
    if(d->log_file[0]) printf("  Log file:     %s\n",d->log_file);
}

void debugger_display_syntax_error(GameDebugger *d, const char *msg,
                                    const char *source, const char *filename,
                                    int lineno, int col_start, int col_end){
    const char *RED=dbg_color_red(), *BLUE=dbg_color_blue();
    const char *YEL=dbg_color_yellow(), *RST=dbg_color_reset();

    fprintf(stderr,"%s[ERROR]%s %s\n",RED,RST,msg?msg:"Syntax error");

    if(source&&lineno>0){
        const char *ls=source; int cur=1;
        while(*ls&&cur<lineno){if(*ls=='\n')cur++;ls++;}
        const char *le=ls; while(*le&&*le!='\n')le++;
        int ll=(int)(le-ls); if(ll>1023)ll=1023;
        char lb[1024]; memcpy(lb,ls,(size_t)ll); lb[ll]=0;
        fprintf(stderr,"%s\n",lb);

        if(col_start<0){
            const char *q=msg?strchr(msg,'\''):NULL;
            if(!q&&msg) q=strchr(msg,'`');
            if(q){ q++;
                const char *qe=strchr(q,'\'');
                if(qe){
                    char tok[128]; int tl=(int)(qe-q); if(tl>127)tl=127;
                    memcpy(tok,q,(size_t)tl); tok[tl]=0;
                    char *found=strstr(lb,tok);
                    if(found){ col_start=(int)(found-lb); col_end=col_start+(int)strlen(tok); }
                }
            }
            if(col_start<0){
                col_start=0;
                while(col_start<ll&&lb[col_start]==' ')col_start++;
                col_end=col_start+1;
            }
        }
        if(col_start<0){col_start=0;}
        if(col_start>ll){col_start=ll;}
        if(col_end<=col_start){col_end=col_start+1;}
        if(col_end>ll){col_end=ll;}
        int cn=col_end-col_start; if(cn<1)cn=1;

        for(int i=0;i<col_start;i++)fputc(' ',stderr);
        fprintf(stderr,"%s",BLUE);
        for(int i=0;i<cn;i++)fputc('^',stderr);
        fprintf(stderr,"%s\n",RST);

        if(filename)fprintf(stderr,"%s:%d:%d\n",filename,lineno,col_start+1);
    }

    if(msg&&(strstr(msg,"Unknown")||strstr(msg,"Unexpected")))
        fprintf(stderr,"%s[REASON]%s Invalid or unexpected code fragment\n",YEL,RST);

    if(msg){
        const char *q=strchr(msg,'\''); if(!q)q=strchr(msg,'`');
        if(q){q++;
            const char *qe=strchr(q,'\'');
            if(qe){
                char uk[128]; int ul=(int)(qe-q); if(ul>127)ul=127;
                memcpy(uk,q,(size_t)ul); uk[ul]=0;
                const char *hint=dbg_find_closest_match(uk,3);
                if(hint)fprintf(stderr,"%s[HINT]%s Did you mean: \"%s\"?\n",YEL,RST,hint);
            }
        }
    }
    d->error_count++;
}

static GameDebugger g_dbg_inst;
static int          g_dbg_ready=0;

GameDebugger *debugger_init(DebugLevel level, const char *log_file){
    memset(&g_dbg_inst,0,sizeof(g_dbg_inst));
    g_dbg_inst.level        = level;
    g_dbg_inst.max_log_size = 10*1024*1024;
    g_dbg_inst.use_colors   = dbg_color_supported();
    g_dbg_inst.start_time   = (double)clock()/(double)CLOCKS_PER_SEC;
    if(log_file&&log_file[0])
        strncpy(g_dbg_inst.log_file,log_file,sizeof(g_dbg_inst.log_file)-1);
    g_dbg_ready=1;
    return &g_dbg_inst;
}

GameDebugger *debugger_get(void){
    if(!g_dbg_ready) debugger_init(DBG_INFO,"debug.log");
    return &g_dbg_inst;
}

void debugger_colorize(GameDebugger *d, const char *text,
                        const char *color_code,
                        char *out, int out_sz) {
    if(d->use_colors && color_code && color_code[0])
        snprintf(out,out_sz,"%s%s\033[0m",color_code,text?text:"");
    else
        snprintf(out,out_sz,"%s",text?text:"");
}

GameDebugger *debugger_enter(GameDebugger *d) {
    /* mirrors __enter__ */
    return d;
}

/* mirrors __exit__: if exception (exc_type != NULL) */
void debugger_exit(GameDebugger *d,
                    const char *exc_type,
                    const char *exc_msg) {
    if(exc_type && exc_type[0])
        debugger_critical_dump(d, exc_type, exc_msg);
}

void color_red_str   (const char *text, char *out, int sz){ snprintf(out,sz,"%s%s\033[0m",dbg_color_red(),   text?text:""); }
void color_blue_str  (const char *text, char *out, int sz){ snprintf(out,sz,"%s%s\033[0m",dbg_color_blue(),  text?text:""); }
void color_yellow_str(const char *text, char *out, int sz){ snprintf(out,sz,"%s%s\033[0m",dbg_color_yellow(),text?text:""); }
