#ifndef BOARD_H
#define BOARD_H

#include "ast.h"

// Color de la pieza
typedef enum {
    COLOR_NONE = 0,
    COLOR_WHITE,
    COLOR_BLACK
} Color;

// Tipo de pieza
typedef enum {
    PIECE_NONE = 0,
    PIECE_PAWN,
    PIECE_KNIGHT,
    PIECE_BISHOP,
    PIECE_ROOK,
    PIECE_QUEEN,
    PIECE_KING
} PieceType;


// Pieza en una casilla
typedef struct {
    Color color;
    PieceType type;
} Piece;


typedef struct {
    Piece board[8][8];

    // Validación de enroques
    int white_can_castle_short;
    int white_can_castle_long;
    int black_can_castle_short;
    int black_can_castle_long;

    // Variable de en passant
    int en_passant_file;
    int en_passant_rank;
} Board;

// Estado de la posición
typedef enum {
    POSITION_NORMAL,
    POSITION_CHECK,
    POSITION_CHECKMATE,
    POSITION_STALEMATE
} PositionStatus;

PositionStatus board_evaluate_status(const Board *b, Color side_to_move);


// Convierte a index
int file_to_index(char file);
int rank_to_index(char rank);

// Inicializa el tablero
void board_init_start(Board *b);

void board_init_stalemate_test(Board *b);

// Imprime el tablero 
void board_print(const Board *b);

/* Aplica un movimiento ya parseado (MoveAST) al tablero.
   side_to_move indica a quién le toca (COLOR_WHITE o COLOR_BLACK).
   Si el movimiento es ilegal, devuelve -1 y escribe un mensaje en error_msg.
   Si es legal, devuelve 0 y modifica el tablero. */
int board_apply_move(Board *b,
                     const MoveAST *mv,
                     Color side_to_move,
                     char *error_msg,
                     size_t error_msg_size);

#endif 