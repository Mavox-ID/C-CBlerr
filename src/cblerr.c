/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

/*
 *  _extract_cli_flags()          → extract_cli_flags()
 *  CCodeGenerator.__init__       → (in codegen.c)
 *  CCodeGenerator.generate()     → (in codegen.c) codegen_generate()
 *  StandaloneCompiler.__init__   → compiler_init()
 *  StandaloneCompiler.log()      → compiler_log()        < [n/t] coloring
 *  _select_compiler()            → compiler_select()
 *  _compiler_exists()            → compiler_exists()
 *  _get_compiler_flags()         → compiler_get_cflags()
 *  _get_linker_flags()           → compiler_get_ldflags()
 *  _handle_compile_error()       → compiler_handle_error()
 *  compile()                     → compiler_compile()    --< try/except wrapping
 *  _compile_c_to_exe()           → compiler_compile_c_to_exe()
 *  _compile_msvc()               → compiler_compile_msvc()
 *  _compile_mingw()              → compiler_compile_mingw()
 *  _compile_gcc()                → compiler_compile_gcc()
 *  _compile_clang()              → compiler_compile_clang()
 *  _compile_lld()                → compiler_compile_lld()
 *  _find_msvc_cl()               → compiler_find_msvc_cl()
 *  main()                        → main()
 */

#define _POSIX_C_SOURCE 200809L

#include "core/lexer.h"
#include "core/parser.h"
#include "core/codegen.h"
#include "core/module_loader.h"
#include "core/debugger.h"
#include "core/type_checker.h"
#include "core/monomorphizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

static inline char *_cbl_strdup(const char *s){
    if(!s) return NULL;
    size_t l=strlen(s)+1;
    char *r=malloc(l);
    if(r){ memcpy(r,s,l); }
    return r;
}
#define strdup _cbl_strdup

#ifdef _WIN32
#  include <windows.h>
#  include <process.h>
#  define PATH_SEP   '\\'
#  define EXE_EXT    ".exe"
#  define IS_WINDOWS 1
#else
#  include <unistd.h>
#  include <sys/stat.h>
#  include <sys/wait.h>
#  define PATH_SEP   '/'
#  define EXE_EXT    ""
#  define IS_WINDOWS 0
#endif

#define C_RESET   "\033[0m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_GREEN   "\033[92m"
#define C_RED     "\033[31m"
#define C_YELLOW  "\033[33m"

static bool        DERR_FLAG        = false;
static bool        SAVE_C_FLAG      = false;  /* -c    */
static const char *g_target_os      = NULL;   /* windows / linux          */
static bool        g_static_link    = false;
static bool        g_dynamic_link   = false;

static void extract_cli_flags(int *argc, char **argv) {
    int new_argc = 0;
    for(int i=0; i<*argc; i++){
        if(strcmp(argv[i],"-derr")==0){
            DERR_FLAG = true;
        } else if(strcmp(argv[i],"-c")==0){
            SAVE_C_FLAG = true;
        } else {
            argv[new_argc++] = argv[i];
        }
    }
    *argc = new_argc;
}

static char *read_file_alloc(const char *path){
    FILE *f=fopen(path,"rb"); if(!f)return NULL;
    fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
    char *buf=malloc((size_t)sz+2); if(!buf){fclose(f);return NULL;}
    size_t rd=fread(buf,1,(size_t)sz,f); buf[rd]=0; fclose(f); return buf;
}
static bool write_file_str(const char *path, const char *data){
    FILE *f=fopen(path,"wb"); if(!f)return false;
    fwrite(data,1,strlen(data),f); fclose(f); return true;
}
static bool path_exists(const char *path){
#ifdef _WIN32
    return GetFileAttributesA(path)!=INVALID_FILE_ATTRIBUTES;
#else
    struct stat st; return stat(path,&st)==0;
#endif
}
static long get_file_size(const char *path){
    FILE *f=fopen(path,"rb"); if(!f)return 0;
    fseek(f,0,SEEK_END); long s=ftell(f); fclose(f); return s;
}
static void path_stem(const char *path, char *out, int sz){
    const char *base=path;
    for(const char *p=path;*p;p++) if(*p=='/'||*p=='\\') base=p+1;
    strncpy(out,base,(size_t)(sz-1)); out[sz-1]=0;
    char *dot=strrchr(out,'.'); if(dot)*dot=0;
}
static void make_tmp_c(const char *stem, char *out, int sz){
#ifdef _WIN32
    char tmp[MAX_PATH]; GetTempPathA(sizeof(tmp),tmp);
    snprintf(out,sz,"%scblerr_standalone\\%s.c",tmp,stem);
    char dir[MAX_PATH]; snprintf(dir,sizeof(dir),"%scblerr_standalone",tmp);
    CreateDirectoryA(dir,NULL);
#else
    const char *td=getenv("TMPDIR"); if(!td) td="/tmp";
    char dir[512]; snprintf(dir,sizeof(dir),"%s/cblerr_standalone",td);
    mkdir(dir,0755);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(out,(size_t)sz,"%s/%s.c",dir,stem); out[sz-1]=0;
#pragma GCC diagnostic pop
#endif
}

