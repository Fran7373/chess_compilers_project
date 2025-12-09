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
int partida_normal_mode(void)
{
    Board board;
    board_init_start(&board);

    Color side = COLOR_WHITE;
    char input[32];

    while (1) {
        board_print(&board);
        printf("%s mueve (formato: e2e4, ENTER para salir): ",
               side == COLOR_WHITE ? "Blancas" : "Negras");

        if (!fgets(input, sizeof(input), stdin)) {
            // EOF: salir de este modo y del programa
            printf("No input (EOF).\n");
            return 0;
        }

        // quitar \n
        input[strcspn(input, "\r\n")] = 0;

        // ENTER vacío -> volver al menú
        if (input[0] == '\0') {
            printf("Saliendo de partida normal.\n");
            return 1;
        }

        // validar formato e2e4 (4 chars)
        if (strlen(input) != 4 ||
            input[0] < 'a' || input[0] > 'h' ||
            input[1] < '1' || input[1] > '8' ||
            input[2] < 'a' || input[2] > 'h' ||
            input[3] < '1' || input[3] > '8') 
        {
            printf("Formato inválido. Use algo como e2e4.\n");
            continue;
        }

        int sf = file_to_index(input[0]);   // origen columna
        int sr = rank_to_index(input[1]);   // origen fila
        int df = file_to_index(input[2]);   // destino columna
        int dr = rank_to_index(input[3]);   // destino fila

        Piece p = board.board[sr][sf];
        Piece dest = board.board[dr][df];

        if (p.type == PIECE_NONE || p.color != side) {
            printf("No hay pieza propia en la casilla de origen.\n");
            continue;
        }

        char san[32] = {0};
        int isCapture = 0;

        // ---------------------------
        //  Validación geométrica básica
        // ---------------------------
        if (p.type == PIECE_PAWN) {
            int dir = (side == COLOR_WHITE) ? 1 : -1;
            int d_rank = dr - sr;
            int d_file = df - sf;

            // avance simple (misma columna)
            if (d_file == 0) {
                if (d_rank == dir) {
                    // un paso: destino debe estar vacío
                    if (dest.type != PIECE_NONE) {
                        printf("Movimiento ilegal: el peón no puede avanzar si hay pieza delante.\n");
                        continue;
                    }
                }
                else if (d_rank == 2 * dir) {
                    int start_rank = (side == COLOR_WHITE) ? 1 : 6;
                    if (sr != start_rank) {
                        printf("Movimiento ilegal: el peón solo puede avanzar dos casillas desde su fila inicial.\n");
                        continue;
                    }
                    int intermediate_rank = sr + dir;
                    if (board.board[intermediate_rank][sf].type != PIECE_NONE ||
                        dest.type != PIECE_NONE) 
                    {
                        printf("Movimiento ilegal: hay piezas bloqueando el avance de dos casillas.\n");
                        continue;
                    }
                }
                else {
                    printf("Movimiento ilegal de peón.\n");
                    continue;
                }
                // avance recto → no captura
                isCapture = 0;
            }
            // posible captura (diagonal)
            else if ((d_file == 1 || d_file == -1) && d_rank == dir) {
                if (dest.type != PIECE_NONE && dest.color != side) {
                    isCapture = 1; // captura normal
                } else if (dest.type == PIECE_NONE &&
                           board.en_passant_file == df &&
                           board.en_passant_rank == dr) {
                    // captura al paso potencial: dejamos que semántica lo valide
                    isCapture = 1;
                } else {
                    printf("Movimiento ilegal: el peón solo se mueve en diagonal si captura.\n");
                    continue;
                }
            } else {
                printf("Movimiento ilegal de peón.\n");
                continue;
            }

            // Construir SAN para peón
            if (isCapture) {
                // ejemplo: h7g6 con captura sería "hxg6"
                snprintf(san, sizeof(san),
                         "%cx%c%c", input[0], input[2], input[3]);
            } else {
                // ejemplo: e2e4 -> "e4"
                snprintf(san, sizeof(san),
                         "%c%c", input[2], input[3]);
            }
        }
        else {
            // Piezas distintas de peón
            char pieceChar = 0;
            switch (p.type) {
                case PIECE_KNIGHT: pieceChar = 'N'; break;
                case PIECE_BISHOP: pieceChar = 'B'; break;
                case PIECE_ROOK:   pieceChar = 'R'; break;
                case PIECE_QUEEN:  pieceChar = 'Q'; break;
                case PIECE_KING:   pieceChar = 'K'; break;
                default: pieceChar = '?'; break;
            }

            if (pieceChar == '?') {
                printf("Tipo de pieza no soportado.\n");
                continue;
            }

            // determinar si es captura
            if (dest.type != PIECE_NONE && dest.color != side) {
                isCapture = 1;
            } else {
                isCapture = 0;
            }

            // para respetar el origen, usamos siempre FILE RANK FILE RANK
            // y su versión con captura: FILE RANK 'x' FILE RANK
            if (isCapture) {
                // ejemplo: caballo de h7 a f6 capturando -> "Nh7xf6"
                snprintf(san, sizeof(san),
                         "%c%c%cx%c%c",
                         pieceChar, input[0], input[1],
                         input[2], input[3]);
            } else {
                // ejemplo: caballo de b1 a c3 -> "Nb1c3"
                snprintf(san, sizeof(san),
                         "%c%c%c%c%c",
                         pieceChar, input[0], input[1],
                         input[2], input[3]);
            }
        }

        printf("Movimiento convertido a SAN: %s\n", san);

        // ---------------------------
        //  Pasar por lexer + parser + semántica
        // ---------------------------
        TokenList tl;
        if (tokenize(san, &tl) != 0) {
            printf("Error léxico al tokenizar SAN.\n");
            continue;
        }

        MoveAST mv;
        if (parse_move(&tl, &mv) != 0) {
            printf("Error sintáctico al parsear SAN.\n");
            tokenlist_free(&tl);
            continue;
        }

        char errmsg[256] = {0};
        if (board_apply_move(&board, &mv, side, errmsg, sizeof(errmsg)) != 0) {
            printf("Error semántico: %s\n", errmsg);
            tokenlist_free(&tl);
            continue;
        }

        tokenlist_free(&tl);

        // cambiar turno
        side = (side == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    }

    // en teoría no se llega aquí
    return 1;
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