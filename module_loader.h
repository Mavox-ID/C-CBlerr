#pragma once
#include "ast.h"

/* Resolves and inlines all import statements into prog.
   source_path is the path of the file being compiled. */
int module_inline_imports(Program *prog, const char *source_path);