static int run_cmd(const char *cmd, char *cap, int cap_sz){
    if(cap) cap[0]=0;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};
    HANDLE hr,hw;
    if(!CreatePipe(&hr,&hw,&sa,0)) return -1;
    STARTUPINFOA si={0}; si.cb=sizeof(si);
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdOutput=hw; si.hStdError=hw;
    si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi={0};
    char cb[8192]; strncpy(cb,cmd,sizeof(cb)-1); cb[sizeof(cb)-1]=0;
    if(!CreateProcessA(NULL,cb,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi)){
        CloseHandle(hr);CloseHandle(hw); return -1;
    }
    CloseHandle(hw);
    if(cap){
        DWORD rd; int tot=0;
        while(tot<cap_sz-1&&ReadFile(hr,(void*)(cap+tot),(DWORD)(cap_sz-tot-1),&rd,NULL)&&rd>0)
            tot+=(int)rd;
        cap[tot]=0;
    }
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD ec=0; GetExitCodeProcess(pi.hProcess,&ec);
    CloseHandle(pi.hProcess);CloseHandle(pi.hThread);CloseHandle(hr);
    return (int)ec;
#else
    char full[8192]; snprintf(full,sizeof(full),"%s 2>&1",cmd);
    FILE *fp=popen(full,"r"); if(!fp) return -1;
    if(cap){
        int tot=0;
        while(tot<cap_sz-1){ int c=fgetc(fp); if(c==EOF)break; cap[tot++]=(char)c; }
        cap[tot]=0;
    }
    int rc=pclose(fp);
    return (rc==-1)?-1:(WIFEXITED(rc)?WEXITSTATUS(rc):1);
#endif
}

typedef struct {
    char  source_file[512];   /* Path(source_file) */
    char  output_exe [512];   /* Path(output_exe)  */
    char  c_file     [512];   /* temp_dir / stem.c */
    bool  verbose;
    bool  is_windows;
    char  system_name[32];    /* platform.system() */
    char  compiler_type[64];  /* resolved compiler */
    char  link_mode[16];      /* static/dynamic/'' */
    long  stack_reserve;
    bool  is_gui_app;
    GameDebugger *debugger;
} StandaloneCompiler;

static void compiler_log(StandaloneCompiler *c, const char *message,
                           const char *level){
    if(!c->verbose) return;
    while(message && *message == '\n') message++;
    char colored[4096]; int ci=0;
    const char *p=message;
    while(*p && ci<(int)sizeof(colored)-64){
        if(*p=='['){
            const char *q=p+1; int n1=0,n2=0; bool got=false;
            while(*q>='0'&&*q<='9'&&q-p<8){ n1=n1*10+(*q-'0'); q++; }
            if(*q=='/'){ q++;
                while(*q>='0'&&*q<='9'&&q-p<16){ n2=n2*10+(*q-'0'); q++; }
                if(*q==']'){ q++; got=true; }
            }
            if(got){
                ci+=snprintf(colored+ci,(size_t)(sizeof(colored)-ci-1),
                             "%s[%d/%d]%s",C_CYAN,n1,n2,C_RESET);
                p=q; continue;
            }
            if(strncmp(p,"[Info]",6)==0){
                ci+=snprintf(colored+ci,(size_t)(sizeof(colored)-ci-1),
                             "%s[Info]%s",C_CYAN,C_RESET);
                p+=6; continue;
            }
        }
        colored[ci++]=*p++;
    }
    colored[ci]=0;

    const char *level_col = C_MAGENTA;
    if(level && strcmp(level,"Warn")==0)  level_col=C_RED;
    if(level && strcmp(level,"Error")==0) level_col=C_RED;

    printf("%s[%s]%s %s\n", level_col, level?level:"Info", C_RESET, colored);
}
static void compiler_log_info(StandaloneCompiler *c, const char *fmt, ...){
    char buf[4096]; va_list ap; va_start(ap,fmt); vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap);
    compiler_log(c,buf,"Info");
}
static void compiler_log_step(StandaloneCompiler *c, int n, int t,
                                const char *fmt, ...){
    char msg[4096]; va_list ap; va_start(ap,fmt); vsnprintf(msg,sizeof(msg),fmt,ap); va_end(ap);
    char full[4096+32]; snprintf(full,sizeof(full),"\n[%d/%d] %s",n,t,msg);
    compiler_log(c,full,"Info");
}

static bool compiler_exists(const char *name){
    char cmd[512]; snprintf(cmd,sizeof(cmd),"%s --version",name);
    char buf[256]; return run_cmd(cmd,buf,sizeof(buf))==0;
}

