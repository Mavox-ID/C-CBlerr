#pragma once
#include <stdbool.h>

void dbg_init(bool color);
void dbg_info(const char *fmt, ...);
void dbg_warn(const char *fmt, ...);
void dbg_error(const char *fmt, ...);
void dbg_syntax_error(const char *msg, const char *source,
                      int lineno, int col_start, int col_end);

/* Levenshtein suggestion */
const char *dbg_suggest(const char *unknown);
