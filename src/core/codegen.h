/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#pragma once
#include "ast.h"

typedef struct {
    char   *buf;
    int     len, cap;
    int     indent;
    char  **dyn_names;
    char  **dyn_vals;
    int     dyn_count, dyn_cap;
    bool    is_gui_app;
    bool    is_windows;
} CodeGen;

char *codegen_generate(Program *prog);

void  codegen_emit_line                (CodeGen *cg, const char *s);
void  codegen_emit_block               (CodeGen *cg, const char *s);
char *codegen_get_c_type               (const char *flux_type);
char *codegen_get_c_declaration        (const char *flux_type, const char *varname);
void  codegen_generate_struct_def      (CodeGen *cg, StructDef *s);
void  codegen_generate_global_var      (CodeGen *cg, GlobalVar *g);
void  codegen_generate_function_signature(CodeGen *cg, FuncDef *f, char *out, int out_sz);
void  codegen_generate_function_def    (CodeGen *cg, FuncDef *f);
void  codegen_generate_statement       (CodeGen *cg, AstNode *stmt);
void  codegen_generate_return          (CodeGen *cg, AstNode *n);
void  codegen_generate_assign          (CodeGen *cg, AstNode *n);
void  codegen_generate_if              (CodeGen *cg, AstNode *n);
void  codegen_generate_while           (CodeGen *cg, AstNode *n);
void  codegen_generate_for             (CodeGen *cg, AstNode *n);
void  codegen_generate_match           (CodeGen *cg, AstNode *n);
void  codegen_generate_enum            (CodeGen *cg, StructDef *s);
char *codegen_generate_expression      (CodeGen *cg, AstNode *expr);