static bool compiler_select(StandaloneCompiler *sc, const char *forced){
    if(forced && forced[0]){
        char cn[64]; strncpy(cn,forced,sizeof(cn)-1); cn[sizeof(cn)-1]=0;
        if(strcmp(cn,"mingw")==0) strcpy(cn,"gcc");
        if(strcmp(cn,"lld")  ==0) strcpy(cn,"clang");
        if(strcmp(cn,"msvc") ==0){
#ifdef _WIN32
            strncpy(sc->compiler_type,"msvc",sizeof(sc->compiler_type)-1);
            return true;
#else
            compiler_log(sc,"Error: MSVC is only available on Windows.","Error");
            return false;
#endif
        }
#ifdef _WIN32
        char exe[128]; snprintf(exe,sizeof(exe),"%s.exe",cn);
        if(compiler_exists(exe)){strncpy(sc->compiler_type,forced,sizeof(sc->compiler_type)-1);return true;}
#endif
        if(compiler_exists(cn)){strncpy(sc->compiler_type,forced,sizeof(sc->compiler_type)-1);return true;}
        char err[256]; snprintf(err,sizeof(err),"Error: compiler %s not found.",forced);
        compiler_log(sc,err,"Error");
        return false;
    }
#ifdef _WIN32
    if(compiler_exists("gcc.exe")){ strncpy(sc->compiler_type,"gcc",sizeof(sc->compiler_type)-1);return true;}
    if(compiler_exists("clang.exe")){strncpy(sc->compiler_type,"clang",sizeof(sc->compiler_type)-1);return true;}
#else
    if(compiler_exists("gcc"))  {strncpy(sc->compiler_type,"gcc",  sizeof(sc->compiler_type)-1);return true;}
    if(compiler_exists("clang")){strncpy(sc->compiler_type,"clang",sizeof(sc->compiler_type)-1);return true;}
#endif
    strncpy(sc->compiler_type,"gcc",sizeof(sc->compiler_type)-1);
    return true;
}

static void compiler_get_cflags(StandaloneCompiler *sc, char *out, int sz){
    const char *env=getenv("CBLERR_CFLAGS");
    if(env){ strncpy(out,env,(size_t)(sz-1)); out[sz-1]=0; return; }
    strncpy(out,"-std=c11 -Os -s -ffunction-sections -fdata-sections -fno-ident",(size_t)(sz-1));
    if(sc->is_windows)
        strncat(out," -fno-asynchronous-unwind-tables -fno-unwind-tables"
                    " -fomit-frame-pointer -mno-stack-arg-probe -fno-math-errno",
                (size_t)(sz-strlen(out)-1));
    out[sz-1]=0;
}

static void compiler_get_ldflags(StandaloneCompiler *sc, char *out, int sz){
    out[0]=0;
    bool win=sc->is_windows;
    if(g_target_os) win=(strcmp(g_target_os,"windows")==0);

    const char *ct=sc->compiler_type;
    bool gcc_compat=(strcmp(ct,"gcc")==0||strcmp(ct,"clang")==0||
                     strcmp(ct,"lld")==0||strcmp(ct,"mingw")==0);

    if(win){
        strncat(out,"-nostartfiles -Wl,--entry=CblerrStartup",(size_t)(sz-strlen(out)-1));
        if(gcc_compat){
            strncat(out," -Wl,--gc-sections",(size_t)(sz-strlen(out)-1));
            strncat(out," -Wl,--build-id=none",(size_t)(sz-strlen(out)-1));
            strncat(out," -Wl,--no-seh",(size_t)(sz-strlen(out)-1));
            strncat(out," -Wl,--file-alignment=512",(size_t)(sz-strlen(out)-1));
            strncat(out," -Wl,--section-alignment=4096",(size_t)(sz-strlen(out)-1));
        }
        if(sc->is_gui_app)
            strncat(out," -mwindows",(size_t)(sz-strlen(out)-1));
        if(sc->stack_reserve>0){
            char stk[64]; snprintf(stk,sizeof(stk)," -Wl,--stack,%ld",sc->stack_reserve);
            strncat(out,stk,(size_t)(sz-strlen(out)-1));
        }
    } else {
        if(gcc_compat)
            strncat(out,"-Wl,--gc-sections",(size_t)(sz-strlen(out)-1));
        if(g_static_link)
            strncat(out," -static",(size_t)(sz-strlen(out)-1));
        if(sc->stack_reserve>0){
            char stk[64]; snprintf(stk,sizeof(stk)," -Wl,-z,stacksize=%ld",sc->stack_reserve);
            strncat(out,stk,(size_t)(sz-strlen(out)-1));
        }
    }
    out[sz-1]=0;
}

