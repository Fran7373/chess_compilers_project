// pgn.c - Modo de análisis y replay de archivos PGN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "semant.h"
#include "pgn.h"

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

static int is_result_token(const char *s) {
    return (strcmp(s, "1-0") == 0 ||
            strcmp(s, "0-1") == 0 ||
            strcmp(s, "1/2-1/2") == 0 ||
            strcmp(s, "*") == 0);
}

static char *clean_pgn_text(const char *input) {
    size_t n = strlen(input);
    char *out = malloc(n + 1);
    if (!out) return NULL;
    
    size_t j = 0;
    int brace = 0, paren = 0;
    
    for (size_t i = 0; i < n; ++i) {
        char c = input[i];
        
        if (c == '{') { brace = 1; continue; }
        if (c == '}') { brace = 0; continue; }
        if (c == '(') { paren = 1; continue; }
        if (c == ')') { paren = 0; continue; }
        
        if (brace || paren) continue;
        
        if (isdigit((unsigned char)c)) {
            size_t k = i;
            while (k < n && isdigit((unsigned char)input[k])) k++;
            if (k < n && input[k] == '.') {
                while (k < n && input[k] == '.') k++;
                i = k - 1;
                continue;
            }
        }
        
        out[j++] = c;
    }
    out[j] = '\0';
    return out;
}

static void trim(char *s) {
    if (!s) return;
    
    char *p = s;
    int len = strlen(s);
    
    while(len > 0 && isspace((unsigned char)s[len-1])) 
        s[--len] = '\0';
    
    while(*p && isspace((unsigned char)*p)) 
        p++;
    
    if (p != s) 
        memmove(s, p, strlen(p) + 1);
}

// ============================================================================
// MANEJO DE ESTRUCTURAS PGN
// ============================================================================

void pgn_game_init(PGNGame *game) {
    memset(game, 0, sizeof(PGNGame));
    game->move_capacity = 100;
    game->moves = malloc(sizeof(GameMove) * game->move_capacity);
}

void pgn_game_free(PGNGame *game) {
    if (game->moves) {
        free(game->moves);
        game->moves = NULL;
    }
}

void pgn_collection_init(PGNCollection *col) {
    col->game_capacity = 10;
    col->game_count = 0;
    col->games = malloc(sizeof(PGNGame) * col->game_capacity);
}

void pgn_collection_free(PGNCollection *col) {
    for (int i = 0; i < col->game_count; i++) {
        pgn_game_free(&col->games[i]);
    }
    free(col->games);
}

static void pgn_game_add_move(PGNGame *game, const char *move_text, 
                              const MoveAST *ast, const Board *board, 
                              Color side) {
    if (game->move_count >= game->move_capacity) {
        game->move_capacity *= 2;
        game->moves = realloc(game->moves, sizeof(GameMove) * game->move_capacity);
    }
    
    GameMove *gm = &game->moves[game->move_count++];
    strncpy(gm->move_text, move_text, sizeof(gm->move_text) - 1);
    gm->ast = *ast;
    gm->board_state = *board;
    gm->side_to_move = side;
}

// ============================================================================
// PARSING DE HEADERS
// ============================================================================

static void parse_pgn_header(const char *line, PGNGame *game) {
    if (strncmp(line, "[Event ", 7) == 0) {
        const char *start = strchr(line, '"');
        const char *end = start ? strrchr(line, '"') : NULL;
        if (start && end && end > start) {
            int len = end - start - 1;
            if (len > 0 && len < 255) {
                strncpy(game->event, start + 1, len);
                game->event[len] = '\0';
            }
        }
    } else if (strncmp(line, "[White ", 7) == 0) {
        const char *start = strchr(line, '"');
        const char *end = start ? strrchr(line, '"') : NULL;
        if (start && end && end > start) {
            int len = end - start - 1;
            if (len > 0 && len < 127) {
                strncpy(game->white, start + 1, len);
                game->white[len] = '\0';
            }
        }
    } else if (strncmp(line, "[Black ", 7) == 0) {
        const char *start = strchr(line, '"');
        const char *end = start ? strrchr(line, '"') : NULL;
        if (start && end && end > start) {
            int len = end - start - 1;
            if (len > 0 && len < 127) {
                strncpy(game->black, start + 1, len);
                game->black[len] = '\0';
            }
        }
    } else if (strncmp(line, "[Result ", 8) == 0) {
        const char *start = strchr(line, '"');
        const char *end = start ? strrchr(line, '"') : NULL;
        if (start && end && end > start) {
            int len = end - start - 1;
            if (len > 0 && len < 15) {
                strncpy(game->result, start + 1, len);
                game->result[len] = '\0';
            }
        }
    }
}

