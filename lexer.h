#ifndef LEXER_H
#define LEXER_H

#include "ast.h"

// Tokeniza una línea en SAN y rellena TokenList.
// Devuelve 0 si OK, -1 si hubo error léxico (ej. línea vacía).
int tokenize(const char *line, TokenList *out);

// devuelve el nombre textual del token (útil para debugging)
const char* token_name(TokenType t);

#endif // LEXER_H
