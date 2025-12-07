#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// listas de tokens
void tokenlist_init(TokenList *tl) {
    tl->items = NULL;
    tl->count = 0;
    tl->cap = 0;
}

// liberar memoria usada por la lista
void tokenlist_free(TokenList *tl) {
    free(tl->items);
    tl->items = NULL;
    tl->count = 0;
    tl->cap = 0;
}

// agregar token a la lista
void tokenlist_push(TokenList *tl, Token t) {
    if (tl->count == tl->cap) {
        size_t newcap = tl->cap ? tl->cap * 2 : 8;
        Token *tmp = realloc(tl->items, newcap * sizeof(Token));
        if (!tmp) { perror("realloc"); exit(EXIT_FAILURE); }
        tl->items = tmp;
        tl->cap = newcap;
    }
    tl->items[tl->count++] = t;
}

/* utilidades */
static int is_file_char(char c) { return (c >= 'a' && c <= 'h'); }
static int is_rank_char(char c) { return (c >= '1' && c <= '8'); }

const char* token_name(TokenType t) {
    switch (t) {
        case TK_UNKNOWN: return "TK_UNKNOWN";
        case TK_PIECE: return "TK_PIECE";
        case TK_FILE: return "TK_FILE";
        case TK_RANK: return "TK_RANK";
        case TK_CAPTURE: return "TK_CAPTURE";
        case TK_PROMOTE: return "TK_PROMOTE";
        case TK_PROMOTE_PIECE: return "TK_PROMOTE_PIECE";
        case TK_CHECK: return "TK_CHECK";
        case TK_MATE: return "TK_MATE";
        case TK_CASTLE_SHORT: return "TK_CASTLE_SHORT";
        case TK_CASTLE_LONG: return "TK_CASTLE_LONG";
        case TK_END: return "TK_END";
        default: return "(invalid)";
    }
}

/* match_at: compara pattern con line en posición i
   Permite tratar 'O' y '0' como equivalentes (para enroque).
   Retorna 1 si cabe el pattern y coincide, 0 si no. */
static int match_at(const char *line, size_t n, size_t i, const char *pattern) {
    size_t plen = strlen(pattern);
    if (i + plen > n) return 0; // no cabe
    for (size_t j = 0; j < plen; ++j) {
        char a = pattern[j];
        char b = line[i + j];
        if (a == 'O') {
            if (!(b == 'O' || b == '0')) return 0;
        } else {
            if (a != b) return 0;
        }
    }
    return 1;
}

int tokenize(const char *line, TokenList *out) {
    if (!line || !out) return -1;
    tokenlist_init(out);

    size_t n = strlen(line);
    size_t i = 0;

    // saltar espacios iniciales
    while (i < n && isspace((unsigned char)line[i])) i++;
    if (i >= n) return -1; // vacío

    while (i < n) {
        char c = line[i];
        if (isspace((unsigned char)c)) { i++; continue; }

        // Enroque: buscar la coincidencia más larga primero (O-O-O), usando match_at
        if (c == 'O' || c == '0') {
            if (match_at(line, n, i, "O-O-O")) {
                Token t = {TK_CASTLE_LONG, {0}};
                snprintf(t.text, sizeof t.text, "O-O-O");
                tokenlist_push(out, t);
                i += strlen("O-O-O");
                continue;
            }
            if (match_at(line, n, i, "O-O")) {
                Token t = {TK_CASTLE_SHORT, {0}};
                snprintf(t.text, sizeof t.text, "O-O");
                tokenlist_push(out, t);
                i += strlen("O-O");
                continue;
            }
            // si no coincide, tratar como unknown
            Token tu = {TK_UNKNOWN, {0}};
            tu.text[0] = c; tu.text[1] = '\0';
            tokenlist_push(out, tu);
            i++;
            continue;
        }

        // captura
        if (c == 'x' || c == 'X') {
            Token t = {TK_CAPTURE, {0}}; t.text[0] = 'x'; t.text[1] = '\0';
            tokenlist_push(out, t); i++; continue;
        }

        // check / mate
        if (c == '+') { Token t = {TK_CHECK, {0}}; t.text[0] = '+'; t.text[1] = '\0'; tokenlist_push(out, t); i++; continue; }
        if (c == '#') { Token t = {TK_MATE, {0}}; t.text[0] = '#'; t.text[1] = '\0'; tokenlist_push(out, t); i++; continue; }

        // promoción '=' opcionalmente seguida de pieza
        if (c == '=') {
            if (i + 1 < n && strchr("QRBN", line[i+1])) {
                Token t1 = {TK_PROMOTE, {0}}; t1.text[0] = '='; t1.text[1] = '\0'; tokenlist_push(out, t1);
                Token t2 = {TK_PROMOTE_PIECE, {0}}; t2.text[0] = line[i+1]; t2.text[1] = '\0'; tokenlist_push(out, t2);
                i += 2; continue;
            } else {
                Token t = {TK_PROMOTE, {0}}; t.text[0] = '='; t.text[1] = '\0'; tokenlist_push(out, t); i++; continue;
            }
        }

        // file (a-h)
        if (is_file_char(c)) { Token t = {TK_FILE, {0}}; t.text[0] = c; t.text[1] = '\0'; tokenlist_push(out, t); i++; continue; }

        // rank (1-8)
        if (is_rank_char(c)) { Token t = {TK_RANK, {0}}; t.text[0] = c; t.text[1] = '\0'; tokenlist_push(out, t); i++; continue; }

        // letra de pieza
        if (strchr("KQRBN", c)) { Token t = {TK_PIECE, {0}}; t.text[0] = c; t.text[1] = '\0'; tokenlist_push(out, t); i++; continue; }

        // unknown
        Token t = {TK_UNKNOWN, {0}}; t.text[0] = c; t.text[1] = '\0'; tokenlist_push(out, t); i++;
    }

    Token tend = {TK_END, {0}}; tend.text[0] = '\0'; tokenlist_push(out, tend);
    return 0;
}