static bool compiler_handle_error(StandaloneCompiler *sc, const char *error_output){
    if(!error_output||!error_output[0]) return false;

    /* Linker errors :> ^.^ */
    if(strstr(error_output,"undefined reference")||
       strstr(error_output,"ld returned 1")){
        fprintf(stderr,"\n%sLinker Error:%s\n%s\n",C_RED,C_RESET,error_output);
        return true;
    }

    char *source = path_exists(sc->source_file)?read_file_alloc(sc->source_file):NULL;

    const char *imp=strstr(error_output,"implicit declaration of function");
    if(!imp) imp=strstr(error_output,"implicit function declaration");
    if(imp && source){
        const char *q=strchr(imp,'\''); if(!q)q=strchr(imp,'`');
        if(q){ q++;
            const char *qe=strchr(q,'\''); if(!qe)qe=strchr(q,'`');
            if(qe){
                char fn[256]; int fl=(int)(qe-q); if(fl>255)fl=255;
                memcpy(fn,q,(size_t)fl); fn[fl]=0;
                const char *ls=source; int ln=1;
                while(*ls){
                    const char *le=strchr(ls,'\n');
                    int ll=le?(int)(le-ls):(int)strlen(ls);
                    char lb[1024]; int cp=ll<1023?ll:1023;
                    memcpy(lb,ls,(size_t)cp); lb[cp]=0;
                    if(strstr(lb,fn)){
                        int col=(int)(strstr(lb,fn)-lb)+1;
                        debugger_display_syntax_error(sc->debugger,
                            fn,source,sc->source_file,ln,col-1,col-1+(int)strlen(fn));
                        free(source); return true;
                    }
                    ln++; if(!le)break; ls=le+1;
                }
            }
        }
    }

    const char *ep=strstr(error_output,"error:");
    if(ep && source){
        ep+=strlen("error:");
        while(*ep==' '||*ep=='\t')ep++;
        char msg[512]; int mi=0;
        while(*ep&&*ep!='\n'&&mi<511) msg[mi++]=(char)*ep++;
        msg[mi]=0;

        char ident[256]={0};
        const char *qi=strchr(msg,'\''); if(!qi)qi=strchr(msg,'`');
        if(qi){ qi++;
            const char *qie=strchr(qi,'\'');
            if(qie){int il=(int)(qie-qi);if(il<255){memcpy(ident,qi,(size_t)il);ident[il]=0;}}
        }
        if(ident[0] && source){
            const char *ls=source; int ln=1;
            while(*ls){
                const char *le=strchr(ls,'\n');
                int ll=le?(int)(le-ls):(int)strlen(ls);
                char lb[1024]; int cp=ll<1023?ll:1023;
                memcpy(lb,ls,(size_t)cp); lb[cp]=0;
                if(strstr(lb,ident)){
                    int col=(int)(strstr(lb,ident)-lb)+1;
                    debugger_display_syntax_error(sc->debugger,
                        msg,source,sc->source_file,ln,col-1,col-1+(int)strlen(ident));
                    free(source); return true;
                }
                ln++; if(!le)break; ls=le+1;
            }
        }
        fprintf(stderr,"%sError%s: %s\n",C_RED,C_RESET,msg);
        if(source) free(source);
        return true;
    }

    if(source) free(source);
    if(error_output[0])
        fprintf(stderr,"\n%sGCC Error%s\n%s\n",C_RED,C_RESET,error_output);
    return false;
}

static const char *compiler_find_msvc_cl(void){
    static char cl[512]={0};
    static const char *known[]={
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.39.33519\\bin\\Hostx64\\x64\\cl.exe",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.29.30133\\bin\\Hostx64\\x64\\cl.exe",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Tools\\MSVC\\14.39.33519\\bin\\Hostx64\\x64\\cl.exe",
        NULL
    };
    for(int i=0;known[i];i++) if(path_exists(known[i])){strncpy(cl,known[i],511);return cl;}
    /* where cl.exe */
    char buf[512]={0};
    if(run_cmd("where cl.exe",buf,sizeof(buf))==0&&buf[0]){
        char *nl=strchr(buf,'\n');if(nl)*nl=0;
        char *cr=strchr(buf,'\r');if(cr)*cr=0;
        snprintf(cl,512,"%s",buf); return cl;
    }
    return NULL;
}