// ============================================================================
// VALIDACIÓN Y CARGA DE PARTIDAS
// ============================================================================

static int validate_and_load_game(PGNGame *game, const char *moves_buffer, int game_number) {
    Board board;
    board_init_start(&board);
    Color side = COLOR_WHITE;
    int move_num = 0;
    
    char *clean = clean_pgn_text(moves_buffer);
    if (!clean) {
        fprintf(stderr, "❌ Partida #%d: Error al limpiar texto PGN\n", game_number);
        return -1;
    }
    
    char *tok = strtok(clean, " \t\r\n");
    while (tok) {
        if (is_result_token(tok)) break;
        
        move_num++;
        
        TokenList tl;
        if (tokenize(tok, &tl) != 0) {
            fprintf(stderr, "❌ Partida #%d - ERROR LÉXICO en movimiento %d: '%s'\n", 
                    game_number, move_num, tok);
            fprintf(stderr, "   La partida no será cargada.\n");
            free(clean);
            return -1;
        }
        
        MoveAST ast;
        if (parse_move(&tl, &ast) != 0) {
            fprintf(stderr, "❌ Partida #%d - ERROR SINTÁCTICO en movimiento %d: '%s'\n", 
                    game_number, move_num, tok);
            fprintf(stderr, "   El movimiento no cumple con la notación SAN estándar.\n");
            tokenlist_free(&tl);
            free(clean);
            return -1;
        }
        
        char err[256] = {0};
        if (board_apply_move(&board, &ast, side, err, sizeof(err)) != 0) {
            fprintf(stderr, "❌ Partida #%d - ERROR SEMÁNTICO en movimiento %d: '%s'\n", 
                    game_number, move_num, tok);
            fprintf(stderr, "   Razón: %s\n", err);
            fprintf(stderr, "   La partida no será cargada.\n");
            tokenlist_free(&tl);
            free(clean);
            return -1;
        }
        
        pgn_game_add_move(game, tok, &ast, &board, side);
        side = (side == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
        
        tokenlist_free(&tl);
        tok = strtok(NULL, " \t\r\n");
    }
    
    free(clean);
    
    if (move_num == 0) {
        fprintf(stderr, "❌ Partida #%d: No contiene movimientos válidos\n", game_number);
        return -1;
    }
    
    return 0;
}

static int load_pgn_games(const char *path, PGNCollection *col) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "No se puede abrir archivo PGN: %s\n", path);
        return -1;
    }
    
    char line[1024];
    PGNGame temp_game;
    char moves_buffer[32768] = {0};
    int in_moves = 0;
    int game_number = 0;
    int has_current_game = 0;
    int valid_games = 0;
    int invalid_games = 0;
    
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        
        if (strncmp(line, "[Event ", 7) == 0) {
            if (has_current_game && moves_buffer[0]) {
                game_number++;
                
                printf("Validando partida #%d: %s vs %s...\n", 
                       game_number,
                       temp_game.white[0] ? temp_game.white : "?",
                       temp_game.black[0] ? temp_game.black : "?");
                
                if (validate_and_load_game(&temp_game, moves_buffer, game_number) == 0) {
                    if (col->game_count >= col->game_capacity) {
                        col->game_capacity *= 2;
                        col->games = realloc(col->games, sizeof(PGNGame) * col->game_capacity);
                    }
                    col->games[col->game_count++] = temp_game;
                    printf("✓ Partida #%d cargada exitosamente (%d movimientos)\n\n", 
                           game_number, temp_game.move_count);
                    valid_games++;
                } else {
                    pgn_game_free(&temp_game);
                    printf("\n");
                    invalid_games++;
                }
            }
            
            pgn_game_init(&temp_game);
            moves_buffer[0] = '\0';
            in_moves = 0;
            has_current_game = 1;
        }
        
        if (line[0] == '[' && has_current_game) {
            parse_pgn_header(line, &temp_game);
            in_moves = 0;
        }
        else if (line[0] == '\0' && has_current_game) {
            in_moves = 1;
        }
        else if (in_moves && line[0] != '\0' && has_current_game) {
            strcat(moves_buffer, " ");
            strcat(moves_buffer, line);
        }
    }
    
    if (has_current_game && moves_buffer[0]) {
        game_number++;
        
        printf("Validando partida #%d: %s vs %s...\n", 
               game_number,
               temp_game.white[0] ? temp_game.white : "?",
               temp_game.black[0] ? temp_game.black : "?");
        
        if (validate_and_load_game(&temp_game, moves_buffer, game_number) == 0) {
            if (col->game_count >= col->game_capacity) {
                col->game_capacity *= 2;
                col->games = realloc(col->games, sizeof(PGNGame) * col->game_capacity);
            }
            col->games[col->game_count++] = temp_game;
            printf("✓ Partida #%d cargada exitosamente (%d movimientos)\n\n", 
                   game_number, temp_game.move_count);
            valid_games++;
        } else {
            pgn_game_free(&temp_game);
            printf("\n");
            invalid_games++;
        }
    }
    
    fclose(f);
    
    printf("════════════════════════════════════════════════════════════\n");
    printf("Resumen de carga:\n");
    printf("  ✓ Partidas válidas:   %d\n", valid_games);
    printf("  ❌ Partidas inválidas: %d\n", invalid_games);
    printf("  📊 Total procesadas:  %d\n", game_number);
    printf("════════════════════════════════════════════════════════════\n\n");
    
    return 0;
}

