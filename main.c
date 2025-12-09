// main.c - Punto de entrada principal
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "semant.h"
#include "pgn.h"
#include "interactivo.h"

int main(int argc, char *argv[]) 
{
    // ----------------------------------------
    // MODO PGN (cuando se pasa archivo por argv)
    // ----------------------------------------
    if (argc >= 2) {
        printf("╔════════════════════════════════════╗\n");
        printf("║          MODO ANÁLISIS PGN         ║\n");
        printf("╚════════════════════════════════════╝\n\n");
        
        pgn_mode(argv[1]);   // NO return → permite volver al menú
    }

    // ----------------------------------------
    // MODO SIN ARCHIVO → elegimos entre opciones
    // ----------------------------------------
    int opcion = 0;

    while (1) {

        printf("╔════════════════════════════════════╗\n");
        printf("║             MENÚ PRINCIPAL         ║\n");
        printf("╠════════════════════════════════════╣\n");
        printf("║  1. Partida normal                 ║\n");
        printf("║  2. Ingresar movimientos SAN       ║\n");
        printf("║  3. Salir                          ║\n");
        printf("╚════════════════════════════════════╝\n");
        printf("Seleccione una opción: ");

        if (scanf("%d", &opcion) != 1) {
            printf("Entrada inválida.\n");
            return 1;
        }

        getchar(); // limpiar salto

if (opcion == 1) {
    int r = partida_normal_mode();
    if (r == 1) continue;
    else return 0;
}

else if (opcion == 2) {
    printf("\nEntrando al modo SAN...\n\n");

    int r = interactive_mode();

    if (r == 1) {
        printf("\nRegresando al menú principal...\n\n");
        continue;           // ← Vuelve al menú
    } else {
        printf("\nSaliendo del programa...\n\n");
        return 0;           // ← Finaliza ejecución
    }
}
        else if (opcion == 3) {
            printf("\nSaliendo...\n");
            break;   // ← sale del while → termina main
        }
        else {
            printf("\nOpción inválida, intente de nuevo.\n\n");
        }
    }

    return 0;
}