static bool compiler_compile_msvc(StandaloneCompiler *sc){
    compiler_log(sc,"Trying MSVC...","Info");
    const char *cl=compiler_find_msvc_cl();
    if(!cl){ compiler_log(sc,"  MSVC not found!","Warn"); return false; }

    char runtime_c[512]={0};
    char runtime_part[256]="";
    {
        snprintf(runtime_c,sizeof(runtime_c),"lib\\cblerr_engine_runtime.c");
        if(path_exists(runtime_c))
            snprintf(runtime_part,sizeof(runtime_part)," \"%s\"",runtime_c);
    }

    char msvc_compile_flags[256]="/O2 /GS- /GR- /Zc:threadSafeInit- /Oi /Os /Gy";
    char msvc_link_flags[512]="/NODEFAULTLIB /INCREMENTAL:NO /OPT:REF /OPT:ICF /ALIGN:16";

    if(sc->is_gui_app)
        strncat(msvc_link_flags," /SUBSYSTEM:WINDOWS /ENTRY:CblerrStartup",
                sizeof(msvc_link_flags)-strlen(msvc_link_flags)-1);
    else
        strncat(msvc_link_flags," /SUBSYSTEM:CONSOLE /ENTRY:CblerrStartup",
                sizeof(msvc_link_flags)-strlen(msvc_link_flags)-1);

    if(sc->stack_reserve>0){
        char stk[64]; snprintf(stk,sizeof(stk)," /STACK:%ld",sc->stack_reserve);
        strncat(msvc_link_flags,stk,sizeof(msvc_link_flags)-strlen(msvc_link_flags)-1);
    }

    const char *libs="opengl32.lib winmm.lib kernel32.lib user32.lib msvcrt.lib gdi32.lib";

    char cmd[4096];
    snprintf(cmd,sizeof(cmd),
        "\"%s\" %s \"%s\"%s /Fe\"%s\" /link %s %s",
        cl,msvc_compile_flags,sc->c_file,runtime_part,
        sc->output_exe,msvc_link_flags,libs);

    compiler_log(sc,"  Starting: cl.exe for code compile...","Info");
    char err[16384]={0};
    int rc=run_cmd(cmd,err,sizeof(err));

    bool exe_found=path_exists(sc->output_exe);
#ifdef _WIN32
    if(!exe_found){
        char exe_path[512]; snprintf(exe_path,sizeof(exe_path),"%s.exe",sc->output_exe);
        exe_found=path_exists(exe_path);
    }
#endif

    if(rc==0&&exe_found){
        compiler_log(sc,"  Compiling via MSVC successful!","Info");
        bool keep=SAVE_C_FLAG||(getenv("CBLERR_KEEP_C")&&strcmp(getenv("CBLERR_KEEP_C"),"1")==0);
        if(keep){ char msg[512]; snprintf(msg,sizeof(msg),"  Leaving the C file because of the -c flag.: %.400s",sc->c_file); compiler_log(sc,msg,"Info"); }
        else{ remove(sc->c_file); }
        return true;
    }
    if(!compiler_handle_error(sc,err)){
        char msg[512]; snprintf(msg,sizeof(msg),"  Compile via MSVC unsuccessful!: %.300s",err);
        compiler_log(sc,msg,"Warn");
    }
    return false;
}

static bool compiler_compile_mingw(StandaloneCompiler *sc){
    compiler_log(sc,"Trying MinGW (gcc.exe)...","Info");

    char cflags[2048]; compiler_get_cflags(sc,cflags,sizeof(cflags));
    char ldflags[2048]; compiler_get_ldflags(sc,ldflags,sizeof(ldflags));

    const char *libs = sc->is_windows
        ? "-lopengl32 -lwinmm -lmsvcrt -lkernel32 -luser32 -lgdi32"
        : "-lm -lc";

    char cmd[8192];
    snprintf(cmd,sizeof(cmd),"gcc.exe %s \"%s\" -o \"%s\" %s %s",
             cflags,sc->c_file,sc->output_exe,ldflags,libs);

    compiler_log(sc,"  Starting: gcc.exe for code compile (Ultra mode)...","Info");
    char err[16384]={0};
    int rc=run_cmd(cmd,err,sizeof(err));

    bool exe_found=path_exists(sc->output_exe);
#ifdef _WIN32
    if(!exe_found){char ep[512];snprintf(ep,sizeof(ep),"%s.exe",sc->output_exe);exe_found=path_exists(ep);}
#endif

    if(rc==0&&exe_found){
        long sz=get_file_size(path_exists(sc->output_exe)?sc->output_exe:sc->output_exe);
        char msg[256]; snprintf(msg,sizeof(msg),"Size of the resulting file: %.2f Kilobytes",(double)sz/1024.0);
        compiler_log(sc,msg,"Info");
        bool keep=SAVE_C_FLAG||(getenv("CBLERR_KEEP_C")&&strcmp(getenv("CBLERR_KEEP_C"),"1")==0);
        if(keep){ char m2[512]; snprintf(m2,sizeof(m2),"  Leave a temporary C file: %.440s",sc->c_file); compiler_log(sc,m2,"Info"); }
        else{ remove(sc->c_file); }
        return true;
    }
    if(!compiler_handle_error(sc,err)){
        char msg[512]; snprintf(msg,sizeof(msg),"  Compilation via MinGW failed!: %.300s",err);
        compiler_log(sc,msg,"Warn");
    }
    return false;
}

static bool compiler_compile_gcc(StandaloneCompiler *sc){
    compiler_log(sc,"Trying GCC (Linux)...","Info");

    char cflags[2048]; compiler_get_cflags(sc,cflags,sizeof(cflags));
    char ldflags[2048]; compiler_get_ldflags(sc,ldflags,sizeof(ldflags));
    const char *libs="-lm -lc";

    char cmd[8192];
    snprintf(cmd,sizeof(cmd),"gcc %s \"%s\" -o \"%s\" %s %s",
             cflags,sc->c_file,sc->output_exe,ldflags,libs);

    compiler_log(sc,"  Running: gcc to compile the code...","Info");
    char err[16384]={0};
    int rc=run_cmd(cmd,err,sizeof(err));

    if(rc==0&&path_exists(sc->output_exe)){
        compiler_log(sc,"  Compilation via GCC successful!","Info");
        return true;
    }
    if(!compiler_handle_error(sc,err)){
        char msg[512]; snprintf(msg,sizeof(msg),"  Error compile via GCC: %.300s",err);
        compiler_log(sc,msg,"Warn");
    }
    return false;
}

