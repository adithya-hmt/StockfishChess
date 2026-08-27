#ifndef FRAMILTON_STATE_H
#define FRAMILTON_STATE_H
#include "platform.h"

#define CER_STATE_MAGIC 0x43524333u
#define CER_STATE_VERSION 4u
#define CER_MAX_PROFILES 4
#define CER_MAX_HISTORY 16
#define CER_NAME_CAP 20
#define CER_HISTORY_MOVES 2048
#define CER_ACTIVE_MOVES 4096
#define CER_FEN_CAP 128

typedef struct {
    char name[CER_NAME_CAP];
    int avatar;
    int accent;
    int rating;
    int games;
    int wins;
    int draws;
    int losses;
    int puzzles_solved;
    int puzzle_streak;
    int best_puzzle_streak;
} CerProfile;

typedef struct {
    char moves[CER_HISTORY_MOVES];
    char start_fen[CER_FEN_CAP];
    int profile_index;
    int result;
    int result_reason;
    int mode;
    int level;
    int human_side;
    int move_count;
    int sequence;
    int64_t white_ms;
    int64_t black_ms;
} CerHistory;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    int onboarding_done;
    int profile_count;
    int active_profile;
    int pin_enabled;
    uint32_t pin_hash;
    int haptics_enabled;
    int coordinates_enabled;
    int legal_moves_enabled;
    int auto_queen;
    int board_theme;
    int piece_theme;
    int reduce_motion;
    int sound_enabled;
    int default_level;
    int default_side;
    int default_time;
    int history_count;
    int next_sequence;
    int puzzle_solved_mask;
    CerProfile profiles[CER_MAX_PROFILES];
    CerHistory history[CER_MAX_HISTORY];
    int active_game;
    int active_mode;
    int active_level;
    int active_human_side;
    int active_time_control;
    int active_board_flipped;
    int64_t active_white_ms;
    int64_t active_black_ms;
    char active_fen[CER_FEN_CAP];
    char active_moves[CER_ACTIVE_MOVES];
} CerPersisted;

void cer_state_defaults(CerPersisted *state);
int cer_state_load(const char *directory, CerPersisted *state);
int cer_state_save(const char *directory, const CerPersisted *state);
uint32_t cer_pin_hash(const char *pin);
int cer_profile_add(CerPersisted *state, const char *name, int avatar, int accent);
CerProfile *cer_active_profile(CerPersisted *state);
const CerProfile *cer_active_profile_const(const CerPersisted *state);
void cer_history_add(CerPersisted *state, const CerHistory *entry);
void cer_copy_text(char *dst, size_t capacity, const char *src);
void cer_profile_record_result(CerProfile *profile, int result, int level);

#endif
