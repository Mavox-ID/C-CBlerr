#pragma once
#include "ast.h"
#include "lexer.h"

Program *parse(Token *tokens, int count);