static bool compiler_compile_clang(StandaloneCompiler *sc){
    compiler_log(sc,"Trying Clang...","Info");

    char cflags[2048]; compiler_get_cflags(sc,cflags,sizeof(cflags));
    char ldflags[2048]; compiler_get_ldflags(sc,ldflags,sizeof(ldflags));
    const char *libs = sc->is_windows
        ? "-lopengl32 -lwinmm -lkernel32 -luser32 -lmsvcrt -lgdi32"
        : "-lm -lc";

    const char *clang_cmd = IS_WINDOWS ? "clang.exe" : "clang";

    char cmd[8192];
    snprintf(cmd,sizeof(cmd),"%s %s \"%s\" -o \"%s\" %s %s",
             clang_cmd,cflags,sc->c_file,sc->output_exe,ldflags,libs);

    char step_msg[256]; snprintf(step_msg,sizeof(step_msg),"  Starting: %s for code compile...",clang_cmd);
    compiler_log(sc,step_msg,"Info");

    char err[16384]={0};
    int rc=run_cmd(cmd,err,sizeof(err));

    bool exe_found=path_exists(sc->output_exe);
#ifdef _WIN32
    if(!exe_found){char ep[512];snprintf(ep,sizeof(ep),"%s.exe",sc->output_exe);exe_found=path_exists(ep);}
#endif

    if(rc==0&&exe_found){
        compiler_log(sc,"  Compile via Clang successful!","Info");
        return true;
    }
    if(!compiler_handle_error(sc,err)){
        char msg[512]; snprintf(msg,sizeof(msg),"  Compile via Clang unsuccessful: %.300s",err);
        compiler_log(sc,msg,"Warn");
    }
    return false;
}

static bool compiler_compile_lld(StandaloneCompiler *sc){
    compiler_log(sc,"Trying LLD...","Info");
    return compiler_compile_clang(sc);
}

static bool compiler_compile_c_to_exe(StandaloneCompiler *sc){
    const char *ct=sc->compiler_type;

    if(strcmp(ct,"gcc")==0){
        if(sc->is_windows) return compiler_compile_mingw(sc);
        else               return compiler_compile_gcc(sc);
    }
    if(strcmp(ct,"clang")==0) return compiler_compile_clang(sc);
    if(strcmp(ct,"lld")  ==0) return compiler_compile_lld(sc);
    if(strcmp(ct,"mingw")==0) return compiler_compile_mingw(sc);
    if(strcmp(ct,"msvc") ==0) return compiler_compile_msvc(sc);

    if(sc->is_windows){
        return compiler_compile_msvc(sc) || compiler_compile_mingw(sc);
    } else {
        return compiler_compile_gcc(sc)   ||
               compiler_compile_clang(sc) ||
               compiler_compile_lld(sc);
    }
}

