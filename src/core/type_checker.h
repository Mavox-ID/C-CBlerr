/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#pragma once
/*
 * Provides semantic analysis: type inference, Top-Down Rule,
 * return-type checking, pointer-safety checks, reserved-function
 * redefinition guard, and struct field resolution.
 */

/* Yeah, I'm created this :> */
/* Try to run 'make' or build.bat (Not open a source code :> ^-^) */
#include "ast.h"
#include "debugger.h"
#include <stdbool.h>

char *type_to_str(const char *t);
bool  type_is_int_family(const char *t);

#define SYMTAB_CAP 512
typedef struct { char *key; char *val; } SymEntry;
typedef struct {
    SymEntry entries[SYMTAB_CAP];
    int      count;
} SymTab;

void        symtab_init (SymTab *s);
void        symtab_free (SymTab *s);
void        symtab_set  (SymTab *s, const char *key, const char *val);
const char *symtab_get  (SymTab *s, const char *key);  /* NULL? -_- */
void        symtab_copy (SymTab *dst, const SymTab *src);

typedef SymTab OriginTab;

#define FUNCSIG_CAP 512
typedef struct {
    char  *name;
    char **param_types;   /* NULL double killer) */
    int    n_params;
    char  *return_type;
    bool   is_vararg;
} FuncSig;
typedef struct {
    FuncSig entries[FUNCSIG_CAP];
    int     count;
} FuncSigTab;

void        funcsig_init   (FuncSigTab *t);
void        funcsig_free   (FuncSigTab *t);
void        funcsig_add    (FuncSigTab *t, const char *name,
                             char **params, int n_params,
                             const char *ret, bool is_vararg);
FuncSig    *funcsig_get    (FuncSigTab *t, const char *name);

#define STRUCTFIELDS_CAP 256
typedef struct {
    char *struct_name;
    char **field_names;
    char **field_types;
    int    n_fields;
} StructFields;
typedef struct {
    StructFields entries[STRUCTFIELDS_CAP];
    int          count;
} StructFieldTab;

void          sftab_init(StructFieldTab *t);
void          sftab_free(StructFieldTab *t);
void          sftab_add (StructFieldTab *t, const char *sname,
                          char **fnames, char **ftypes, int n);
StructFields *sftab_get (StructFieldTab *t, const char *sname);

typedef struct {
    char message[2048];
} SemanticError;

typedef struct {
    GameDebugger   *debugger;
    StructFieldTab  struct_fields;
    FuncSigTab      functions;
    SymTab          globals;
    char           *current_return_type;
    bool            had_error;
    char            error_msg[2048];
} TypeChecker;

void     tc_init   (TypeChecker *tc, GameDebugger *dbg);
void     tc_free   (TypeChecker *tc);
bool     tc_check  (TypeChecker *tc, Program *prog);
bool     tc_check_expr (TypeChecker *tc, AstNode *expr,
                         SymTab *symbols, OriginTab *origins,
                         char *out_type, int out_sz);
bool     tc_check_stmt (TypeChecker *tc, AstNode *stmt,
                         SymTab *symbols, OriginTab *origins,
                         const char *func_ret);
