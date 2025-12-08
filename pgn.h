// pgn.h - Header para modo de análisis PGN
#ifndef PGN_H
#define PGN_H

#include "ast.h"
#include "semant.h"

// ============================================================================
// ESTRUCTURAS
// ============================================================================

// Representa un movimiento con su estado de tablero
typedef struct {
    char move_text[64];      // Texto del movimiento (ej: "Nf3")
    MoveAST ast;             // AST del movimiento parseado
    Board board_state;       // Estado del tablero DESPUÉS del movimiento
    Color side_to_move;      // Color que hizo el movimiento
} GameMove;

// Representa una partida completa de ajedrez
typedef struct {
    char event[256];         // Nombre del evento
    char white[128];         // Nombre del jugador blanco
    char black[128];         // Nombre del jugador negro
    char result[16];         // Resultado (1-0, 0-1, 1/2-1/2, *)
    GameMove *moves;         // Array dinámico de movimientos
    int move_count;          // Cantidad de movimientos
    int move_capacity;       // Capacidad del array
} PGNGame;

// Colección de múltiples partidas
typedef struct {
    PGNGame *games;          // Array dinámico de partidas
    int game_count;          // Cantidad de partidas
    int game_capacity;       // Capacidad del array
} PGNCollection;

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

// Inicializa una partida PGN
void pgn_game_init(PGNGame *game);

// Libera memoria de una partida PGN
void pgn_game_free(PGNGame *game);

// Inicializa una colección de partidas
void pgn_collection_init(PGNCollection *col);

// Libera memoria de una colección
void pgn_collection_free(PGNCollection *col);

// Ejecuta el modo PGN completo (carga, selección y replay)
// Retorna 0 en éxito, -1 en error
int pgn_mode(const char *path);

#endif // PGN_H