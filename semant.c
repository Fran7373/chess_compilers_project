#include <stdio.h>
#include <string.h>
#include "semant.h"


// Convierte columna de caracter a índice
int file_to_index(char file_char) {
    if (file_char < 'a' || file_char > 'h') {
        return -1;
    }
    return file_char - 'a';
}

// Convierte fila de carácter a índice
int rank_to_index(char rank_char) {
    if (rank_char < '1' || rank_char > '8') {
        return -1;
    }
    return rank_char - '1';
}

// Mueve una pieza a una casilla (rank, file)
static void set_piece(Board *b, int rank, int file, Color color, PieceType type) {
    b->board[rank][file].color = color;
    b->board[rank][file].type = type;
}

// Inicializa el tablero en la posición inicial estándar de ajedrez
void board_init_start(Board *b) {
    if (!b) return;

    memset(b->board, 0, sizeof(b->board)); // 1) Vaciar el tablero

    // 2) Poner piezas blancas
    int r;      // columna
    int f;      // fila

    r = 1;
    for (f = 0; f < 8; ++f) {
        set_piece(b, r, f, COLOR_WHITE, PIECE_PAWN); // Pone los peones blancos
    }

    r = 0;      // Pone piezas mayores blancas
    set_piece(b, r, 0, COLOR_WHITE, PIECE_ROOK);
    set_piece(b, r, 1, COLOR_WHITE, PIECE_KNIGHT);
    set_piece(b, r, 2, COLOR_WHITE, PIECE_BISHOP);
    set_piece(b, r, 3, COLOR_WHITE, PIECE_QUEEN);
    set_piece(b, r, 4, COLOR_WHITE, PIECE_KING);
    set_piece(b, r, 5, COLOR_WHITE, PIECE_BISHOP);
    set_piece(b, r, 6, COLOR_WHITE, PIECE_KNIGHT);
    set_piece(b, r, 7, COLOR_WHITE, PIECE_ROOK);

    // 3) Poner piezas negras
    r = 6;
    for (f = 0; f < 8; ++f) {
        set_piece(b, r, f, COLOR_BLACK, PIECE_PAWN); // Pone los peones negros
    }

    r = 7;      // Pone piezas mayores negras
    set_piece(b, r, 0, COLOR_BLACK, PIECE_ROOK);
    set_piece(b, r, 1, COLOR_BLACK, PIECE_KNIGHT);
    set_piece(b, r, 2, COLOR_BLACK, PIECE_BISHOP);
    set_piece(b, r, 3, COLOR_BLACK, PIECE_QUEEN);
    set_piece(b, r, 4, COLOR_BLACK, PIECE_KING);
    set_piece(b, r, 5, COLOR_BLACK, PIECE_BISHOP);
    set_piece(b, r, 6, COLOR_BLACK, PIECE_KNIGHT);
    set_piece(b, r, 7, COLOR_BLACK, PIECE_ROOK);

    // 4) Variables de derechos de enroque
    b->white_can_castle_short = 1;
    b->white_can_castle_long  = 1;
    b->black_can_castle_short = 1;
    b->black_can_castle_long  = 1;

    b->en_passant_file = -1;
    b->en_passant_rank = -1;
}

// Inicializa el tablero en una posición de ahogado para pruebas
void board_init_stalemate_test(Board *b)
{
    // Vaciar todo
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            b->board[r][f].type  = PIECE_NONE;
            b->board[r][f].color = COLOR_NONE;
        }
    }

    // Colocar rey blanco en h6 (fila 5, columna 7)
    b->board[5][7].type  = PIECE_KING;
    b->board[5][7].color = COLOR_WHITE;

    // Colocar dama blanca en g6 (fila 5, columna 6)
    b->board[5][6].type  = PIECE_QUEEN;
    b->board[5][6].color = COLOR_WHITE;

    // Colocar rey negro en h8 (fila 7, columna 7)
    b->board[7][7].type  = PIECE_KING;
    b->board[7][7].color = COLOR_BLACK;

    // Sin enroques ni en passant
    b->white_can_castle_short = 0;
    b->white_can_castle_long  = 0;
    b->black_can_castle_short = 0;
    b->black_can_castle_long  = 0;
    b->en_passant_file = -1;
    b->en_passant_rank = -1;
}


// Convierte el char de MoveAST.piece a enum PieceType
static PieceType piece_type_from_char(char c) {
    switch (c) {
        case 'P': return PIECE_PAWN;
        case 'N': return PIECE_KNIGHT;
        case 'B': return PIECE_BISHOP;
        case 'R': return PIECE_ROOK;
        case 'Q': return PIECE_QUEEN;
        case 'K': return PIECE_KING;
        default:  return PIECE_NONE;
    }
}

// Valida el movimiento de un caballo 
// 1 = válido
// 0 = inválido
static int can_knight_move(const Board *b,
                           int sr, int sf,  //source rank/file
                           int dr, int df,  //dest rank/file
                           int is_capture,
                           Color side_to_move)
{
    int d_rank = dr - sr;
    int d_file = df - sf;
    if (d_rank < 0) d_rank = -d_rank;
    if (d_file < 0) d_file = -d_file;

    /* patrón en L: (2,1) o (1,2) */
    if (!((d_rank == 2 && d_file == 1) ||
          (d_rank == 1 && d_file == 2))) {
        return 0;
    }

    const Piece *dest = &b->board[dr][df]; // Va a la casilla de destino

    if (is_capture) {       // Valida captura
        if (dest->type == PIECE_NONE) return 0;
        if (dest->color == side_to_move) return 0; // No puede capturar pieza propia
    } else {
        if (dest->type != PIECE_NONE) return 0; // No indica captura pero hay una pieza ahí
    }

    return 1;
}


