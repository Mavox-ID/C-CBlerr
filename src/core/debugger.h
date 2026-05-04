/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum { DBG_NONE=0,DBG_ERROR=1,DBG_WARNING=2,DBG_INFO=3,DBG_VERBOSE=4,DBG_TRACE=5 } DebugLevel;

#define STACKFRAME_MAX_CODE 512
#define STACKFRAME_MAX_PATH 512
#define STACKFRAME_MAX_FUNC 256
typedef struct { char filename[STACKFRAME_MAX_PATH]; char function[STACKFRAME_MAX_FUNC]; int lineno; char code[STACKFRAME_MAX_CODE]; } StackFrame;

#define CRASH_MAX_FRAMES 64
typedef struct { char timestamp[64]; char exception_type[256]; char exception_message[2048]; StackFrame stack_frames[CRASH_MAX_FRAMES]; int n_frames; double elapsed_time; long mem_rss_kb; } CrashContext;

#define DEBUGGER_MAX_LOGPATH 512
typedef struct { DebugLevel level; char log_file[DEBUGGER_MAX_LOGPATH]; bool use_colors; long max_log_size; double start_time; int error_count; int warning_count; } GameDebugger;

void          dbg_init(bool color);
GameDebugger *debugger_init(DebugLevel level, const char *log_file);
GameDebugger *debugger_get(void);

void debugger_log_error  (GameDebugger *d, const char *fmt, ...);
void debugger_log_warning(GameDebugger *d, const char *fmt, ...);
void debugger_log_info   (GameDebugger *d, const char *fmt, ...);
void debugger_log_verbose(GameDebugger *d, const char *fmt, ...);
void debugger_log_trace  (GameDebugger *d, const char *fmt, ...);

void debugger_critical_dump(GameDebugger *d, const char *exc_type, const char *exc_msg);
CrashContext debugger_capture_context(GameDebugger *d, const char *exc_type, const char *exc_msg, StackFrame *frames, int n_frames);
void debugger_print_summary(GameDebugger *d);
void debugger_watch_memory(GameDebugger *d, void *address);
void debugger_display_syntax_error(GameDebugger *d, const char *msg, const char *source, const char *filename, int lineno, int col_start, int col_end);

int         dbg_levenshtein(const char *a, const char *b);
double      dbg_token_similarity(const char *unknown, const char *candidate);
const char *dbg_find_closest_match(const char *unknown, int max_distance);

bool        dbg_color_supported(void);
const char *dbg_color_red(void);
const char *dbg_color_yellow(void);
const char *dbg_color_blue(void);
const char *dbg_color_cyan(void);
const char *dbg_color_reset(void);

#define DBG_ERROR(...)   debugger_log_error  (debugger_get(), __VA_ARGS__)
#define DBG_WARN(...)    debugger_log_warning(debugger_get(), __VA_ARGS__)
#define DBG_INFO(...)    debugger_log_info   (debugger_get(), __VA_ARGS__)
#define DBG_VERBOSE(...) debugger_log_verbose(debugger_get(), __VA_ARGS__)
#define DBG_TRACE(...)   debugger_log_trace  (debugger_get(), __VA_ARGS__)

void debugger_colorize(GameDebugger *d, const char *text, const char *color_code, char *out, int out_sz);
GameDebugger *debugger_enter(GameDebugger *d);
void debugger_exit(GameDebugger *d, const char *exc_type, const char *exc_msg);
void color_red_str   (const char *text, char *out, int sz);
void color_blue_str  (const char *text, char *out, int sz);
void color_yellow_str(const char *text, char *out, int sz);
