// main.c - Punto de entrada principal
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "semant.h"
#include "pgn.h"
#include "interactivo.h"

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        printf("╔════════════════════════════════════╗\n");
        printf("║   MODO ANÁLISIS PGN                ║\n");
        printf("╚════════════════════════════════════╝\n\n");
        
        return pgn_mode(argv[1]);
    } else {
        printf("╔════════════════════════════════════╗\n");
        printf("║   MODO INTERACTIVO                 ║\n");
        printf("╚════════════════════════════════════╝\n\n");
        printf("Ingresa movimientos en notación SAN.\n");
        printf("Línea vacía para salir.\n\n");
        
        return interactive_mode();
    }
}