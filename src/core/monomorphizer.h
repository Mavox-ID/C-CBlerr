/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#pragma once
/*
 * I implements generic function/struct instantiation:
 *   stringify_type()
 *   type_to_str()
 *   collect_placeholders_from_type()
 *   collect_placeholders_from_func()
 *   replace_types_in_node()
 *   monomorphize()
 *   clone_and_instantiate_function()
 *   clone_and_instantiate_struct()
 */
#include "ast.h"
#include <stdbool.h>
char *mono_stringify_type(const char *t);

void mono_collect_placeholders_type(const char *t,
                                     char **acc, int *n_acc, int max_acc);

void mono_collect_placeholders_func(FuncDef *f,
                                     char **acc, int *n_acc, int max_acc);

void mono_replace_types(AstNode *n,
                         const char **keys,
                         const char **vals,
                         int          n_mapping);

void mono_replace_types_body(NodeVec *body,
                              const char **keys,
                              const char **vals,
                              int          n);

void monomorphize(Program *prog);

FuncDef   *mono_find_function(Program *prog, const char *name);
StructDef *mono_find_struct  (Program *prog, const char *name);
char *mono_resolve_type(const char *t, const char **keys, const char **vals, int n);

/* visit()   */
void mono_visit         (Program *prog, AstNode *node, char **done_names, int *n_done);
void mono_visit_function(Program *prog, FuncDef *f,    char **done_names, int *n_done);
void mono_visit_program (Program *prog);
