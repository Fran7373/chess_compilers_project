// interactivo.c - Modo interactivo de juego
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "semant.h"
#include "interactivo.h"

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

int interactive_mode(void) {
    Board board;
    board_init_start(&board);
    board_print(&board);

    Color side_to_move = COLOR_WHITE;

    while (1) {
        char input[256];

        printf("Ingrese un movimiento SAN: ");
        if (!fgets(input, sizeof(input), stdin)) {
            fprintf(stderr, "No input (EOF).\n");
            return 0;
        }

        input[strcspn(input, "\r\n")] = '\0';

        if (input[0] == '\0') {
            fprintf(stderr, "Entrada vacía.\n");
            return 1;
        }

        TokenList tl;
        if (tokenize(input, &tl) != 0) {
            fprintf(stderr, "Error léxico al tokenizar la entrada.\n");
            continue;
        }

        printf("\nTokens detectados:\n");
        for (size_t i = 0; i < tl.count; ++i) {
            Token *t = &tl.items[i];
            printf("   %2zu: %-18s '%s'\n", i, token_name(t->type), t->text);
            if (t->type == TK_END) break;
        }

        MoveAST m;
        if (parse_move(&tl, &m) != 0) {
            printf("\nError sintáctico al parsear '%s'\n\n", input);
            tokenlist_free(&tl);
            continue;
        }

        printf("\n");
        print_moveast(&m);

        char error_msg[256] = {0};
        int serr = board_apply_move(&board, &m, side_to_move,
                                    error_msg, sizeof(error_msg));

        if (serr == 0) {
            printf("\nMovimiento semánticamente válido. Tablero actualizado:\n");
            board_print(&board);

            PositionStatus st = board_evaluate_status(&board,
                                              (side_to_move == COLOR_WHITE)
                                                ? COLOR_BLACK
                                                : COLOR_WHITE);

            switch (st) {
                case POSITION_CHECK:
                    printf("La posición resultante es JAQUE al rival.\n\n");
                    break;
                case POSITION_CHECKMATE:
                    printf("La posición resultante es JAQUE MATE.\n\n");
                    break;
                case POSITION_STALEMATE:
                    printf("La posición resultante es TABLAS por ahogado.\n\n");
                    break;
                default:
                    break;
            }

            side_to_move = (side_to_move == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;

        } else {
            printf("\nError semántico: %s\n", error_msg);
            printf("\nEl tablero permanece igual:\n");
            board_print(&board);
        }

        tokenlist_free(&tl);
    }

    return 0;
}