static bool compiler_compile(StandaloneCompiler *sc){
    GameDebugger *dbg = debugger_init(DBG_INFO, NULL);
    sc->debugger = dbg;

    compiler_log_info(sc,"CBlerr Console Compiler");
    compiler_log_info(sc,"OS: %s", sc->system_name);
    compiler_log_info(sc,"Output file: %s", sc->output_exe);

    bool ok = false;
    char *source = NULL;

    /* ── [1/4] Read source ── */
    compiler_log_step(sc,1,4,"Reading Code...");
    if(!path_exists(sc->source_file)){
        char msg[1024]; snprintf(msg,sizeof(msg),"File not found!: %s",sc->source_file);
        compiler_log(sc,msg,"Error");
        goto cleanup;
    }
    source = read_file_alloc(sc->source_file);
    if(!source){ compiler_log(sc,"Cannot read source file","Error"); goto cleanup; }
    {
        char msg[2048];
        snprintf(msg,sizeof(msg),"  Readed %zu bytes from %s",(unsigned long)strlen(source),sc->source_file);
        compiler_log_info(sc,"%s",msg);
    }

    /* ── [2/4] Tokenize ── */
    compiler_log_step(sc,2,4,"Tokenization...");
    int tok_count=0;
    Token *tokens = tokenize(source,&tok_count);
    {
        char msg[128]; snprintf(msg,sizeof(msg),"  Generated %d tokens",tok_count);
        compiler_log_info(sc,"%s",msg);
    }

    /* ── [3/4] Parse ── */
    compiler_log_step(sc,3,4,"Code Parsing...");
    Program *prog = parse(tokens,tok_count);
    tokens_free(tokens,tok_count);
    compiler_log_info(sc,"  AST Created successful!");

    /* inline_imports(ast, self.source_file)) */
    if(module_inline_imports(prog,sc->source_file)!=0){
        char msg[512]; snprintf(msg,sizeof(msg),"Error \"Import\": failed to resolve");
        compiler_log(sc,msg,"Error");
        goto cleanup;
    }

    /* ast.functions: if fn.name=='WinMain') */
    for(int i=0;i<prog->funcs.len;i++){
        if(prog->funcs.data[i].name&&strcmp(prog->funcs.data[i].name,"WinMain")==0)
            sc->is_gui_app=true;
    }
    {
        FuncDef *main_fn=NULL;
        for(int i=0;i<prog->funcs.len;i++)
            if(prog->funcs.data[i].name&&strcmp(prog->funcs.data[i].name,"main")==0)
                {main_fn=&prog->funcs.data[i];break;}
        if(main_fn&&!main_fn->is_extern){
            bool found_return0=false,found_endofcode=false;
            for(int i=0;i<main_fn->body.len;i++){
                AstNode *s=main_fn->body.data[i];
                if(s&&s->kind==ND_RETURN){
                    if(s->ret.value&&s->ret.value->kind==ND_LITERAL_INT&&s->ret.value->ival==0)
                        found_return0=true;
                    if(s->ret.is_endofcode) found_endofcode=true;
                }
            }
            if(found_return0&&!found_endofcode&&DERR_FLAG)
                compiler_log_info(sc,"Tip: use \"endofcode\" instead of \"return 0\".");
        }
    }

    monomorphize(prog);

    {
        TypeChecker tc; tc_init(&tc,dbg);
        bool tc_ok=tc_check(&tc,prog);
        if(!tc_ok&&DERR_FLAG){
            debugger_critical_dump(dbg,"TypeError",tc.error_msg);
        }
        tc_free(&tc);
    }

    /* ── [4/4] Generate C ── */
    compiler_log_step(sc,4,4,"Generated code...");
    char *c_code = codegen_generate(prog);
    {
        char msg[128]; snprintf(msg,sizeof(msg),"  Generated %zu bytes C-code.",strlen(c_code));
        compiler_log_info(sc,"%s",msg);
    }
    if(!write_file_str(sc->c_file,c_code)){
        compiler_log(sc,"Cannot write C file","Error");
        free(c_code); goto cleanup;
    }
    {
        char msg[512]; snprintf(msg,sizeof(msg),"  C-code saved to:%.450s",sc->c_file);
        compiler_log_info(sc,"%s",msg);
    }
    free(c_code);

    /* ── [5/5] Compile C to exe ── */
    compiler_log_step(sc,5,5,"Compiling C-code to executable file...");
    ok = compiler_compile_c_to_exe(sc);

    if(ok) printf("%sCompilation Successfully!%s\n",C_GREEN,C_RESET);

cleanup:
    if(source) free(source);
    return ok;
}

static bool compiler_init(StandaloneCompiler *sc,
                            const char *source_file,
                            const char *output_exe,
                            bool verbose,
                            const char *link_mode,
                            long stack_reserve,
                            const char *forced_compiler){
    memset(sc,0,sizeof(StandaloneCompiler));
    strncpy(sc->source_file,source_file,sizeof(sc->source_file)-1);
    snprintf(sc->output_exe, sizeof(sc->output_exe), "%s", output_exe);
    sc->verbose       = verbose;
    sc->stack_reserve = stack_reserve;
    sc->is_gui_app    = false;
    sc->debugger      = NULL;

#ifdef _WIN32
    sc->is_windows=true; strncpy(sc->system_name,"Windows",sizeof(sc->system_name)-1);
#else
    sc->is_windows=false;
    /* detect Linux / Darwin */
#  ifdef __APPLE__
    strncpy(sc->system_name,"Darwin",sizeof(sc->system_name)-1);
#  else
    strncpy(sc->system_name,"Linux",sizeof(sc->system_name)-1);
#  endif
#endif

    if(g_target_os){
        if(strcmp(g_target_os,"windows")==0) sc->is_windows=true;
        if(strcmp(g_target_os,"linux")==0)   sc->is_windows=false;
    }

    char stem[256]; path_stem(source_file,stem,sizeof(stem));
    make_tmp_c(stem,sc->c_file,sizeof(sc->c_file));

    if(!path_exists(sc->source_file)) {
#ifndef _WIN32
        char exe_path[512]={0};
        ssize_t exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
        if(exe_len > 0) {
            exe_path[exe_len]=0;
            /* get directory of binary */
            char *last_slash = strrchr(exe_path, '/');
            if(last_slash) *last_slash = 0;
            /* try: bin_dir/source, bin_dir/../source, bin_dir/../../source */
            char candidate[512];
            snprintf(candidate,sizeof(candidate),"%s/%s",exe_path,source_file);
            if(path_exists(candidate)){ snprintf(sc->source_file,sizeof(sc->source_file),"%s",candidate); }
            else {
                snprintf(candidate,sizeof(candidate),"%s/../%s",exe_path,source_file);
                if(path_exists(candidate)){ snprintf(sc->source_file,sizeof(sc->source_file),"%s",candidate); }
                else {
                    snprintf(candidate,sizeof(candidate),"%s/../../%s",exe_path,source_file);
                    if(path_exists(candidate)){ snprintf(sc->source_file,sizeof(sc->source_file),"%s",candidate); }
                }
            }
        }
#endif
    }

    /* link mode */
    if(link_mode) strncpy(sc->link_mode,link_mode,sizeof(sc->link_mode)-1);

    /* self.compiler_type = self._select_compiler(compiler_type)) */
    return compiler_select(sc,forced_compiler);
}

