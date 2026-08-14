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
    if (index < 0 || index >= 64) { out[0] = out[1] = '?'; out[2] = 0; return; }
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
    g->start_side_white = 1;
    for (i = 0; i < 8; ++i) {
        g->board[i] = rank1[i];
        g->board[8 + i] = 'P';
        g->board[48 + i] = 'p';
        g->board[56 + i] = rank8[i];
    }
}

int sf_game_load_fen(SfGame *g, const char *fen) {
    int rank = 7, file = 0;
    size_t i = 0, n = 0;
    if (!g || !fen) return 0;
    memset(g, 0, sizeof(*g));
    while (fen[i] && fen[i] != ' ') {
        char c = fen[i++];
        if (c == '/') {
            if (file != 8 || rank <= 0) return 0;
            --rank; file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) return 0;
        } else if (strchr("prnbqkPRNBQK", c)) {
            if (file >= 8 || rank < 0) return 0;
            g->board[rank * 8 + file++] = c;
        } else return 0;
    }
    if (rank != 0 || file != 8 || fen[i] != ' ') return 0;
    ++i;
    if (fen[i] != 'w' && fen[i] != 'b') return 0;
    g->start_side_white = fen[i] == 'w';
    while (fen[n] && n + 1 < sizeof(g->start_fen)) { g->start_fen[n] = fen[n]; ++n; }
    if (fen[n]) return 0;
    g->start_fen[n] = 0;
    return 1;
}

int sf_history_move_count(const SfGame *g) {
    int count = 0, in_move = 0;
    size_t i;
    if (!g) return 0;
    for (i = 0; i < g->history_len; ++i) {
        if (g->history[i] == ' ') in_move = 0;
        else if (!in_move) { in_move = 1; ++count; }
    }
    return count;
}

int sf_side_to_move(const SfGame *g) {
    int moves;
    if (!g) return 1;
    moves = sf_history_move_count(g);
    return (moves & 1) ? !g->start_side_white : g->start_side_white;
}

static char promoted(char pawn, char promo) {
    if (pawn == 'P' && promo >= 'a' && promo <= 'z') return (char)(promo - 'a' + 'A');
    return promo;
}