// Busca la casilla origen (sr,sf) de un movimiento de caballo.
// 0 = valido
// -1 = error (mensaje en error_msg)
static int find_knight_source(const Board *b,
                              const MoveAST *mv,    // movimiento a aplicar
                              Color side_to_move,
                              int *out_sr,      // source rank
                              int *out_sf,      // source file
                              char *error_msg,
                              size_t error_msg_size)
{
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    if (df < 0 || df > 7 || dr < 0 || dr > 7) {
        snprintf(error_msg, error_msg_size,
                 "Destino inválido: %c%c",
                 mv->dest_file ? mv->dest_file : '?',
                 mv->dest_rank ? mv->dest_rank : '?');
        return -1;
    }

    int src_file_filter = -1; // Si src_file_filter->-1 => no filtra por fila 
    int src_rank_filter = -1; // Si src_rank_filter->-1 => no filtra por columna 

    if (mv->src_file) { // consulta si en el AST existe src_file
        src_file_filter = file_to_index(mv->src_file);
    }
    if (mv->src_rank) { // consulta si en el AST existe src_rank
        src_rank_filter = rank_to_index(mv->src_rank);
    }

    int found = 0;
    int best_sr = -1, best_sf = -1;

    /* revisar todas las casillas del tablero */
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            const Piece *p = &b->board[r][f];

            if (p->type != PIECE_KNIGHT) continue;
            if (p->color != side_to_move) continue;

            /* aplicar filtros de desambiguación, si existen */
            if (src_file_filter != -1 && f != src_file_filter) continue; // filtra por columna
            if (src_rank_filter != -1 && r != src_rank_filter) continue; // filtra por fila

            /* ¿puede este caballo ir al destino? */
            if (!can_knight_move(b, r, f, dr, df, mv->is_capture, side_to_move)) {
                continue;
            }

            /* candidato válido encontrado */
            found++;
            best_sr = r;
            best_sf = f;
        }
    }

    if (found == 0) {
        snprintf(error_msg, error_msg_size,
                 "No se encontró ningún caballo que pueda jugar %s",
                 mv->raw);
        return -1;
    } else if (found > 1) {
        snprintf(error_msg, error_msg_size,
                 "El movimiento %s es ambiguo: más de un caballo puede hacerlo",
                 mv->raw);
        return -1;
    }

    *out_sr = best_sr;
    *out_sf = best_sf;
    return 0;
}

// Valida si un peón puede moverse de (sr,sf) a (dr,df)
// 1 = válido
// 0 = inválido
static int can_pawn_move(const Board *b,
                         int sr, int sf,
                         int dr, int df,
                         int is_capture,
                         Color side_to_move,
                         int is_promotion_requested)
{
    int dir;          // dirección de avance en columna 
    int start_rank;   // fila inicial
    int last_rank;    // última fila

    if (side_to_move == COLOR_WHITE) {
        dir = 1;        // avanza filas mayores
        start_rank = 1; 
        last_rank = 7; 
    } else if (side_to_move == COLOR_BLACK) {
        dir = -1;       // avanza filas menores
        start_rank = 6;
        last_rank = 0;
    } else 
        return 0; // color inválido

    int d_rank = dr - sr;
    int d_file = df - sf;

    const Piece *dest = &b->board[dr][df];

    // Caso de captura
    if (is_capture) {
        // Movimiento debe ser una casilla diagonal en la dirección correcta
        if ((d_rank != dir) || (d_file != 1 && d_file != -1)) {
            return 0;
        }

        const Piece *dest = &b->board[dr][df];

        if (dest->type != PIECE_NONE) {
            // Captura normal: debe haber pieza enemiga en destino
            if (dest->color == side_to_move) { // no puedes capturar una pieza propia
                return 0;
            }
        } else {
            // Posible captura al paso: destino vacío
            if (dr == b->en_passant_rank && df == b->en_passant_file) {
                // Determinar la casilla donde está el peón enemigo que se captura al paso
                int pawn_rank = (side_to_move == COLOR_WHITE) ? dr - 1 : dr + 1;
                if (pawn_rank < 0 || pawn_rank > 7) {
                    return 0;
                }
                const Piece *ep_pawn = &b->board[pawn_rank][df];
                if (ep_pawn->type != PIECE_PAWN ||
                    ep_pawn->color == side_to_move) {
                    return 0;
                }
                // Es una captura al paso legal
            } else {
                // No hay pieza en destino y tampoco coincide con casilla de en passant
                return 0;
            }
        }
    // Movimiento normal
    } else {
        if (d_file != 0) { // valida movimiento en la misma columna
            return 0;
        }
        if (d_rank == dir) { // casilla de adelante debe estar vacía
            if (dest->type != PIECE_NONE) {
                return 0;
            }
        }
        else if (d_rank == 2 * dir && sr == start_rank) { // valida doble paso
            int intermediate_rank = sr + dir;
            const Piece *intermediate = &b->board[intermediate_rank][sf];

            if (intermediate->type != PIECE_NONE) { // valida que no haya pieza en el camino
                return 0;
            }
            if (dest->type != PIECE_NONE) { // destino debe estar vacío
                return 0;
            }
        }
        else {
            return 0; // movimiento inválido
        }
    }

    // Validar promoción (obligatoria y en la fila correcta)
    int reaching_last_rank = (dr == last_rank);

    // Si se pide promoción, debe llegar a la última fila
    if (is_promotion_requested && !reaching_last_rank) {
        return 0;
    }

    // Si llega a la última fila, la promoción es obligatoria
    if (!is_promotion_requested && reaching_last_rank) {
        return 0;
    }

    return 1;
}