// ============================================================================
// MODO REPLAY
// ============================================================================

static void display_game_header(const PGNGame *game) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║ Event:  %-50s ║\n", game->event[0] ? game->event : "Unknown");
    printf("║ White:  %-50s ║\n", game->white[0] ? game->white : "?");
    printf("║ Black:  %-50s ║\n", game->black[0] ? game->black : "?");
    printf("║ Result: %-50s ║\n", game->result[0] ? game->result : "*");
    printf("╚════════════════════════════════════════════════════════════╝\n");
}

static void replay_game(PGNGame *game) {
    int current_move = -1;
    Board display_board;
    
    display_game_header(game);
    
    printf("\nComandos:\n");
    printf("  [Enter] o 'n' = Siguiente movimiento\n");
    printf("  'b' = Movimiento anterior\n");
    printf("  'j <num>' = Saltar a movimiento <num>\n");
    printf("  'q' = Salir del replay\n\n");
    
    board_init_start(&display_board);
    board_print(&display_board);
    printf("\nPosición inicial (0/%d movimientos)\n", game->move_count);
    
    char input[256];
    while (1) {
        printf("\nreplay> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        
        input[strcspn(input, "\r\n")] = '\0';
        trim(input);
        
        if (input[0] == '\0' || strcmp(input, "n") == 0) {
            if (current_move + 1 < game->move_count) {
                current_move++;
                display_board = game->moves[current_move].board_state;
                printf("\n");
                board_print(&display_board);
                printf("\nMovimiento %d/%d: %s (%s)\n", 
                       current_move + 1, game->move_count,
                       game->moves[current_move].move_text,
                       game->moves[current_move].side_to_move == COLOR_WHITE ? "Blancas" : "Negras");
            } else {
                printf("Ya estás en el último movimiento\n");
            }
        }
        else if (strcmp(input, "b") == 0) {
            if (current_move >= 0) {
                current_move--;
                if (current_move >= 0) {
                    display_board = game->moves[current_move].board_state;
                } else {
                    board_init_start(&display_board);
                }
                printf("\n");
                board_print(&display_board);
                if (current_move >= 0) {
                    printf("\nMovimiento %d/%d: %s (%s)\n", 
                           current_move + 1, game->move_count,
                           game->moves[current_move].move_text,
                           game->moves[current_move].side_to_move == COLOR_WHITE ? "Blancas" : "Negras");
                } else {
                    printf("\nPosición inicial (0/%d movimientos)\n", game->move_count);
                }
            } else {
                printf("Ya estás en la posición inicial\n");
            }
        }
        else if (input[0] == 'j' && input[1] == ' ') {
            int target = atoi(input + 2);
            if (target < 0) {
                current_move = -1;
                board_init_start(&display_board);
                printf("\n");
                board_print(&display_board);
                printf("\nPosición inicial (0/%d movimientos)\n", game->move_count);
            } else if (target > 0 && target <= game->move_count) {
                current_move = target - 1;
                display_board = game->moves[current_move].board_state;
                printf("\n");
                board_print(&display_board);
                printf("\nMovimiento %d/%d: %s (%s)\n", 
                       current_move + 1, game->move_count,
                       game->moves[current_move].move_text,
                       game->moves[current_move].side_to_move == COLOR_WHITE ? "Blancas" : "Negras");
            } else {
                printf("Movimiento fuera de rango (1-%d)\n", game->move_count);
            }
        }
        else if (strcmp(input, "q") == 0) {
            break;
        }
        else {
            printf("Comando no reconocido. Usa Enter/'n' (siguiente), 'b' (anterior), 'j <num>' (saltar), 'q' (salir)\n");
        }
    }
}

// ============================================================================
// FUNCIÓN PRINCIPAL DEL MODO PGN
// ============================================================================

int pgn_mode(const char *path) {
    PGNCollection col;
    pgn_collection_init(&col);
    
    printf("Cargando partidas desde: %s\n", path);
    
    if (load_pgn_games(path, &col) != 0) {
        pgn_collection_free(&col);
        return -1;
    }
    
    printf("\n✓ Se cargaron %d partida(s)\n\n", col.game_count);
    
    if (col.game_count == 0) {
        printf("No se encontraron partidas válidas en el archivo\n");
        pgn_collection_free(&col);
        return -1;
    }
    
    while (1) {
        printf("\n════════════════════════════════════════════════════════════\n");
        printf("PARTIDAS DISPONIBLES:\n");
        printf("════════════════════════════════════════════════════════════\n");
        for (int i = 0; i < col.game_count; i++) {
            printf("[%d] %s: %s vs %s (%d movimientos) - %s\n", 
                   i + 1,
                   col.games[i].event[0] ? col.games[i].event : "Sin título",
                   col.games[i].white[0] ? col.games[i].white : "?",
                   col.games[i].black[0] ? col.games[i].black : "?",
                   col.games[i].move_count,
                   col.games[i].result[0] ? col.games[i].result : "*");
        }
        printf("════════════════════════════════════════════════════════════\n");
        
        char input[256];
        int selected = -1;
        
        printf("\nSeleccione partida (1-%d) o 'q' para salir del programa: ", col.game_count);
        if (!fgets(input, sizeof(input), stdin)) break;
        
        input[strcspn(input, "\r\n")] = '\0';
        trim(input);
        
        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
            printf("\nSaliendo del programa...\n");
            pgn_collection_free(&col);
            return 0;
        }
        
        selected = atoi(input) - 1;
        if (selected < 0 || selected >= col.game_count) {
            printf("Selección inválida. Intente de nuevo.\n");
            continue;
        }
        
        replay_game(&col.games[selected]);
        
        printf("\n¿Desea ver otra partida? (Presione Enter para continuar)\n");
    }
    
    pgn_collection_free(&col);
    return 0;
}