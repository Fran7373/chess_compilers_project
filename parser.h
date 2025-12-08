#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

// Parsea una lista de tokens (generada por tokenize) y llena MoveAST.
// Devuelve 0 en éxito, -1 en error de parseo (y escribe motivo en stderr).
int parse_move(const TokenList *tokens, MoveAST *out);

#endif // PARSER_H