// Busca la casilla origen (sr,sf) de un movimiento de peón.
// 0 = válido
// -1 = error
static int find_pawn_source(const Board *b,
                            const MoveAST *mv,
                            Color side_to_move,
                            int *out_sr,
                            int *out_sf,
                            char *error_msg,
                            size_t error_msg_size)
{
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    if (df < 0 || df > 7 || dr < 0 || dr > 7) {
        snprintf(error_msg, error_msg_size,
                 "Destino inválido para peón: %c%c",
                 mv->dest_file ? mv->dest_file : '?',
                 mv->dest_rank ? mv->dest_rank : '?');
        return -1;
    }

    int src_file_filter = (mv->src_file) ? file_to_index(mv->src_file) : -1;
    int src_rank_filter = (mv->src_rank) ? rank_to_index(mv->src_rank) : -1;

    int is_promotion_requested = (mv->promotion != 0); // 1 si hay promoción

    int found = 0; // contador de candidatos encontrados
    int best_sr = -1, best_sf = -1; // mejores candidatos

    // Recorremos todos los peones del jugador
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {

            const Piece *p = &b->board[r][f];
            if (p->type != PIECE_PAWN || p->color != side_to_move)
                continue;

            // Filtro por desambiguación 
            if (src_file_filter != -1 && f != src_file_filter)
                continue;
            if (src_rank_filter != -1 && r != src_rank_filter)
                continue;

            // CAPTURA AL PASO
            if (mv->is_capture &&
                b->en_passant_file == df &&
                b->en_passant_rank == dr)
            {
                int dir = (side_to_move == COLOR_WHITE) ? 1 : -1;

                // Un peón que está en columna adyacente y en la fila correcta
                if (r == dr - dir && (f == df + 1 || f == df - 1)) {

                    // Confirmar que detrás del destino hay peón enemigo
                    int pawn_rank = dr - dir;
                    const Piece *ep = &b->board[pawn_rank][df];

                    if (ep->type == PIECE_PAWN && ep->color != side_to_move) {
                        *out_sr = r;
                        *out_sf = f;
                        return 0; // origen encontrado
                    }
                }
            }

            // MOVIMIENTOS NORMALES O CAPTURA NORMAL 
            if (!can_pawn_move(b, r, f, dr, df,
                               mv->is_capture,
                               side_to_move,
                               is_promotion_requested))
                continue;

            found++;
            best_sr = r;
            best_sf = f;
        }
    }

    if (found == 0) {
        snprintf(error_msg, error_msg_size,
                 "No se encontró ningún peón que pueda jugar %s",
                 mv->raw);
        return -1;
    } else if (found > 1) {
        snprintf(error_msg, error_msg_size,
                 "El movimiento %s es ambiguo: más de un peón puede hacerlo",
                 mv->raw);
        return -1;
    }

    *out_sr = best_sr;
    *out_sf = best_sf;
    return 0;
}

// Verifica si el camino entre (sr,sf) y (dr,df) está libre (sin piezas en el medio)
// Solo para movimientos en horizontal, vertical o diagonal
// 1 = libre
// 0 = bloqueado
static int path_is_clear(const Board *b, int sr, int sf, int dr, int df)
{
    // contition ? value_if_true : value_if_false
    int step_r = (dr > sr) ? 1 : (dr < sr) ? -1 : 0; // dirección en fila
    int step_f = (df > sf) ? 1 : (df < sf) ? -1 : 0; // dirección en columna

    // vertical si      step_f == 0
    // horizontal si    step_r == 0
    // diagonal si      ambos != 0

    int r = sr + step_r;
    int f = sf + step_f;

    while (r != dr || f != df) {
        if (b->board[r][f].type != PIECE_NONE)
            return 0;  // hay algo en el camino

        r += step_r;
        f += step_f;
    }
    return 1;
}

// Valida el movimiento de un alfil
// 1 = válido
// 0 = inválido
static int can_bishop_move(const Board *b,
                           int sr, int sf,
                           int dr, int df,
                           int is_capture,
                           Color side)
{
    const Piece *dest = &b->board[dr][df]; // pieza en destino

    int d_rank = dr - sr;
    int d_file = df - sf;

    if (d_rank < 0) d_rank = -d_rank;
    if (d_file < 0) d_file = -d_file;

    // debe ser diagonal exacta
    if (d_rank != d_file) return 0;

    // verificar camino libre
    if (!path_is_clear(b, sr, sf, dr, df)) return 0;

    // manejar captura / no captura
    if (is_capture) {
        if (dest->type == PIECE_NONE) return 0;
        if (dest->color == side) return 0;
    } else {
        if (dest->type != PIECE_NONE) return 0;
    }
    return 1;
}

// Busca la casilla origen (sr,sf) de un movimiento de alfil.
// 0 = válido
// -1 = error
static int find_bishop_source(const Board *b,
                              const MoveAST *mv,
                              Color side,
                              int *out_sr,
                              int *out_sf,
                              char *err,
                              size_t err_sz)
{
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    //valida el filtro del origen
    int src_file_filter = (mv->src_file ? file_to_index(mv->src_file) : -1);
    int src_rank_filter = (mv->src_rank ? rank_to_index(mv->src_rank) : -1);

    int found = 0;

    // revisar todas las casillas del tablero
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            const Piece *p = &b->board[r][f];

            if (p->type != PIECE_BISHOP || p->color != side) continue;

            if (src_file_filter != -1 && f != src_file_filter) continue;
            if (src_rank_filter != -1 && r != src_rank_filter) continue;

            if (!can_bishop_move(b, r, f, dr, df, mv->is_capture, side))
                continue;

            found++;
            *out_sr = r;
            *out_sf = f;
        }
    }

    if (found == 0) {
        snprintf(err, err_sz, "Ningún alfil puede jugar %s", mv->raw);
        return -1;
    }
    if (found > 1) {
        snprintf(err, err_sz, "Movimiento ambiguo: más de un alfil puede jugar %s", mv->raw);
        return -1;
    }
    return 0;
}

// Valida el movimiento de una torre
// 1 = válido
// 0 = inválido
static int can_rook_move(const Board *b,
                         int sr, int sf,
                         int dr, int df,
                         int is_capture,
                         Color side)
{
    const Piece *dest = &b->board[dr][df];

    // debe ser vertical u horizontal
    if (!(sr == dr || sf == df)) return 0;

    // verificar camino libre
    if (!path_is_clear(b, sr, sf, dr, df)) return 0;

    // captura / no captura
    if (is_capture) {
        if (dest->type == PIECE_NONE) return 0;
        if (dest->color == side) return 0;
    } else {
        if (dest->type != PIECE_NONE) return 0;
    }
    return 1;
}

