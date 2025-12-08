// src/lexer_test.c
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "board.h"

// =============================================================
//   Conversión desde coordenadas (src, dst) a SAN
// =============================================================
char *square_to_san(char *buf, const char *src, const char *dst, const Board *b) {

    int sf = src[0] - 'a';
    int sr = src[1] - '1';
    int df = dst[0] - 'a';
    int dr = dst[1] - '1';

    const Piece *p    = &b->board[sr][sf];
    const Piece *dest = &b->board[dr][df];

    if (p->type == PIECE_NONE) {
        strcpy(buf, "");
        return buf;
    }

    char pieceChar;
    switch (p->type) {
        case PIECE_PAWN:   pieceChar = 0;   break;
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

    if (p->type == PIECE_PAWN) {
        if (is_capture) {
            snprintf(buf, 16, "%c%c%c%c", src[0], 'x', dst[0], dst[1]);
        } else {
            snprintf(buf, 16, "%c%c", dst[0], dst[1]);
        }
        return buf;
    }

    if (is_capture) {
        snprintf(buf, 16, "%c%c%c%c", pieceChar, 'x', dst[0], dst[1]);
    } else {
        snprintf(buf, 16, "%c%c%c", pieceChar, dst[0], dst[1]);
    }

    return buf;
}

// =============================================================
//   Jugar usando origen/destino
// =============================================================
void jugar_interfaz() {

    Board board;
    board_init_start(&board);
    board_print(&board);
    Color side_to_move = COLOR_WHITE;

    while (1) {

        printf("\n(Escriba 0 para volver al menú)\n");

        if (side_to_move == COLOR_WHITE)
            printf("\n>>> Turno de las BLANCAS <<<\n");
        else
            printf("\n>>> Turno de las NEGRAS <<<\n");

        char src[3], dst[3], san[16];

        printf("Tomar ficha (ej: e2): ");
        scanf("%2s", src);

        if (strcmp(src, "0") == 0) return;

        printf("Mover ficha (ej: e4): ");
        scanf("%2s", dst);

        if (strcmp(dst, "0") == 0) return;

        square_to_san(san, src, dst, &board);
        printf("SAN generado: %s\n", san);

        TokenList tl;
        tokenize(san, &tl);

        MoveAST m;
        if (parse_move(&tl, &m) != 0) {
            printf("Error sintactico.\n");
            tokenlist_free(&tl);
            continue;
        }

        char error_msg[256];
        if (board_apply_move(&board, &m, side_to_move, error_msg, sizeof(error_msg)) == 0) {
            board_print(&board);
            side_to_move = (side_to_move == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE);
        } else {
            printf("Error semantico: %s\n", error_msg);
            board_print(&board);
        }

        tokenlist_free(&tl);
    }
}

// =============================================================
//   Jugar ingresando SAN directamente
// =============================================================
void jugar_san() {

    Board board;
    board_init_start(&board);
    board_print(&board);
    Color side_to_move = COLOR_WHITE;

    char input[256];

    while (1) {

        printf("\n(Escriba 0 para volver al menú)\n");

        if (side_to_move == COLOR_WHITE)
            printf("\n>>> Turno de las BLANCAS <<<\n");
        else
            printf("\n>>> Turno de las NEGRAS <<<\n");

        printf("Ingrese jugada SAN: ");

        if (!fgets(input, sizeof(input), stdin)) {
            printf("Entrada invalida.\n");
            return;
        }

        input[strcspn(input, "\r\n")] = 0;

        if (strcmp(input, "0") == 0) return;

        TokenList tl;
        tokenize(input, &tl);

        MoveAST m;
        if (parse_move(&tl, &m) != 0) {
            printf("Error sintactico.\n");
            tokenlist_free(&tl);
            continue;
        }

        char error_msg[256];
        if (board_apply_move(&board, &m, side_to_move, error_msg, sizeof(error_msg)) == 0) {
            board_print(&board);
            side_to_move = (side_to_move == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE);
        } else {
            printf("Error semantico: %s\n", error_msg);
            board_print(&board);
        }

        tokenlist_free(&tl);
    }
}

// =============================================================
//   Cargar PGN
// =============================================================
void cargar_pgn() {

    char filename[256];
    printf("Nombre del archivo PGN (0 para volver): ");
    scanf("%255s", filename);

    if (strcmp(filename, "0") == 0) return;

    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("No se pudo abrir el archivo.\n");
        return;
    }

    Board board;
    board_init_start(&board);
    board_print(&board);
    Color side_to_move = COLOR_WHITE;

    char san[64];

    while (fscanf(f, "%63s", san) == 1) {

        if (strchr(san, '.')) continue;

        printf("\nLeyendo movimiento: %s\n", san);

        TokenList tl;
        tokenize(san, &tl);

        MoveAST m;
        if (parse_move(&tl, &m) != 0) {
            printf("Error sintactico en %s\n", san);
            tokenlist_free(&tl);
            continue;
        }

        char error_msg[256];
        if (board_apply_move(&board, &m, side_to_move, error_msg, sizeof(error_msg)) == 0) {
            board_print(&board);
            side_to_move = (side_to_move == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE);
        } else {
            printf("Error semantico en %s: %s\n", san, error_msg);
        }

        tokenlist_free(&tl);
    }

    fclose(f);
}

// =============================================================
//                 MENU PRINCIPAL
// =============================================================
int main(void) {
    system("chcp 65001 > nul");
    int opcion = 0;

    while (1) {
        printf("\n========= MENU PRINCIPAL =========\n");
        printf("1. Jugar partida (origen/destino)\n");
        printf("2. Jugar ingresando SAN\n");
        printf("3. Cargar archivo PGN\n");
        printf("4. Salir\n");
        printf("Seleccione una opcion: ");

        if (scanf("%d", &opcion) != 1) {
            printf("Entrada invalida.\n");
            return 0;
        }

        getchar(); // limpiar salto de línea pendiente

        if (opcion == 1)      jugar_interfaz();
        else if (opcion == 2) jugar_san();
        else if (opcion == 3) cargar_pgn();
        else if (opcion == 4) {
            printf("Saliendo...\n");
            return 0;
        }
        else printf("Opcion no valida.\n");
    }
}
