#include "app_model.h"

#ifdef SF_FREESTANDING
void *memset(void *, int, size_t);
void *memcpy(void *, const void *, size_t);
size_t strlen(const char *);
int open(const char *, int, ...);
long read(int, void *, size_t);
long write(int, const void *, size_t);
int close(int);
int rename(const char *, const char *);
int unlink(const char *);
#define O_RDONLY 0
#define O_WRONLY 1
#define O_CREAT 64
#define O_TRUNC 512
#else
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#endif

static void copy_text(char *dst, size_t cap, const char *src) {
    size_t i = 0;
    if (!dst || cap == 0) return;
    if (src) while (src[i] && i + 1 < cap) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

const char *cer_preset_name(int index) {
    static const char *names[] = {"White Knight", "Tactician", "Endgame Fox", "Board Wizard", "Quiet Bishop", "Local Hero"};
    if (index < 0) index = 0;
    return names[index % 6];
}

uint32_t cer_pin_hash(const char *digits) {
    uint32_t h = 2166136261u;
    size_t i = 0;
    if (!digits) return 0;
    while (digits[i]) { h ^= (unsigned char)digits[i++]; h *= 16777619u; }
    h ^= 0xC3E1E171u;
    h *= 16777619u;
    return h;
}

void cer_data_defaults(CerData *data) {
    if (!data) return;
    memset(data, 0, sizeof(*data));
    data->magic = CER_DATA_MAGIC;
    data->version = CER_DATA_VERSION;
    data->active_profile = 0;
    data->settings.haptics = 1;
    data->settings.legal_hints = 1;
    data->settings.coordinates = 1;
    data->settings.auto_flip = 0;
    data->settings.board_theme = 0;
    data->settings.piece_theme = 0;
    data->settings.engine_level = 2;
    data->settings.time_control = 1;
    data->settings.sound = 0;
    cer_add_profile(data, "White Knight", 0, 0);
    data->checksum = cer_data_checksum(data);
}

int cer_data_validate(const CerData *data) {
    if (!data || data->magic != CER_DATA_MAGIC || data->version != CER_DATA_VERSION) return 0;
    if (data->profile_count < 1 || data->profile_count > CER_MAX_PROFILES) return 0;
    if (data->active_profile < 0 || data->active_profile >= data->profile_count) return 0;
    if (data->history_count < 0 || data->history_count > CER_MAX_HISTORY) return 0;
    if (data->settings.engine_level < 0 || data->settings.engine_level > 4) return 0;
    if (data->settings.time_control < 0 || data->settings.time_control > 3) return 0;
    return data->checksum == cer_data_checksum(data);
}

uint32_t cer_data_checksum(const CerData *data) {
    const unsigned char *p = (const unsigned char *)data;
    const size_t n = sizeof(*data) - sizeof(data->checksum);
    uint32_t h = 2166136261u;
    size_t i;
    if (!data) return 0;
    for (i = 0; i < n; ++i) { h ^= p[i]; h *= 16777619u; }
    return h;
}

static int read_exact(int fd, void *buffer, size_t size) {
    unsigned char *p = (unsigned char *)buffer;
    size_t got = 0;
    while (got < size) {
        long n = read(fd, p + got, size - got);
        if (n <= 0) return 0;
        got += (size_t)n;
    }
    return 1;
}

static int write_exact(int fd, const void *buffer, size_t size) {
    const unsigned char *p = (const unsigned char *)buffer;
    size_t sent = 0;
    while (sent < size) {
        long n = write(fd, p + sent, size - sent);
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

int cer_data_load(CerData *data, const char *path) {
    int fd;
    CerData temp;
    if (!data || !path) return 0;
    fd = open(path, O_RDONLY);
    if (fd < 0) { cer_data_defaults(data); return 0; }
    if (!read_exact(fd, &temp, sizeof(temp))) { close(fd); cer_data_defaults(data); return 0; }
    close(fd);
    if (!cer_data_validate(&temp)) { cer_data_defaults(data); return 0; }
    memcpy(data, &temp, sizeof(temp));
    return 1;
}

int cer_data_save(CerData *data, const char *path) {
    char temp_path[1024];
    size_t n;
    int fd, ok;
    if (!data || !path) return 0;
    data->magic = CER_DATA_MAGIC;
    data->version = CER_DATA_VERSION;
    data->checksum = cer_data_checksum(data);
    n = strlen(path);
    if (n + 5 >= sizeof(temp_path)) return 0;
    memcpy(temp_path, path, n);
    memcpy(temp_path + n, ".tmp", 5);
    fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return 0;
    ok = write_exact(fd, data, sizeof(*data));
    close(fd);
    if (!ok) { unlink(temp_path); return 0; }
    unlink(path);
    if (rename(temp_path, path) != 0) { unlink(temp_path); return 0; }
    return 1;
}

int cer_add_profile(CerData *data, const char *name, int avatar, int accent) {
    CerProfile *p;
    if (!data || data->profile_count >= CER_MAX_PROFILES) return -1;
    p = &data->profiles[data->profile_count];
    memset(p, 0, sizeof(*p));
    copy_text(p->name, sizeof(p->name), name ? name : cer_preset_name(data->profile_count));
    p->avatar = avatar < 0 ? 0 : avatar % 6;
    p->accent = accent < 0 ? 0 : accent % 4;
    p->rating = 800;
    ++data->profile_count;
    return data->profile_count - 1;
}

void cer_apply_result(CerData *data, int result) {
    CerProfile *p;
    if (!data || data->active_profile < 0 || data->active_profile >= data->profile_count) return;
    p = &data->profiles[data->active_profile];
    if (result == CER_RESULT_WIN) { ++p->wins; p->rating += 12; }
    else if (result == CER_RESULT_LOSS) { ++p->losses; p->rating -= p->rating > 200 ? 8 : 0; }
    else if (result == CER_RESULT_DRAW) { ++p->draws; p->rating += 1; }
}

void cer_record_game(CerData *data, const char *moves, int result, int level, int mode, int human_side) {
    CerHistoryEntry *entry;
    int i;
    if (!data) return;
    if (data->history_count < CER_MAX_HISTORY) ++data->history_count;
    for (i = data->history_count - 1; i > 0; --i) data->history[i] = data->history[i - 1];
    entry = &data->history[0];
    memset(entry, 0, sizeof(*entry));
    copy_text(entry->moves, sizeof(entry->moves), moves ? moves : "");
    entry->result = result;
    entry->opponent_level = level;
    entry->mode = mode;
    entry->human_side = human_side;
    entry->game_number = ++data->games_played;
    if (moves) {
        int count = 0, in_move = 0;
        size_t k;
        for (k = 0; moves[k]; ++k) {
            if (moves[k] == ' ') in_move = 0;
            else if (!in_move) { in_move = 1; ++count; }
        }
        entry->move_count = count;
    }
    cer_apply_result(data, result);
    memset(&data->active_game, 0, sizeof(data->active_game));
}