static long parse_stack_size(const char *raw){
    if(!raw) return 0;
    char s[64]; strncpy(s,raw,sizeof(s)-1); s[sizeof(s)-1]=0;
    int last=(int)strlen(s)-1;
    if(last>=0){
        char c=(char)toupper((unsigned char)s[last]);
        if(c=='M'){ s[last]=0; return (long)(atof(s)*1024*1024); }
        if(c=='K'){ s[last]=0; return (long)(atof(s)*1024); }
    }
    return atol(s);
}

static void print_usage(void){
    printf("Usage: CBlerr <source_file.cbl> [options]\n");
    printf("Options:\n");
    printf("  -o <file>              Path to the output executable\n");
    printf("  -t <target>            Target platform: Windows or Linux\n");
    printf("  --verbose              Show verbose output\n");
    printf("  -c                     Save the generated C file\n");
    printf("  -static                Force static linking\n");
    printf("  -dynamic               Force dynamic linking\n");
    printf("  --stack-size <N[K|M]>  Reserve stack size\n");
    printf("  --gcc                  Use the GCC compiler\n");
    printf("  --clang                Use the Clang compiler\n");
    printf("  --lld                  Use Clang + LLD linker\n");
    printf("  --mingw                Use MinGW (GCC for Windows)\n");
    printf("  --msvc                 Use MSVC cl.exe (Windows only)\n");
    printf("  -derr                  Verbose debug dump on error\n");
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    printf("CBlerr in C  -> https://github.com/Mavox-ID/C-CBlerr\n");
    printf("CBlerr Orig. -> https://github.com/Tankman02/CBlerr\n");
    printf("License MIT  -> https://github.com/Mavox-ID/C-CBlerr/blob/main/LICENSE\n");
}

int main(int argc, char **argv){
    dbg_init(true);

    extract_cli_flags(&argc,argv);

    if(argc<2){ print_usage(); return 1; }

    const char *source_file   = argv[1];
    const char *output_exe    = NULL;
    const char *target        = NULL;
    bool        verbose       = true;
    const char *link_mode     = NULL;
    long        stack_size    = 0;
    const char *compiler_type = NULL;

    int i=2;
    while(i<argc){
        const char *a=argv[i];
        if(strcmp(a,"-o")==0&&i+1<argc){     output_exe=argv[++i]; i++; }
        else if(strcmp(a,"-t")==0&&i+1<argc){target=argv[++i]; g_target_os=target; i++; }
        else if(strcmp(a,"--verbose")==0){   verbose=true; i++; }
        else if(strcmp(a,"-static")==0){     link_mode="static"; g_static_link=true; i++; }
        else if(strcmp(a,"-dynamic")==0){    link_mode="dynamic"; g_dynamic_link=true; i++; }
        else if(strcmp(a,"--gcc")==0){       compiler_type="gcc"; i++; }
        else if(strcmp(a,"--clang")==0){     compiler_type="clang"; i++; }
        else if(strcmp(a,"--lld")==0){       compiler_type="lld"; i++; }
        else if(strcmp(a,"--mingw")==0){     compiler_type="mingw"; i++; }
        else if(strcmp(a,"--msvc")==0){      compiler_type="msvc"; i++; }
        else if(strcmp(a,"--stack-size")==0&&i+1<argc){stack_size=parse_stack_size(argv[++i]);i++;}
        else i++;
    }

    char out_buf[512];
    if(!output_exe){
        char stem[256]; path_stem(source_file,stem,sizeof(stem));
        bool win_target = IS_WINDOWS;
        if(target) win_target=(strcmp(target,"windows")==0);
        snprintf(out_buf,sizeof(out_buf),"%s%s",stem,win_target?".exe":"");
        output_exe=out_buf;
    }

    StandaloneCompiler sc;
    if(!compiler_init(&sc,source_file,output_exe,verbose,
                      link_mode,stack_size,compiler_type)){
        return 1;
    }

    bool success = compiler_compile(&sc);

    if(success){
        printf("Executable file: %s\n",sc.output_exe);
        return 0;
    } else {
        printf("\nCompile Error!\n");
        return 1;
    }
}
