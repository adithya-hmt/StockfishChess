#ifndef FRAMILTON_APP_MODEL_H
#define FRAMILTON_APP_MODEL_H

#ifdef SF_FREESTANDING
typedef __SIZE_TYPE__ size_t;
typedef unsigned int uint32_t;
#else
#include <stddef.h>
#include <stdint.h>
#endif

#define CER_DATA_MAGIC 0x43524333u
#define CER_DATA_VERSION 3u
#define CER_MAX_PROFILES 4
#define CER_MAX_HISTORY 24
#define CER_PROFILE_NAME 24
#define CER_GAME_HISTORY 4096

typedef enum {
    CER_RESULT_NONE = 0,
    CER_RESULT_WIN = 1,
    CER_RESULT_DRAW = 2,
    CER_RESULT_LOSS = 3
} CerResult;

typedef struct {
    char name[CER_PROFILE_NAME];
    int avatar;
    int accent;
    int rating;
    int wins;
    int draws;
    int losses;
    int puzzle_solved;
    int puzzle_streak;
    int best_streak;
    int app_lock;
    uint32_t pin_hash;
} CerProfile;

typedef struct {
    int haptics;
    int legal_hints;
    int coordinates;
    int auto_flip;
    int board_theme;
    int piece_theme;
    int engine_level;
    int time_control;
    int sound;
} CerSettings;

typedef struct {
    char moves[1024];
    int result;
    int opponent_level;
    int mode;
    int human_side;
    int move_count;
    int game_number;
} CerHistoryEntry;

typedef struct {
    int active;
    int mode;
    int human_side;
    int level;
    int time_control;
    int hints;
    int white_ms;
    int black_ms;
    char moves[CER_GAME_HISTORY];
} CerActiveGame;

typedef struct {
    uint32_t magic;
    uint32_t version;
    int onboarding_complete;
    int profile_count;
    int active_profile;
    CerProfile profiles[CER_MAX_PROFILES];
    CerSettings settings;
    CerHistoryEntry history[CER_MAX_HISTORY];
    int history_count;
    int games_played;
    CerActiveGame active_game;
    uint32_t checksum;
} CerData;

void cer_data_defaults(CerData *data);
int cer_data_validate(const CerData *data);
uint32_t cer_data_checksum(const CerData *data);
uint32_t cer_pin_hash(const char *digits);
int cer_data_load(CerData *data, const char *path);
int cer_data_save(CerData *data, const char *path);
int cer_add_profile(CerData *data, const char *name, int avatar, int accent);
void cer_record_game(CerData *data, const char *moves, int result, int level, int mode, int human_side);
void cer_apply_result(CerData *data, int result);
const char *cer_preset_name(int index);

#endif
