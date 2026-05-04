/* Rewrited by Mavox-ID | License -> MIT */
/* https://github.com/Mavox-ID/C-CBlerr  */
/* Original CBlerr by Tankman02 ->       */
/* https://github.com/Tankman02/CBlerr   */

#pragma once
#include "ast.h"
#include "lexer.h"

Program *parse(Token *tokens, int count);
AstNode *parse_atom_or_access_simple(Token *tokens, int count, int *pos);
bool scan_postfix_for_assignment(Token *tokens, int count, int start_pos, int *out_end);
char *parse_type_str(Token *tokens, int count, int *pos);