// Busca la casilla origen (sr,sf) de un movimiento de torre.
// 0 = válido
// -1 = error
static int find_rook_source(const Board *b,
                            const MoveAST *mv,
                            Color side,
                            int *out_sr,
                            int *out_sf,
                            char *err,
                            size_t err_sz)
{
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    int src_file_filter = (mv->src_file ? file_to_index(mv->src_file) : -1);
    int src_rank_filter = (mv->src_rank ? rank_to_index(mv->src_rank) : -1);

    int found = 0;

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            const Piece *p = &b->board[r][f];

            if (p->type != PIECE_ROOK || p->color != side) continue;

            if (src_file_filter != -1 && f != src_file_filter) continue;
            if (src_rank_filter != -1 && r != src_rank_filter) continue;

            if (!can_rook_move(b, r, f, dr, df, mv->is_capture, side))
                continue;

            found++;
            *out_sr = r;
            *out_sf = f;
        }
    }

    if (found == 0) {
        snprintf(err, err_sz, "Ninguna torre puede jugar %s", mv->raw);
        return -1;
    }
    if (found > 1) {
        snprintf(err, err_sz, "Movimiento ambiguo: más de una torre puede jugar %s", mv->raw);
        return -1;
    }
    return 0;
}

// Valida el movimiento de una dama
// 1 = válido
// 0 = inválido
static int can_queen_move(const Board *b,
                          int sr, int sf,
                          int dr, int df,
                          int is_capture,
                          Color side)
{
    // movimiento recto (torre) o diagonal (alfil)
    int is_rook_like   = (sr == dr || sf == df);
    int is_bishop_like = ( (dr - sr == df - sf) || (dr - sr == sf - df) );

    if (!is_rook_like && !is_bishop_like) return 0;

    if (!path_is_clear(b, sr, sf, dr, df)) return 0;

    const Piece *dest = &b->board[dr][df];

    if (is_capture) {
        if (dest->type == PIECE_NONE) return 0;
        if (dest->color == side) return 0;
    } else {
        if (dest->type != PIECE_NONE) return 0;
    }
    return 1;
}

// Busca la casilla origen (sr,sf) de un movimiento de dama.
// 0 = válido
// -1 = error
static int find_queen_source(const Board *b,
                             const MoveAST *mv,
                             Color side,
                             int *out_sr,
                             int *out_sf,
                             char *err,
                             size_t err_sz)
{
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    int src_file_filter = (mv->src_file ? file_to_index(mv->src_file) : -1);
    int src_rank_filter = (mv->src_rank ? rank_to_index(mv->src_rank) : -1);

    int found = 0;

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            const Piece *p = &b->board[r][f];

            if (p->type != PIECE_QUEEN || p->color != side) continue;

            if (src_file_filter != -1 && f != src_file_filter) continue;
            if (src_rank_filter != -1 && r != src_rank_filter) continue;

            if (!can_queen_move(b, r, f, dr, df, mv->is_capture, side))
                continue;

            found++;
            *out_sr = r;
            *out_sf = f;
        }
    }

    if (found == 0) {
        snprintf(err, err_sz, "Ninguna dama puede jugar %s", mv->raw);
        return -1;
    }
    if (found > 1) {
        snprintf(err, err_sz, "Movimiento ambiguo: más de una dama puede jugar %s", mv->raw);
        return -1;
    }
    return 0;
}

// Valida el movimiento de un rey
// 1 = válido
// 0 = inválido
static int can_king_move(const Board *b,
                         int sr, int sf,
                         int dr, int df,
                         int is_capture,
                         Color side)
{
    int d_rank = dr - sr;
    int d_file = df - sf;
    if (d_rank < 0) d_rank = -d_rank;
    if (d_file < 0) d_file = -d_file;

    // Rey debe moverse 1 casilla como máximo en cada eje (sin enroque)
    if (d_rank > 1 || d_file > 1 || (d_rank == 0 && d_file == 0)) {
        return 0;
    }

    const Piece *dest = &b->board[dr][df];

    if (is_capture) {
        if (dest->type == PIECE_NONE) return 0;
        if (dest->color == side) return 0;
    } else {
        if (dest->type != PIECE_NONE) return 0;
    }

    return 1;
}

// Valida si la casilla (r,f) está atacada por el bando 'by_side'
// 1 = está atacada
// 0 = no está atacada
static int is_square_attacked(const Board *b, int r, int f, Color by_side)
{
    if (!b) return 0;

    Color enemy = by_side;

    // 1) Ataques de peones
    if (enemy == COLOR_WHITE) {
        // Peón blanco ataca (r+1, f-1) y (r+1, f+1)
        int pr = r - 1;
        if (pr >= 0) {
            if (f - 1 >= 0) {
                const Piece *p = &b->board[pr][f-1];
                if (p->color == COLOR_WHITE && p->type == PIECE_PAWN) return 1;
            }
            if (f + 1 < 8) {
                const Piece *p = &b->board[pr][f+1];
                if (p->color == COLOR_WHITE && p->type == PIECE_PAWN) return 1;
            }
        }
    } else if (enemy == COLOR_BLACK) {
        // Peón negro ataca (r-1, f-1) y (r-1, f+1)
        int pr = r + 1;
        if (pr < 8) {
            if (f - 1 >= 0) {
                const Piece *p = &b->board[pr][f-1];
                if (p->color == COLOR_BLACK && p->type == PIECE_PAWN) return 1;
            }
            if (f + 1 < 8) {
                const Piece *p = &b->board[pr][f+1];
                if (p->color == COLOR_BLACK && p->type == PIECE_PAWN) return 1;
            }
        }
    }

    // 2) Ataques de caballos 
    const int knight_moves[8][2] = {
        { 2, 1}, { 2,-1}, {-2, 1}, {-2,-1},
        { 1, 2}, { 1,-2}, {-1, 2}, {-1,-2}
    };
    // Recorre los posibles movimientos del caballo
    for (int k = 0; k < 8; ++k) {
        int rr = r + knight_moves[k][0];
        int ff = f + knight_moves[k][1];
        if (rr < 0 || rr >= 8 || ff < 0 || ff >= 8) continue;
        const Piece *p = &b->board[rr][ff];
        if (p->color == enemy && p->type == PIECE_KNIGHT) return 1;
    }

    // 3) Ataques en líneas rectas (torres y damas)
    const int dirs_straight[4][2] = {
        { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1}
    };
    // Recorre las 4 direcciones rectas
    for (int d = 0; d < 4; ++d) {
        int dr = dirs_straight[d][0];
        int df = dirs_straight[d][1];
        int rr = r + dr;
        int ff = f + df;
        // Avanza en esa dirección hasta que se salga del tablero o encuentre una pieza
        while (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) {
            const Piece *p = &b->board[rr][ff];
            if (p->type != PIECE_NONE) {
                if (p->color == enemy &&
                    (p->type == PIECE_ROOK || p->type == PIECE_QUEEN)) {
                    return 1;
                }
                break; // pieza bloquea el ataque
            }
            rr += dr;
            ff += df;
        }
    }

    // 4) Ataques en diagonales (alfiles y damas)
    const int dirs_diag[4][2] = {
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };
    // Recorre las 4 direcciones diagonales
    for (int d = 0; d < 4; ++d) {
        int dr = dirs_diag[d][0];
        int df = dirs_diag[d][1];
        int rr = r + dr;
        int ff = f + df;
        // Avanza en esa dirección hasta que se salga del tablero o encuentre una pieza
        while (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) {
            const Piece *p = &b->board[rr][ff];
            if (p->type != PIECE_NONE) {
                if (p->color == enemy &&
                    (p->type == PIECE_BISHOP || p->type == PIECE_QUEEN)) {
                    return 1;
                }
                break;
            }
            rr += dr;
            ff += df;
        }
    }

    // 5) Ataques del rey enemigo 
    for (int dr = -1; dr <= 1; ++dr) {
        for (int df = -1; df <= 1; ++df) {
            if (dr == 0 && df == 0) continue;
            int rr = r + dr;
            int ff = f + df;
            if (rr < 0 || rr >= 8 || ff < 0 || ff >= 8) continue;
            const Piece *p = &b->board[rr][ff];
            if (p->color == enemy && p->type == PIECE_KING) {
                return 1;
            }
        }
    }

    return 0;
}

