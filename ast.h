#ifndef AST_H
#define AST_H

#include <stddef.h>

// Definiciones para el análisis sintáctico de movimientos de ajedrez
typedef enum {
    TK_UNKNOWN,
    TK_PIECE,        // K Q R B N
    TK_FILE,         // a-h
    TK_RANK,         // 1-8
    TK_CAPTURE,      // x
    TK_PROMOTE,      // =
    TK_PROMOTE_PIECE,// piece after =
    TK_CHECK,        // +
    TK_MATE,         // #
    TK_CASTLE_SHORT, // O-O or 0-0
    TK_CASTLE_LONG,  // O-O-O or 0-0-0
    TK_END
} TokenType;

// Es cada token reconocido
typedef struct {
    TokenType type;
    char text[8]; // textual value (suficiente para "O-O-O")
} Token;


// Lista dinámica de tokens
typedef struct {
    Token *items;
    size_t count;
    size_t cap;
} TokenList;

// Nodo AST para un movimiento de ajedrez
typedef struct {
    char piece;        // 'K','Q','R','B','N' o 'P' para peón
    char src_file;     // 'a'..'h' o 0
    char src_rank;     // '1'..'8' o 0
    char dest_file;    // 'a'..'h'
    char dest_rank;    // '1'..'8'
    char promotion;    // 'Q','R','B','N' o 0
    int is_capture;
    int is_castle_short;
    int is_castle_long;
    int is_check;
    int is_mate;
    char raw[64];
} MoveAST;

// helpers para lista de tokens
void tokenlist_init(TokenList *tl);
void tokenlist_free(TokenList *tl);
void tokenlist_push(TokenList *tl, Token t);

#endif // AST_H

