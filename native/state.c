#include "state.h"

static size_t cer_len(const char *s) { return s ? strlen(s) : 0; }

void cer_copy_text(char *dst, size_t capacity, const char *src) {
    size_t i = 0;
    if (!dst || capacity == 0) return;
    if (src) while (src[i] && i + 1 < capacity) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static int cer_path(char *out, size_t cap, const char *dir, const char *name) {
    size_t a = cer_len(dir), b = cer_len(name);
    if (!out || !dir || !name || a + b + 2 > cap) return 0;
    memcpy(out, dir, a);
    if (a && dir[a - 1] != '/') out[a++] = '/';
    memcpy(out + a, name, b + 1);
    return 1;
}

static int cer_write_bytes(int fd, const void *data, size_t length) {
    const char *p = (const char *)data;
    while (length) {
        ssize_t n = write(fd, p, length);
        if (n <= 0) return 0;
        p += n;
        length -= (size_t)n;
    }
    return 1;
}

void cer_state_defaults(CerPersisted *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->magic = CER_STATE_MAGIC;
    state->version = CER_STATE_VERSION;
    state->active_profile = -1;
    state->haptics_enabled = 1;
    state->coordinates_enabled = 1;
    state->legal_moves_enabled = 1;
    state->sound_enabled = 0;
    state->auto_queen = 0;
    state->board_theme = 0;
    state->piece_theme = 0;
    state->default_level = 1;
    state->default_side = 2;
    state->default_time = 0;
    state->next_sequence = 1;
}

int cer_state_load(const char *directory, CerPersisted *state) {
    char path[1024];
    int fd;
    size_t total = 0;
    if (!state) return 0;
    cer_state_defaults(state);
    if (!cer_path(path, sizeof(path), directory, "framilton-v3.state")) return 0;
    fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    while (total < sizeof(*state)) {
        ssize_t n = read(fd, (char *)state + total, sizeof(*state) - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    close(fd);
    if (total != sizeof(*state) || state->magic != CER_STATE_MAGIC || state->version != CER_STATE_VERSION) {
        cer_state_defaults(state);
        return 0;
    }
    if (state->profile_count < 0 || state->profile_count > CER_MAX_PROFILES ||
        state->history_count < 0 || state->history_count > CER_MAX_HISTORY ||
        state->active_profile < -1 || state->active_profile >= state->profile_count) {
        cer_state_defaults(state);
        return 0;
    }
    return 1;
}

int cer_state_save(const char *directory, const CerPersisted *state) {
    char path[1024], temp[1024];
    int fd;
    if (!directory || !state) return 0;
    if (!cer_path(path, sizeof(path), directory, "framilton-v3.state") ||
        !cer_path(temp, sizeof(temp), directory, "framilton-v3.state.tmp")) return 0;
    fd = open(temp, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return 0;
    if (!cer_write_bytes(fd, state, sizeof(*state))) { close(fd); unlink(temp); return 0; }
    close(fd);
    if (rename(temp, path) != 0) { unlink(temp); return 0; }
    return 1;
}

uint32_t cer_pin_hash(const char *pin) {
    static const char salt[] = "FRAMILTON-LOCAL-V3";
    uint32_t h = 2166136261u;
    size_t i;
    for (i = 0; i < sizeof(salt) - 1; ++i) { h ^= (unsigned char)salt[i]; h *= 16777619u; }
    if (pin) for (i = 0; pin[i]; ++i) { h ^= (unsigned char)pin[i]; h *= 16777619u; }
    return h;
}

int cer_profile_add(CerPersisted *state, const char *name, int avatar, int accent) {
    CerProfile *p;
    int index;
    if (!state || state->profile_count >= CER_MAX_PROFILES) return -1;
    index = state->profile_count++;
    p = &state->profiles[index];
    memset(p, 0, sizeof(*p));
    cer_copy_text(p->name, sizeof(p->name), name && name[0] ? name : "PLAYER");
    p->avatar = avatar;
    p->accent = accent;
    p->rating = 1200;
    state->active_profile = index;
    return index;
}

CerProfile *cer_active_profile(CerPersisted *state) {
    if (!state || state->active_profile < 0 || state->active_profile >= state->profile_count) return 0;
    return &state->profiles[state->active_profile];
}

const CerProfile *cer_active_profile_const(const CerPersisted *state) {
    if (!state || state->active_profile < 0 || state->active_profile >= state->profile_count) return 0;
    return &state->profiles[state->active_profile];
}

void cer_history_add(CerPersisted *state, const CerHistory *entry) {
    CerHistory copy;
    if (!state || !entry) return;
    copy = *entry;
    copy.sequence = state->next_sequence++;
    if (state->history_count < CER_MAX_HISTORY) ++state->history_count;
    memmove(&state->history[1], &state->history[0], (size_t)(state->history_count - 1) * sizeof(CerHistory));
    state->history[0] = copy;
}

void cer_profile_record_result(CerProfile *profile, int result, int level) {
    int delta;
    if (!profile) return;
    ++profile->games;
    delta = level <= 0 ? 8 : (level == 1 ? 14 : (level == 2 ? 20 : 26));
    if (result == 1) { ++profile->wins; profile->rating += delta; }
    else if (result == 2) { ++profile->draws; profile->rating += delta / 5; }
    else { ++profile->losses; profile->rating -= delta / 2; }
    if (profile->rating < 400) profile->rating = 400;
    if (profile->rating > 3200) profile->rating = 3200;
}