// Verifica si el rey del bando del color 'side' está en jaque
// 1 = en jaque
// 0 = no en jaque
static int is_king_in_check(const Board *b, Color side)
{
    if (!b || side == COLOR_NONE) return 0;

    int king_r = -1, king_f = -1;

    // Buscar al rey de 'side' en el tablero
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            const Piece *p = &b->board[r][f];
            if (p->color == side && p->type == PIECE_KING) {
                king_r = r;
                king_f = f;
                break;
            }
        }
        if (king_r != -1) break;
    }

    if (king_r == -1) {
        // Posición ilegal (no hay rey), pero por ahora devolvemos "no en jaque"
        return 0;
    }

    Color enemy = (side == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    return is_square_attacked(b, king_r, king_f, enemy);
}

static int has_any_legal_move(const Board *b, Color side)
{
    if (!b || side == COLOR_NONE) return 0;

    Color enemy = (side == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;

    for (int sr = 0; sr < 8; ++sr) {
        for (int sf = 0; sf < 8; ++sf) {

            const Piece *p = &b->board[sr][sf];
            if (p->color != side || p->type == PIECE_NONE)
                continue;

            PieceType pt = p->type;

            // Recorremos todas las casillas destino posibles
            for (int dr = 0; dr < 8; ++dr) {
                for (int df = 0; df < 8; ++df) {

                    // No validamos quedarse en la misma casilla
                    if (dr == sr && df == sf) continue;

                    const Piece *dest = &b->board[dr][df];

                    // Omitir movimientos que capturan pieza propia
                    if (dest->type != PIECE_NONE && dest->color == side)
                        continue;

                    int is_capture = (dest->type != PIECE_NONE);

                    // Lógica especial para peones: en passant
                    int is_en_passant_capture = 0;

                    if (pt == PIECE_PAWN && dest->type == PIECE_NONE) {
                        if (b->en_passant_file == df && b->en_passant_rank == dr) {
                            int pawn_rank = (side == COLOR_WHITE) ? dr - 1 : dr + 1;
                            if (pawn_rank >= 0 && pawn_rank < 8) {
                                const Piece *ep = &b->board[pawn_rank][df];
                                if (ep->type == PIECE_PAWN && ep->color == enemy) {
                                    is_capture = 1;
                                    is_en_passant_capture = 1;
                                }
                            }
                        }
                    }

                    int is_promotion_requested = 0;
                    if (pt == PIECE_PAWN) {
                        int last_rank = (side == COLOR_WHITE) ? 7 : 0;
                        if (dr == last_rank) {
                            is_promotion_requested = 1;
                        }
                    }

                    // Comprobar movimiento geométricamente válido
                    int ok = 0;
                    switch (pt) {
                        case PIECE_PAWN:
                            ok = can_pawn_move(b, sr, sf, dr, df,
                                               is_capture,
                                               side,
                                               is_promotion_requested);
                            break;
                        case PIECE_KNIGHT:
                            ok = can_knight_move(b, sr, sf, dr, df, is_capture, side);
                            break;
                        case PIECE_BISHOP:
                            ok = can_bishop_move(b, sr, sf, dr, df, is_capture, side);
                            break;
                        case PIECE_ROOK:
                            ok = can_rook_move(b, sr, sf, dr, df, is_capture, side);
                            break;
                        case PIECE_QUEEN:
                            ok = can_queen_move(b, sr, sf, dr, df, is_capture, side);
                            break;
                        case PIECE_KING:
                            ok = can_king_move(b, sr, sf, dr, df, is_capture, side);
                            break;
                        default:
                            ok = 0;
                            break;
                    }

                    if (!ok) continue;

                    // Simular el movimiento en un tablero temporal
                    Board tmp = *b;
                    Piece moving = tmp.board[sr][sf];

                    tmp.board[sr][sf].type  = PIECE_NONE;
                    tmp.board[sr][sf].color = COLOR_NONE;

                    // Captura al paso: eliminar el peón enemigo en la casilla correcta
                    if (pt == PIECE_PAWN && is_en_passant_capture) {
                        int pawn_rank = (side == COLOR_WHITE) ? dr - 1 : dr + 1;
                        tmp.board[pawn_rank][df].type  = PIECE_NONE;
                        tmp.board[pawn_rank][df].color = COLOR_NONE;
                    }

                    // Promoción: para efectos de jaque/ahogado, basta con promover a dama
                    if (pt == PIECE_PAWN && is_promotion_requested) {
                        moving.type = PIECE_QUEEN;
                    }

                    tmp.board[dr][df] = moving;

                    // Si después de este movimiento el rey de 'side' NO está en jaque,
                    // entonces existe al menos una jugada legal.
                    if (!is_king_in_check(&tmp, side)) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

// Actualiza los derechos de enroque cuando una pieza de 'side' se mueve
static void update_castling_rights_on_move(Board *b,
                                           Color side,
                                           PieceType pt,
                                           int sr, int sf)
{
    if (!b) return;

    if (side == COLOR_WHITE) {
        if (pt == PIECE_KING) { // Si el rey se mueve pierde ambos enroques
            b->white_can_castle_short = 0;
            b->white_can_castle_long  = 0;
        } else if (pt == PIECE_ROOK && sr == 0) { // Se mueve torre blanca
            if (sf == 0) {          // Torre de a1
                b->white_can_castle_long = 0;
            } else if (sf == 7) {   // Torre de h1
                b->white_can_castle_short = 0;
            }
        }
    } else if (side == COLOR_BLACK) {
        if (pt == PIECE_KING) { // Si el rey se mueve pierde ambos enroques
            b->black_can_castle_short = 0;
            b->black_can_castle_long  = 0;
        } else if (pt == PIECE_ROOK && sr == 7) { // Se mueve torre negra
            if (sf == 0) {          // Torre de a8
                b->black_can_castle_long = 0;
            } else if (sf == 7) {   // Torre de h8
                b->black_can_castle_short = 0;
            }
        }
    }
}

// Actualiza los derechos de enroque cuando se captura una torre
static void update_castling_rights_on_capture(Board *b,
                                              const Piece *captured,
                                              int dr, int df)
{
    if (!b || !captured) return;
    if (captured->type != PIECE_ROOK) return;

    if (captured->color == COLOR_WHITE && dr == 0) { // Torre blanca capturada en fila 1
        if (df == 0) {  // a1
            b->white_can_castle_long = 0;
        } else if (df == 7) {  // h1
            b->white_can_castle_short = 0;
        }
    } else if (captured->color == COLOR_BLACK && dr == 7) { // Torre negra capturada en fila 8
        if (df == 0) {  // a8
            b->black_can_castle_long = 0;
        } else if (df == 7) {  // h8
            b->black_can_castle_short = 0;
        }
    }
}


// Aplica el movimiento de enroque al tablero 'b'.
// 0 = éxito
// -1 = error
static int apply_castling(Board *b,
                          const MoveAST *mv,
                          Color side,
                          char *err,
                          size_t err_sz)
{
    if (!b || !mv) {
        snprintf(err, err_sz, "Argumentos nulos en apply_castling");
        return -1;
    }

    Color enemy = (side == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;

    int king_rank, king_file_start;
    int rook_file_start, king_file_end, rook_file_end;

    // Determinar filas y columnas según el color
    if (side == COLOR_WHITE) {
        // Posición inicial del rey
        king_rank = 0;
        king_file_start = 4;
        // Determinar si es enroque corto o largo
        if (mv->is_castle_short) {
            if (!b->white_can_castle_short) {
                snprintf(err, err_sz, "Blanco no tiene derecho a enroque corto.");
                return -1;
            }
            rook_file_start = 7; // h
            king_file_end = 6;   // g
            rook_file_end = 5;   // f
        } else {
            if (!b->white_can_castle_long) {
                snprintf(err, err_sz, "Blanco no tiene derecho a enroque largo.");
                return -1;
            }
            rook_file_start = 0; // a
            king_file_end = 2;   // c
            rook_file_end = 3;   // d
        }
    // Color negro
    } else {
        king_rank = 7;
        king_file_start = 4;
        if (mv->is_castle_short) {
            if (!b->black_can_castle_short) {
                snprintf(err, err_sz, "Negro no tiene derecho a enroque corto.");
                return -1;
            }
            rook_file_start = 7; // h
            king_file_end = 6;   // g
            rook_file_end = 5;   // f
        } else {
            if (!b->black_can_castle_long) {
                snprintf(err, err_sz, "Negro no tiene derecho a enroque largo.");
                return -1;
            }
            rook_file_start = 0; // a
            king_file_end = 2;   // c
            rook_file_end = 3;   // d
        }
    }

    // Verificar que haya rey y torre correctos
    Piece *king = &b->board[king_rank][king_file_start];
    Piece *rook = &b->board[king_rank][rook_file_start];

    if (king->type != PIECE_KING || king->color != side) {
        snprintf(err, err_sz, "No hay rey correcto en su casilla inicial para enrocar.");
        return -1;
    }
    if (rook->type != PIECE_ROOK || rook->color != side) {
        snprintf(err, err_sz, "No hay torre correcta en su casilla inicial para enrocar.");
        return -1;
    }

    // 1) El rey no debe estar en jaque antes del enroque
    if (is_king_in_check(b, side)) {
        snprintf(err, err_sz, "No puedes enrocar estando en jaque.");
        return -1;
    }

    // 2) Las casillas entre rey y torre deben estar vacías
    int step = (rook_file_start > king_file_start) ? 1 : -1;
    for (int f = king_file_start + step; f != rook_file_start; f += step) {
        if (b->board[king_rank][f].type != PIECE_NONE) {
            snprintf(err, err_sz, "No se puede enrocar: hay piezas entre rey y torre.");
            return -1;
        }
    }

    // 3) Valida que las casillas por las que pasa el rey no estén atacadas
    int king_path_files[3];
    int path_len = 0;
    if (mv->is_castle_short) {
        king_path_files[0] = king_file_start;
        king_path_files[1] = king_file_start + step; // f
        king_path_files[2] = king_file_end;          // g
        path_len = 3;
    } else {
        king_path_files[0] = king_file_start;
        king_path_files[1] = king_file_start + step; // d
        king_path_files[2] = king_file_end;          // c
        path_len = 3;
    }

    for (int i = 0; i < path_len; ++i) {
        int f = king_path_files[i];
        if (is_square_attacked(b, king_rank, f, enemy)) {
            snprintf(err, err_sz, "No se puede enrocar: el rey pasaría por casilla atacada.");
            return -1;
        }
    }

    // 4) Aplicar enroque: mover rey y torre
    Piece king_copy = *king;
    Piece rook_copy = *rook;

    king->type = PIECE_NONE; king->color = COLOR_NONE;
    rook->type = PIECE_NONE; rook->color = COLOR_NONE;

    b->board[king_rank][king_file_end] = king_copy;
    b->board[king_rank][rook_file_end] = rook_copy;

    // 5) Actualizar derechos de enroque
    if (side == COLOR_WHITE) {
        b->white_can_castle_short = 0;
        b->white_can_castle_long = 0;
    } else {
        b->black_can_castle_short = 0;
        b->black_can_castle_long = 0;
    }

    return 0;
}

// Evalúa el estado de la posición para el bando 'side_to_move'
PositionStatus board_evaluate_status(const Board *b, Color side_to_move)
{
    if (!b || side_to_move == COLOR_NONE) {
        return POSITION_NORMAL;
    }

    int in_check  = is_king_in_check(b, side_to_move);
    int has_moves = has_any_legal_move(b, side_to_move);

    if (in_check && has_moves)  return POSITION_CHECK;
    if (in_check && !has_moves) return POSITION_CHECKMATE;
    if (!in_check && !has_moves) return POSITION_STALEMATE;
    return POSITION_NORMAL;
}



int board_apply_move(Board *b,
                     const MoveAST *mv,
                     Color side_to_move,
                     char *error_msg,
                     size_t error_msg_size)
{
    if (!b || !mv) {
        snprintf(error_msg, error_msg_size, "Argumentos nulos en board_apply_move");
        return -1;
    }

    // 1) Caso especial: enroque
    if (mv->is_castle_short || mv->is_castle_long) {
        Board tmp = *b;

        // Intentar aplicar el enroque en el tablero temporal
        if (apply_castling(&tmp, mv, side_to_move, error_msg, error_msg_size) != 0) {
            return -1;
        }

        // Validar que el rey no quede en jaque después del enroque
        if (is_king_in_check(&tmp, side_to_move)) {
            snprintf(error_msg, error_msg_size,
                    "Enroque ilegal: el rey quedaría en jaque.");
            return -1;
        }

        // Si es legal, aplicar en el tablero real
        *b = tmp;
        return 0;
    }

    // 2) Determinar tipo de pieza a partir de mv->piece 

    char piece_char = mv->piece ? mv->piece : 'P'; // Por defecto peón
    PieceType pt = piece_type_from_char(piece_char);

    // Validar que haya un tipo de pieza válido
    if (pt == PIECE_NONE) {
        snprintf(error_msg, error_msg_size,
                 "Tipo de pieza inválido en el movimiento: %c",
                 piece_char);
        return -1;
    }

    // Variables para casilla origen y destino
    int sr = -1, sf = -1;
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    // 3) Validar destino dentro del tablero
    if (df < 0 || df > 7 || dr < 0 || dr > 7) {
        snprintf(error_msg, error_msg_size,
                 "Destino inválido: %c%c",
                 mv->dest_file ? mv->dest_file : '?',
                 mv->dest_rank ? mv->dest_rank : '?');
        return -1;
    }

    // 4) Reglas adicionales de peones: promoción obligatoria/correcta
    if (pt == PIECE_PAWN) {
        int last_rank = (side_to_move == COLOR_WHITE) ? 7 : 0;
        int reaching_last = (dr == last_rank);
        int has_promo = (mv->promotion != 0);

        if (reaching_last && !has_promo) {
            snprintf(error_msg, error_msg_size,
                     "Movimiento ilegal: el peón que llega a la última fila debe promocionar (%s).",
                     mv->raw);
            return -1;
        }

        if (!reaching_last && has_promo) {
            snprintf(error_msg, error_msg_size,
                     "Movimiento ilegal: solo se puede promocionar al llegar a la última fila (%s).",
                     mv->raw);
            return -1;
        }
    }

    // 5) Encontrar la casilla origen (sr,sf)
    if (pt == PIECE_KNIGHT) {
        if (find_knight_source(b, mv, side_to_move, &sr, &sf,
                            error_msg, error_msg_size) != 0)
            return -1;
    }
    else if (pt == PIECE_PAWN) {
        if (find_pawn_source(b, mv, side_to_move, &sr, &sf,
                            error_msg, error_msg_size) != 0)
            return -1;
    }
    else if (pt == PIECE_BISHOP) {
        if (find_bishop_source(b, mv, side_to_move, &sr, &sf,
                            error_msg, error_msg_size) != 0)
            return -1;
    }
    else if (pt == PIECE_ROOK) {
        if (find_rook_source(b, mv, side_to_move, &sr, &sf,
                            error_msg, error_msg_size) != 0)
            return -1;
    }
    else if (pt == PIECE_QUEEN) {
        if (find_queen_source(b, mv, side_to_move, &sr, &sf,
                            error_msg, error_msg_size) != 0)
            return -1;
    }
    else if (pt == PIECE_KING) {
        int found = 0;

        for (int r = 0; r < 8; ++r) {
            for (int f = 0; f < 8; ++f) {
                Piece *p = &b->board[r][f];

                if (p->type == PIECE_KING && p->color == side_to_move) {

                    // ¿Puede este rey ir a (dr, df)?
                    if (!can_king_move(b, r, f, dr, df, mv->is_capture, side_to_move)) {
                        snprintf(error_msg, error_msg_size,
                                "Movimiento ilegal del rey: no puede ir a %c%c",
                                mv->dest_file, mv->dest_rank);
                        return -1;
                    }

                    // Guardar origen
                    sr = r;
                    sf = f;
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }

        if (!found) {
            snprintf(error_msg, error_msg_size,
                    "No se encontró el rey del bando que mueve.");
            return -1;
        }
    }
    else {
        snprintf(error_msg, error_msg_size,
                 "Tipo de pieza no soportada.");
        return -1;
    }

    // 6) Simular el movimiento en un tablero temporal 

    Board tmp = *b;

    // Piezas involucradas antes de mover
    Piece moving_before = tmp.board[sr][sf];
    Piece captured_before = tmp.board[dr][df];  // puede ser NONE

    // Detectar si esta jugada es una captura al paso
    int is_en_passant_capture = 0;
    if (pt == PIECE_PAWN &&
        mv->is_capture &&
        captured_before.type == PIECE_NONE &&  /* destino está vacío */
        b->en_passant_file == df &&
        b->en_passant_rank == dr) {
        is_en_passant_capture = 1;
    }

    // Prohibir capturar al rey enemigo
    if (captured_before.type == PIECE_KING &&
        captured_before.color != side_to_move) {
        snprintf(error_msg, error_msg_size,
                 "Movimiento ilegal: no se puede capturar al rey.");
        return -1;
    }

    // Actualizar derechos de enroque en el tablero temporal:
    update_castling_rights_on_move(&tmp, side_to_move, pt, sr, sf);
    update_castling_rights_on_capture(&tmp, &captured_before, dr, df);

    // Aplicar el movimiento en tmp
    Piece moving = moving_before;

    // Vaciar la casilla origen
    tmp.board[sr][sf].type = PIECE_NONE;
    tmp.board[sr][sf].color = COLOR_NONE;

    // Si es captura al paso, eliminar el peón enemigo en la casilla correcta
    if (is_en_passant_capture) {
        int pawn_rank = (side_to_move == COLOR_WHITE) ? dr - 1 : dr + 1;
        tmp.board[pawn_rank][df].type  = PIECE_NONE;
        tmp.board[pawn_rank][df].color = COLOR_NONE;
    }

    // Manejar promoción de peón
    if (pt == PIECE_PAWN && mv->promotion) {
        PieceType promo_type = piece_type_from_char(mv->promotion);

        if (promo_type != PIECE_QUEEN &&
            promo_type != PIECE_ROOK  &&
            promo_type != PIECE_BISHOP &&
            promo_type != PIECE_KNIGHT) {
            snprintf(error_msg, error_msg_size,
                     "Pieza de promoción inválida: %c. Debe ser Q, R, B o N.",
                     mv->promotion);
            return -1;
        }
        moving.type = promo_type;
    }

    // Colocar la pieza en la casilla destino
    tmp.board[dr][df] = moving;

    tmp.en_passant_file = -1;
    tmp.en_passant_rank = -1;

    if (pt == PIECE_PAWN && !mv->is_capture) {
        int dir = (side_to_move == COLOR_WHITE) ? 1 : -1;
        // Si el peón avanzó dos casillas, hay posibilidad de en passant
        if (dr - sr == 2 * dir) {
            tmp.en_passant_file = df;
            tmp.en_passant_rank = sr + dir; // casilla intermedia que "saltó"
        }
    }

    // 7) Validar si el rey propio queda en jaque en la posición resultante
    if (is_king_in_check(&tmp, side_to_move)) {
        snprintf(error_msg, error_msg_size,
                 "Movimiento ilegal: el rey quedaría en jaque tras %s", mv->raw);
        return -1;
    }

    // 8) Validar coherencia de jaque y jaque mate
    Color enemy = (side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    int enemy_in_check   = is_king_in_check(&tmp, enemy);
    int enemy_has_moves  = has_any_legal_move(&tmp, enemy);

    int expect_check = (mv->is_check || mv->is_mate);

    // Caso 1: se marcó + o # pero el rey enemigo NO está en jaque
    if (expect_check && !enemy_in_check) {
        snprintf(error_msg, error_msg_size,
                 "Movimiento %s está anotado como jaque/jaque mate, "
                 "pero el rey enemigo no está en jaque.", mv->raw);
        return -1;
    }

    // Caso 2: NO se marcó + ni # pero el rey enemigo SÍ está en jaque
    if (!expect_check && enemy_in_check) {
        snprintf(error_msg, error_msg_size,
                 "Movimiento %s da jaque, pero no está marcado con '+' o '#'.",
                 mv->raw);
        return -1;
    }

    // Caso 3: se marcó # pero NO es jaque mate (tiene jugadas legales)
    if (mv->is_mate && enemy_in_check && enemy_has_moves) {
        snprintf(error_msg, error_msg_size,
                 "Movimiento %s está anotado como jaque mate ('#'), "
                 "pero el rival aún tiene movimientos legales.", mv->raw);
        return -1;
    }

    // Caso 4: NO se marcó # pero en realidad es jaque mate
    if (!mv->is_mate && enemy_in_check && !enemy_has_moves) {
        snprintf(error_msg, error_msg_size,
                 "Movimiento %s produce jaque mate, pero no está marcado con '#'.",
                 mv->raw);
        return -1;
    }
    
    // 8) Si es legal, copiar tablero temporal al real
    *b = tmp;

    return 0;
}

// Convierte una pieza en su representación de carácter para impresión
static char piece_to_char(const Piece *p) {
    if (!p || p->type == PIECE_NONE) return '.';

    char c = '?';
    switch (p->type) {
        case PIECE_PAWN:   c = 'P'; break;
        case PIECE_KNIGHT: c = 'N'; break;
        case PIECE_BISHOP: c = 'B'; break;
        case PIECE_ROOK:   c = 'R'; break;
        case PIECE_QUEEN:  c = 'Q'; break;
        case PIECE_KING:   c = 'K'; break;
        default:           c = '?'; break;
    }

    /* Piezas negras en minúscula, blancas en mayúscula */
    if (p->color == COLOR_BLACK) {
        c = (char)(c + ('a' - 'A')); /* convertir a minúscula */
    }

    return c;
}

// Imprime el tablero en la consola
void board_print(const Board *b) {
    if (!b) return;

    printf("\n  Tablero actual:\n\n");
    for (int r = 7; r >= 0; --r) {
        printf("%d  ", r + 1);   /* número de fila (1..8) */
        for (int f = 0; f < 8; ++f) {
            char c = piece_to_char(&b->board[r][f]);
            printf("%c ", c);
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h\n\n");
}