int sf_apply_uci(SfGame *g, const char *move) {
    size_t n;
    int from, to, ff, tf;
    char piece, target;
    if (!g || !move) return 0;
    n = strlen(move);
    if (!is_move_text(move, n)) return 0;
    from = sf_square_to_index(move);
    to = sf_square_to_index(move + 2);
    if (from < 0 || to < 0) return 0;
    piece = g->board[from];
    if (!piece) return 0;
    target = g->board[to];
    ff = from % 8; tf = to % 8;

    if ((piece == 'P' || piece == 'p') && ff != tf && target == 0) {
        int captured = piece == 'P' ? to - 8 : to + 8;
        if (captured >= 0 && captured < 64) g->board[captured] = 0;
    }
    if ((piece == 'K' || piece == 'k') && (tf - ff == 2 || ff - tf == 2)) {
        if (tf == 6) {
            int rook_from = (from / 8) * 8 + 7, rook_to = (from / 8) * 8 + 5;
            g->board[rook_to] = g->board[rook_from]; g->board[rook_from] = 0;
        } else if (tf == 2) {
            int rook_from = (from / 8) * 8, rook_to = (from / 8) * 8 + 3;
            g->board[rook_to] = g->board[rook_from]; g->board[rook_from] = 0;
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
    const char *p; size_t n = 0;
    if (!line || !out || strncmp(line, "bestmove ", 9) != 0) return 0;
    p = line + 9;
    while (p[n] && p[n] != ' ' && p[n] != '\r' && p[n] != '\n' && n < 5) ++n;
    if (!is_move_text(p, n)) return 0;
    memcpy(out, p, n); out[n] = 0; return 1;
}

int sf_parse_perft_move(const char *line, char out[6]) {
    const char *colon; size_t n;
    if (!line || !out) return 0;
    colon = strchr(line, ':'); if (!colon) return 0;
    n = (size_t)(colon - line); if (!is_move_text(line, n)) return 0;
    memcpy(out, line, n); out[n] = 0; return 1;
}

int sf_pick_legal_move_promotion(const char **legal, size_t count, const char from[2], const char to[2], char promotion, char out[6]) {
    size_t i;
    if (!legal || !from || !to || !out) return 0;
    for (i = 0; i < count; ++i) {
        const char *m = legal[i]; size_t n = m ? strlen(m) : 0;
        if ((n == 4 || n == 5) && m[0] == from[0] && m[1] == from[1] && m[2] == to[0] && m[3] == to[1]) {
            if (n == 5 && promotion && m[4] != promotion) continue;
            if (n == 5 && !promotion && m[4] != 'q') continue;
            memcpy(out, m, n); out[n] = 0; return 1;
        }
    }
    return 0;
}

int sf_pick_legal_move(const char **legal, size_t count, const char from[2], const char to[2], char out[6]) {
    return sf_pick_legal_move_promotion(legal, count, from, to, 0, out);
}

int sf_history_get_move(const SfGame *g, int index, char out[6]) {
    int current = 0; size_t pos = 0;
    if (!g || !out || index < 0) return 0;
    while (pos < g->history_len) {
        size_t start, n;
        while (pos < g->history_len && g->history[pos] == ' ') ++pos;
        if (pos >= g->history_len) break;
        start = pos; while (pos < g->history_len && g->history[pos] != ' ') ++pos; n = pos - start;
        if (current++ == index) { if (n != 4 && n != 5) return 0; memcpy(out, g->history + start, n); out[n] = 0; return 1; }
    }
    return 0;
}

static int reset_to_base(SfGame *g, const char *fen) { return fen && fen[0] ? sf_game_load_fen(g, fen) : (sf_game_reset(g), 1); }

int sf_game_from_history(SfGame *g, const char *history, int plies) {
    return sf_game_from_fen_history(g, 0, history, plies);
}

int sf_game_from_fen_history(SfGame *g, const char *fen, const char *history, int plies) {
    size_t pos = 0, len; int applied = 0;
    if (!g || !history || !reset_to_base(g, fen)) return 0;
    len = strlen(history);
    while (pos < len && (plies < 0 || applied < plies)) {
        size_t start, n; char move[6];
        while (pos < len && history[pos] == ' ') ++pos;
        if (pos >= len) break;
        start = pos; while (pos < len && history[pos] != ' ') ++pos; n = pos - start;
        if (n != 4 && n != 5) return 0;
        memcpy(move, history + start, n); move[n] = 0;
        if (!sf_apply_uci(g, move)) return 0;
        ++applied;
    }
    return 1;
}

int sf_undo_plies(SfGame *g, int plies) {
    char saved[sizeof(g->history)], fen[sizeof(g->start_fen)];
    size_t len; int removed = 0;
    if (!g || plies <= 0 || !g->history_len) return 0;
    len = g->history_len; memcpy(saved, g->history, len + 1); memcpy(fen, g->start_fen, sizeof(fen));
    while (removed < plies && len > 0) {
        while (len > 0 && saved[len - 1] == ' ') --len;
        while (len > 0 && saved[len - 1] != ' ') --len;
        if (len > 0) --len;
        ++removed;
    }
    saved[len] = 0;
    if (!sf_game_from_fen_history(g, fen[0] ? fen : 0, saved, -1)) return 0;
    return removed;
}

int sf_find_king(const SfGame *g, int white) {
    int i; char king = white ? 'K' : 'k';
    if (!g) return -1;
    for (i = 0; i < 64; ++i) if (g->board[i] == king) return i;
    return -1;
}

int sf_is_insufficient_material(const SfGame *g) {
    int i, minor = 0;
    if (!g) return 0;
    for (i = 0; i < 64; ++i) {
        char p = g->board[i];
        if (p == 'P' || p == 'p' || p == 'R' || p == 'r' || p == 'Q' || p == 'q') return 0;
        if (p == 'B' || p == 'b' || p == 'N' || p == 'n') ++minor;
    }
    return minor <= 1;
}

static int on_board(int file, int rank) { return file >= 0 && file < 8 && rank >= 0 && rank < 8; }

int sf_is_square_attacked(const SfGame *g, int square, int by_white) {
    static const int ndf[8] = {1,2,2,1,-1,-2,-2,-1};
    static const int ndr[8] = {2,1,-1,-2,-2,-1,1,2};
    static const int ddf[4] = {1,1,-1,-1};
    static const int ddr[4] = {1,-1,1,-1};
    static const int rdf[4] = {1,-1,0,0};
    static const int rdr[4] = {0,0,1,-1};
    int f, r, i;
    char pawn=by_white?'P':'p', knight=by_white?'N':'n', bishop=by_white?'B':'b';
    char rook=by_white?'R':'r', queen=by_white?'Q':'q', king=by_white?'K':'k';
    if (!g || square < 0 || square >= 64) return 0;
    f = square % 8; r = square / 8;
    {
        int sr = r + (by_white ? -1 : 1);
        if (on_board(f-1,sr) && g->board[sr*8+f-1] == pawn) return 1;
        if (on_board(f+1,sr) && g->board[sr*8+f+1] == pawn) return 1;
    }
    for (i=0;i<8;++i) { int sf=f+ndf[i], sr=r+ndr[i]; if(on_board(sf,sr)&&g->board[sr*8+sf]==knight)return 1; }
    for (i=-1;i<=1;++i) { int j; for(j=-1;j<=1;++j) if(i||j) if(on_board(f+i,r+j)&&g->board[(r+j)*8+f+i]==king)return 1; }
    for(i=0;i<4;++i){int sf=f+ddf[i],sr=r+ddr[i];while(on_board(sf,sr)){char p=g->board[sr*8+sf];if(p){if(p==bishop||p==queen)return 1;break;}sf+=ddf[i];sr+=ddr[i];}}
    for(i=0;i<4;++i){int sf=f+rdf[i],sr=r+rdr[i];while(on_board(sf,sr)){char p=g->board[sr*8+sf];if(p){if(p==rook||p==queen)return 1;break;}sf+=rdf[i];sr+=rdr[i];}}
    return 0;
}

int sf_is_in_check(const SfGame *g, int white) { int king = sf_find_king(g, white); return king >= 0 ? sf_is_square_attacked(g, king, !white) : 0; }

int sf_material_balance(const SfGame *g) {
    int i, score = 0;
    if (!g) return 0;
    for(i=0;i<64;++i){char p=g->board[i];int value=0;char u=sf_is_white_piece(p)?p:(sf_is_black_piece(p)?(char)(p-'a'+'A'):0);switch(u){case'P':value=100;break;case'N':value=320;break;case'B':value=330;break;case'R':value=500;break;case'Q':value=900;break;default:break;}if(sf_is_white_piece(p))score+=value;else if(sf_is_black_piece(p))score-=value;}
    return score;
}

unsigned long long sf_board_hash(const SfGame *g) {
    unsigned long long h=1469598103934665603ULL;int i;if(!g)return 0;for(i=0;i<64;++i){h^=(unsigned char)g->board[i];h*=1099511628211ULL;}h^=(unsigned long long)sf_side_to_move(g);h*=1099511628211ULL;return h;
}
