/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#pragma once
#include "ast.h"

int module_inline_imports(Program *prog, const char *source_path);

/* _resolve_module_path() if _resolve_module_path() in module_loader.c from .py */
/*Bro, add this please ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
bool module_resolve_path(const char *module_name, const char *base_dir,
                          char *out_path, int out_sz);
