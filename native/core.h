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
} SfGame;

void sf_game_reset(SfGame *g);
int sf_square_to_index(const char square[2]);
void sf_index_to_square(int index, char out[3]);
int sf_apply_uci(SfGame *g, const char *move);
int sf_parse_bestmove(const char *line, char out[6]);
int sf_parse_perft_move(const char *line, char out[6]);
int sf_pick_legal_move(const char **legal, size_t count, const char from[2], const char to[2], char out[6]);
int sf_is_white_piece(char p);
int sf_is_black_piece(char p);

#endif
