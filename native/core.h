#ifndef SF_CORE_H
#define SF_CORE_H

#ifdef SF_FREESTANDING
typedef __SIZE_TYPE__ size_t;
#else
#include <stddef.h>
#endif

typedef struct {
    char board[64];
    char history[4096];
    size_t history_len;
    char start_fen[128];
    int start_side_white;
} SfGame;

void sf_game_reset(SfGame *g);
int sf_game_load_fen(SfGame *g, const char *fen);
int sf_side_to_move(const SfGame *g);
int sf_square_to_index(const char square[2]);
void sf_index_to_square(int index, char out[3]);
int sf_apply_uci(SfGame *g, const char *move);
int sf_parse_bestmove(const char *line, char out[6]);
int sf_parse_perft_move(const char *line, char out[6]);
int sf_pick_legal_move(const char **legal, size_t count, const char from[2], const char to[2], char out[6]);
int sf_is_white_piece(char p);
int sf_is_black_piece(char p);
int sf_undo_plies(SfGame *g, int plies);

int sf_history_move_count(const SfGame *g);
int sf_history_get_move(const SfGame *g, int index, char out[6]);
int sf_game_from_history(SfGame *g, const char *history, int plies);
int sf_game_from_fen_history(SfGame *g, const char *fen, const char *history, int plies);
int sf_find_king(const SfGame *g, int white);
int sf_is_insufficient_material(const SfGame *g);
int sf_pick_legal_move_promotion(const char **legal, size_t count, const char from[2], const char to[2], char promotion, char out[6]);
int sf_is_square_attacked(const SfGame *g, int square, int by_white);
int sf_is_in_check(const SfGame *g, int white);
int sf_material_balance(const SfGame *g);
unsigned long long sf_board_hash(const SfGame *g);
#endif
