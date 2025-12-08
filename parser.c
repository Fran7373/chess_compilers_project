// src/parser.c  (actualizado: soporte para PIECE FILE RANK FILE RANK y PIECE FILE RANK x FILE RANK)
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// helper: devuelve token en posición i o NULL
static const Token* tok_at(const TokenList *tl, size_t i) {
    if (!tl || i >= tl->count) return NULL;
    return &tl->items[i];
}

// helper: concatena texto bruto del movimiento en out->raw
static void build_raw(MoveAST *out, const TokenList *tl) {
    out->raw[0] = '\0';
    for (size_t i = 0; i < tl->count; ++i) {
        const Token *t = &tl->items[i];
        if (t->type == TK_END) break;
        strncat(out->raw, t->text, sizeof(out->raw) - strlen(out->raw) - 1);
    }
}

// helper: asegura que el siguiente token sea TK_END; si no lo es devuelve error (-1)
static int ensure_no_extra_tokens(const TokenList *tokens, size_t idx) {
    const Token *rem = tok_at(tokens, idx);
    if (rem && rem->type != TK_END) {
        fprintf(stderr, "parse_move: token inesperado tras movimiento: '%s' (tipo %d)\n", rem->text, (int)rem->type);
        return -1;
    }
    return 0;
}

int parse_move(const TokenList *tokens, MoveAST *out) {
    if (!tokens || !out) {
        fprintf(stderr, "parse_move: argumentos nulos\n");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    build_raw(out, tokens);

    size_t i = 0;
    const Token *t = tok_at(tokens, i);
    if (!t) return -1;

    // Manejo de enroque (tokens TK_CASTLE_LONG / TK_CASTLE_SHORT)
    if (t->type == TK_CASTLE_LONG) {
        out->is_castle_long = 1;
        i++;
        // opcional + o #
        const Token *t2 = tok_at(tokens, i);
        if (t2 && t2->type == TK_CHECK) { out->is_check = 1; i++; t2 = tok_at(tokens, i); }
        if (t2 && t2->type == TK_MATE) { out->is_mate = 1; i++; }

        // asegurar que no queden tokens extra
        if (ensure_no_extra_tokens(tokens, i) != 0) return -1;
        return 0;
    }
    if (t->type == TK_CASTLE_SHORT) {
        out->is_castle_short = 1;
        i++;
        const Token *t2 = tok_at(tokens, i);
        if (t2 && t2->type == TK_CHECK) { out->is_check = 1; i++; t2 = tok_at(tokens, i); }
        if (t2 && t2->type == TK_MATE) { out->is_mate = 1; i++; }

        if (ensure_no_extra_tokens(tokens, i) != 0) return -1;
        return 0;
    }

    // Determinar si es movimiento de pieza o peón
    t = tok_at(tokens, i);
    if (!t) { fprintf(stderr, "parse_move: tokens vacíos\n"); return -1; }

    if (t->type == TK_PIECE) {
        // movimiento de pieza
        out->piece = t->text[0];
        i++;
        // tokens próximos
        const Token *a = tok_at(tokens, i);
        const Token *b = tok_at(tokens, i+1);
        const Token *c = tok_at(tokens, i+2);
        const Token *d = tok_at(tokens, i+3);
        const Token *e = tok_at(tokens, i+4);

        // ------------------------------------------------------------
        // NUEVO: Patrón FILE RANK FILE RANK  (ej. Qh4e1)  -> src_file+src_rank, dest_file+dest_rank
        // y variante con captura: FILE RANK CAPTURE FILE RANK (Qh4xe1)
        // ------------------------------------------------------------
        if (a && b && c && d && a->type == TK_FILE && b->type == TK_RANK && c->type == TK_FILE && d->type == TK_RANK) {
            out->src_file = a->text[0];
            out->src_rank = b->text[0];
            out->dest_file = c->text[0];
            out->dest_rank = d->text[0];
            i += 4;
        } else if (a && b && c && d && e && a->type == TK_FILE && b->type == TK_RANK && c->type == TK_CAPTURE && d->type == TK_FILE && e->type == TK_RANK) {
            // Qh4xe1
            out->src_file = a->text[0];
            out->src_rank = b->text[0];
            out->is_capture = 1;
            out->dest_file = d->text[0];
            out->dest_rank = e->text[0];
            i += 5;
        }
        // ------------------------------------------------------------
        // Patrón FILE FILE RANK  -> src_file, dest_file, dest_rank  (ej. Raxb1 -> a b 1)
        else if (a && b && c && a->type == TK_FILE && b->type == TK_FILE && c->type == TK_RANK) {
            out->src_file = a->text[0];
            out->dest_file = b->text[0];
            out->dest_rank = c->text[0];
            i += 3;
        }
        // RANK FILE RANK -> src_rank, dest_file, dest_rank (ej. N1c3)
        else if (a && b && c && a->type == TK_RANK && b->type == TK_FILE && c->type == TK_RANK) {
            out->src_rank = a->text[0];
            out->dest_file = b->text[0];
            out->dest_rank = c->text[0];
            i += 3;
        }
        // FILE CAPTURE FILE RANK -> src_file, capture, dest (ej. Raxb1)
        else if (a && b && c && d && a->type == TK_FILE && b->type == TK_CAPTURE && c->type == TK_FILE && d->type == TK_RANK) {
            out->src_file = a->text[0];
            out->is_capture = 1;
            out->dest_file = c->text[0];
            out->dest_rank = d->text[0];
            i += 4;
        }
        // CAPTURE FILE RANK -> capture + dest (sin desambiguación) e.g. Nxd4
        else if (a && a->type == TK_CAPTURE && b && b->type == TK_FILE && c && c->type == TK_RANK) {
            out->is_capture = 1;
            out->dest_file = b->text[0];
            out->dest_rank = c->text[0];
            i += 3;
        }
        // FILE RANK -> destino directo (ej. Nf3)
        else if (a && b && a->type == TK_FILE && b->type == TK_RANK) {
            out->dest_file = a->text[0];
            out->dest_rank = b->text[0];
            i += 2;
        }
        // caso: desambiguación antes del 'x', e.g. Nfxe5  (handled as a + b == FILE + CAPTURE above but ensure)
        else if (a && b && a->type == TK_FILE && b->type == TK_CAPTURE) {
            const Token *c2 = tok_at(tokens, i+2);
            const Token *d2 = tok_at(tokens, i+3);
            if (c2 && c2->type == TK_FILE && d2 && d2->type == TK_RANK) {
                out->src_file = a->text[0];
                out->is_capture = 1;
                out->dest_file = c2->text[0];
                out->dest_rank = d2->text[0];
                i += 4;
            } else {
                fprintf(stderr, "parse_move: sintaxis inesperada tras desambiguación y captura.\n");
                return -1;
            }
        }
        else {
            fprintf(stderr, "parse_move: patrón de movimiento de pieza no reconocido (tokens alrededor de índice %zu).\n", i);
            return -1;
        }

        // promocion (rara en piezas, pero por si aparece)
        const Token *p = tok_at(tokens, i);
        if (p && p->type == TK_PROMOTE) {
            const Token *pp = tok_at(tokens, i+1);
            if (pp && pp->type == TK_PROMOTE_PIECE) {
                out->promotion = pp->text[0];
                i += 2;
            } else { i += 1; }
        }

        // check / mate
        p = tok_at(tokens, i);
        if (p && p->type == TK_CHECK) { out->is_check = 1; i++; p = tok_at(tokens, i); }
        if (p && p->type == TK_MATE) { out->is_mate = 1; i++; }

        // asegurar que no queden tokens inesperados
        if (ensure_no_extra_tokens(tokens, i) != 0) return -1;

        return 0;
    } else {
        // movimiento de peón (no hay token TK_PIECE al inicio)
        out->piece = 'P';
        // formatos: FILE RANK  (e4)
        //           FILE CAPTURE FILE RANK  (exd5)
        //           FILE RANK PROMOTE... (e8=Q)
        const Token *a = tok_at(tokens, i);
        const Token *b = tok_at(tokens, i+1);
        const Token *c = tok_at(tokens, i+2);
        const Token *d = tok_at(tokens, i+3);

        if (!a) { fprintf(stderr, "parse_move: fin inesperado en movimiento de peón\n"); return -1; }

        // caso captura: exd5
        if (a->type == TK_FILE && b && b->type == TK_CAPTURE && c && c->type == TK_FILE && d && d->type == TK_RANK) {
            out->src_file = a->text[0]; // columna origen del peón
            out->is_capture = 1;
            out->dest_file = c->text[0];
            out->dest_rank = d->text[0];
            i += 4;
        }
        // caso simple: e4
        else if (a->type == TK_FILE && b && b->type == TK_RANK) {
            out->dest_file = a->text[0];
            out->dest_rank = b->text[0];
            i += 2;
        }
        else {
            fprintf(stderr, "parse_move: formato inválido para movimiento de peón cerca del token %zu\n", i);
            return -1;
        }

        // promoción opcional: '=' + piece
        const Token *p = tok_at(tokens, i);
        if (p && p->type == TK_PROMOTE) {
            const Token *pp = tok_at(tokens, i+1);
            if (pp && pp->type == TK_PROMOTE_PIECE) {
                out->promotion = pp->text[0];
                i += 2;
            } else { i += 1; }
        }

        // check / mate
        p = tok_at(tokens, i);
        if (p && p->type == TK_CHECK) { out->is_check = 1; i++; p = tok_at(tokens, i); }
        if (p && p->type == TK_MATE) { out->is_mate = 1; i++; }

        // asegurar que no queden tokens inesperados
        if (ensure_no_extra_tokens(tokens, i) != 0) return -1;

        return 0;
    }

    // no debería llegar aquí
    return -1;
}
