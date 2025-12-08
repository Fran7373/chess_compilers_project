// src/lexer_test.c
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "board.h"

// -----------------------------------------------------------
// Conversión desde coordenadas (src, dst) a SAN básico.
// Ejemplos:
//    e2 → e4   => "e4"
//    e4 → d5   => "exd5"
//    g1 → f3   => "Nf3"
//    f3 → e5   => "Nxe5"
// -----------------------------------------------------------
char *square_to_san(char *buf, const char *src, const char *dst, const Board *b) {

    int sf = src[0] - 'a';
    int sr = src[1] - '1';
    int df = dst[0] - 'a';
    int dr = dst[1] - '1';

    const Piece *p    = &b->board[sr][sf];
    const Piece *dest = &b->board[dr][df];

    if (p->type == PIECE_NONE) {
        strcpy(buf, ""); // Origen vacío → no se puede generar SAN.
        return buf;
    }

    // Traducir tipo de pieza a letra SAN
    char pieceChar;
    switch (p->type) {
        case PIECE_PAWN:   pieceChar = 0;   break; // Los peones no llevan letra
        case PIECE_KNIGHT: pieceChar = 'N'; break;
        case PIECE_BISHOP: pieceChar = 'B'; break;
        case PIECE_ROOK:   pieceChar = 'R'; break;
        case PIECE_QUEEN:  pieceChar = 'Q'; break;
        case PIECE_KING:   pieceChar = 'K'; break;
        default:
            strcpy(buf, "");
            return buf;
    }

    int is_capture = (dest->type != PIECE_NONE);

    // ------------------------------
    // CASO 1: PEÓN
    // ------------------------------
    if (p->type == PIECE_PAWN) {
        if (is_capture) {
            // exd5 → columna origen + 'x' + destino
            snprintf(buf, 16, "%c%c%c%c", src[0], 'x', dst[0], dst[1]);
        } else {
            // e4 → destino
            snprintf(buf, 16, "%c%c", dst[0], dst[1]);
        }
        return buf;
    }

    // ------------------------------
    // CASO 2: PIEZAS (N, B, R, Q, K)
    // ------------------------------
    if (is_capture) {
        // Nxe5, Bxd3, etc.
        snprintf(buf, 16, "%c%c%c%c", pieceChar, 'x', dst[0], dst[1]);
    } else {
        // Nf3, Bd6, etc.
        snprintf(buf, 16, "%c%c%c", pieceChar, dst[0], dst[1]);
    }

    return buf;
}



static void print_moveast(const MoveAST *m) {
    if (!m) return;
    printf("Parsed MoveAST:\n");
    printf("  raw: \"%s\"\n", m->raw[0] ? m->raw : "(empty)");
    printf("  piece: %c\n", m->piece ? m->piece : '-');
    printf("  src_file: %c\n", m->src_file ? m->src_file : '-');
    printf("  src_rank: %c\n", m->src_rank ? m->src_rank : '-');
    printf("  dest_file: %c\n", m->dest_file ? m->dest_file : '-');
    printf("  dest_rank: %c\n", m->dest_rank ? m->dest_rank : '-');
    printf("  is_capture: %d\n", m->is_capture);
    printf("  promotion: %c\n", m->promotion ? m->promotion : '-');
    printf("  is_castle_short: %d\n", m->is_castle_short);
    printf("  is_castle_long: %d\n", m->is_castle_long);
    printf("  is_check: %d\n", m->is_check);
    printf("  is_mate: %d\n", m->is_mate);
}

int main(void) {
    
    Board board;
    board_init_start(&board);
    board_print(&board);

    Color side_to_move = COLOR_WHITE;

    while (1)
    {
        char src[3], dst[3], san[16];

        printf("Tomar ficha (origen, ej: e2): ");
        if (scanf("%2s", src) != 1) {
            printf("Entrada inválida.\n");
            return 0;
        }

        printf("Mover ficha (destino, ej: e4): ");
        if (scanf("%2s", dst) != 1) {
            printf("Entrada inválida.\n");
            return 0;
        }

        // Convertir origen + destino a SAN
        square_to_san(san, src, dst, &board);
        printf("SAN generado: %s\n", san);

        // --------------------------
        // Lexer
        // --------------------------
        TokenList tl;
        if (tokenize(san, &tl) != 0) {
            fprintf(stderr, "Error léxico al tokenizar la entrada.\n");
            continue;
        }

        printf("\nTokens detectados:\n");
        for (size_t i = 0; i < tl.count; ++i) {
            Token *t = &tl.items[i];
            printf("   %2zu: %-18s '%s'\n", i, token_name(t->type), t->text);
            if (t->type == TK_END) break;
        }

        // --------------------------
        // Parser
        // --------------------------
        MoveAST m;
        if (parse_move(&tl, &m) != 0) {
            printf("\nError sintáctico al parsear '%s'\n\n", san);
            tokenlist_free(&tl);
            continue;
        }

        printf("\n");
        print_moveast(&m);

        // --------------------------
        // Semántica
        // --------------------------
        char error_msg[256] = {0};

        int serr = board_apply_move(&board, &m, side_to_move,
                                    error_msg, sizeof(error_msg));

        if (serr == 0) {
            printf("\nMovimiento semánticamente válido. Tablero actualizado:\n");
            board_print(&board);

            // Cambiar turno
            side_to_move = (side_to_move == COLOR_WHITE)
                               ? COLOR_BLACK
                               : COLOR_WHITE;
        } else {
            printf("\nError semántico: %s\n", error_msg);
            printf("\nEl tablero permanece igual:\n");
            board_print(&board);
        }

        tokenlist_free(&tl);
    }

    return 0;
}
