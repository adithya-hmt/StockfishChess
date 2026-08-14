#include "core.h"

#ifdef SF_FREESTANDING
void *memset(void *, int, size_t);
void *memcpy(void *, const void *, size_t);
size_t strlen(const char *);
int strncmp(const char *, const char *, size_t);
char *strchr(const char *, int);
#else
#include <string.h>
#endif

static int is_file(char c) { return c >= 'a' && c <= 'h'; }
static int is_rank(char c) { return c >= '1' && c <= '8'; }
static int is_move_text(const char *s, size_t n) {
    if (n != 4 && n != 5) return 0;
    if (!is_file(s[0]) || !is_rank(s[1]) || !is_file(s[2]) || !is_rank(s[3])) return 0;
    if (n == 5 && strchr("qrbn", s[4]) == 0) return 0;
    return 1;
}

int sf_square_to_index(const char square[2]) {
    if (!square || !is_file(square[0]) || !is_rank(square[1])) return -1;
    return (square[1] - '1') * 8 + (square[0] - 'a');
}

void sf_index_to_square(int index, char out[3]) {
    if (!out) return;
    if (index < 0 || index >= 64) {
        out[0] = out[1] = '?'; out[2] = 0; return;
    }
    out[0] = (char)('a' + index % 8);
    out[1] = (char)('1' + index / 8);
    out[2] = 0;
}

int sf_is_white_piece(char p) { return p >= 'A' && p <= 'Z'; }
int sf_is_black_piece(char p) { return p >= 'a' && p <= 'z'; }

void sf_game_reset(SfGame *g) {
    static const char rank1[] = "RNBQKBNR";
    static const char rank8[] = "rnbqkbnr";
    int i;
    if (!g) return;
    memset(g, 0, sizeof(*g));
    for (i = 0; i < 8; ++i) {
        g->board[i] = rank1[i];
        g->board[8 + i] = 'P';
        g->board[48 + i] = 'p';
        g->board[56 + i] = rank8[i];
    }
}

static char promoted(char pawn, char promo) {
    char q = promo;
    if (pawn == 'P' && q >= 'a' && q <= 'z') q = (char)(q - 'a' + 'A');
    return q;
}

int sf_apply_uci(SfGame *g, const char *move) {
    size_t n;
    int from, to;
    char piece, target;
    int ff, tf;
    if (!g || !move) return 0;
    n = strlen(move);
    if (!is_move_text(move, n)) return 0;
    from = sf_square_to_index(move);
    to = sf_square_to_index(move + 2);
    if (from < 0 || to < 0) return 0;
    piece = g->board[from];
    if (!piece) return 0;
    target = g->board[to];
    ff = from % 8;
    tf = to % 8;

    if ((piece == 'P' || piece == 'p') && ff != tf && target == 0) {
        int captured = piece == 'P' ? to - 8 : to + 8;
        if (captured >= 0 && captured < 64) g->board[captured] = 0;
    }

    if ((piece == 'K' || piece == 'k') && (tf - ff == 2 || ff - tf == 2)) {
        if (tf == 6) {
            int rook_from = (from / 8) * 8 + 7;
            int rook_to = (from / 8) * 8 + 5;
            g->board[rook_to] = g->board[rook_from];
            g->board[rook_from] = 0;
        } else if (tf == 2) {
            int rook_from = (from / 8) * 8;
            int rook_to = (from / 8) * 8 + 3;
            g->board[rook_to] = g->board[rook_from];
            g->board[rook_from] = 0;
        }
    }

    g->board[from] = 0;
    g->board[to] = n == 5 ? promoted(piece, move[4]) : piece;

    if (g->history_len) {
        if (g->history_len + 1 >= sizeof(g->history)) return 0;
        g->history[g->history_len++] = ' ';
    }
    if (g->history_len + n >= sizeof(g->history)) return 0;
    memcpy(g->history + g->history_len, move, n);
    g->history_len += n;
    g->history[g->history_len] = 0;
    return 1;
}

int sf_parse_bestmove(const char *line, char out[6]) {
    const char *p;
    size_t n = 0;
    if (!line || !out || strncmp(line, "bestmove ", 9) != 0) return 0;
    p = line + 9;
    while (p[n] && p[n] != ' ' && p[n] != '\r' && p[n] != '\n' && n < 5) ++n;
    if (!is_move_text(p, n)) return 0;
    memcpy(out, p, n);
    out[n] = 0;
    return 1;
}

int sf_parse_perft_move(const char *line, char out[6]) {
    const char *colon;
    size_t n;
    if (!line || !out) return 0;
    colon = strchr(line, ':');
    if (!colon) return 0;
    n = (size_t)(colon - line);
    if (!is_move_text(line, n)) return 0;
    memcpy(out, line, n);
    out[n] = 0;
    return 1;
}

int sf_pick_legal_move(const char **legal, size_t count, const char from[2], const char to[2], char out[6]) {
    size_t i;
    if (!legal || !from || !to || !out) return 0;
    for (i = 0; i < count; ++i) {
        const char *m = legal[i];
        size_t n = m ? strlen(m) : 0;
        if ((n == 4 || n == 5) && m[0] == from[0] && m[1] == from[1] && m[2] == to[0] && m[3] == to[1]) {
            if (n == 5 && m[4] != 'q') continue;
            memcpy(out, m, n);
            out[n] = 0;
            return 1;
        }
    }
    return 0;
}
