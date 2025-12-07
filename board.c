#include <stdio.h>
#include <string.h>
#include "board.h"


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

/* Inicializa el tablero en la posición inicial estándar */
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
        if ((d_rank != dir) || (d_file != 1 && d_file != -1)) { // valida movimiento diagonal
            return 0;
        }
        if (dest->type == PIECE_NONE) { // Debe haber una pieza ahí
            return 0;
        }
        if (dest->color == side_to_move) { // no puedes capturar propia
            return 0;
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

    // Validar promoción
    if (is_promotion_requested) {
        if (dr != last_rank) { // solo se puede promover en la última fila
            return 0; 
        }
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

    int src_file_filter = -1;
    int src_rank_filter = -1;

    if (mv->src_file) {
        src_file_filter = file_to_index(mv->src_file);
    }
    if (mv->src_rank) {
        src_rank_filter = rank_to_index(mv->src_rank);
    }

    int is_promotion_requested = (mv->promotion != 0); // 1 si hay promoción

    int found = 0; // contador de candidatos encontrados
    int best_sr = -1, best_sf = -1; // mejores candidatos

    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            const Piece *p = &b->board[r][f];

            if (p->type != PIECE_PAWN) continue;
            if (p->color != side_to_move) continue;

            if (src_file_filter != -1 && f != src_file_filter) continue;
            if (src_rank_filter != -1 && r != src_rank_filter) continue;

            // valida si este peón puede moverse al destino
            if (!can_pawn_move(b, r, f, dr, df,
                               mv->is_capture,
                               side_to_move,
                               is_promotion_requested)) {
                continue;
            }

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
    // Enroques aún no soportados en esta versión
    if (mv->is_castle_short || mv->is_castle_long) {
        snprintf(error_msg, error_msg_size,
                 "Enroque aún no soportado en esta versión.");
        return -1;
    }

    // Determinar tipo de pieza a partir de mv->piece
    // Si mv->piece == 0, asumimos 'P'
    char piece_char = mv->piece ? mv->piece : 'P';
    PieceType pt = piece_type_from_char(piece_char);

    // 1) Validar que la pieza sea válida
    int sr = -1, sf = -1;
    int df = file_to_index(mv->dest_file);
    int dr = rank_to_index(mv->dest_rank);

    // 2) Validar destino
    if (df < 0 || df > 7 || dr < 0 || dr > 7) {
        snprintf(error_msg, error_msg_size,
                 "Destino inválido: %c%c",
                 mv->dest_file ? mv->dest_file : '?',
                 mv->dest_rank ? mv->dest_rank : '?');
        return -1;
    }

    // 3) Encontrar la casilla origen (sr,sf)
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
    else {
        snprintf(error_msg, error_msg_size,
                "Por ahora todavía no soportamos movimientos de rey.");
        return -1;
    }


    // 4) Aplicar el movimiento
    Piece moving = b->board[sr][sf];
    b->board[sr][sf].type = PIECE_NONE;
    b->board[sr][sf].color = COLOR_NONE;

    // 5) Manejo de promoción de peón
    if (pt == PIECE_PAWN && mv->promotion) {
        // convertir mv->promotion ('Q','R','B','N') a PieceType
        PieceType promo_type = piece_type_from_char(mv->promotion);
        if (promo_type == PIECE_NONE || promo_type == PIECE_PAWN) {
            // si algo raro, lo dejamos como dama por defecto o marcamos error;
               // por ahora lo dejamos como dama si mv->promotion == 'Q' */
            promo_type = PIECE_QUEEN;
        }
        moving.type = promo_type;
    }

    // 6) Colocar la pieza en destino
    b->board[dr][df] = moving;

    /* Más adelante aquí verificaremos si el rey queda en jaque
       y, si es así, revertiremos el movimiento. */

    return 0;
}




// Helper interno: devuelve un carácter para imprimir la pieza
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

/* Imprime el tablero desde la perspectiva normal:
   8 ... 1 en vertical, a..h en horizontal */
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
