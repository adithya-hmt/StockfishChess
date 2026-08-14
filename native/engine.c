#include "engine.h"

#define CER_MAX_LINE 2048

static size_t e_len(const char *s) { return s ? strlen(s) : 0; }
static int e_eq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }
static void e_copy(char *dst, size_t cap, const char *src) {
    size_t i = 0;
    if (!dst || !cap) return;
    if (src) while (src[i] && i + 1 < cap) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}
static int e_write_all(int fd, const char *s) {
    size_t left = e_len(s);
    while (left) {
        ssize_t n = write(fd, s, left);
        if (n <= 0) return 0;
        s += n;
        left -= (size_t)n;
    }
    return 1;
}
static int e_parse_int(const char *s, int *value) {
    int sign = 1, n = 0, any = 0;
    if (!s || !value) return 0;
    if (*s == '-') { sign = -1; ++s; }
    else if (*s == '+') ++s;
    while (*s >= '0' && *s <= '9') { any = 1; n = n * 10 + (*s - '0'); ++s; }
    if (!any) return 0;
    *value = n * sign;
    return 1;
}
static char *e_find_token(const char *line, const char *token) {
    char *p = strstr(line, token);
    return p ? p + strlen(token) : 0;
}
static int e_append_int(char *out, size_t cap, size_t *at, int value) {
    char tmp[16];
    int n = 0, i;
    unsigned v;
    if (!out || !at || *at >= cap) return 0;
    if (value < 0) { if (*at + 1 >= cap) return 0; out[(*at)++] = '-'; v = (unsigned)(-value); }
    else v = (unsigned)value;
    do { tmp[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < (int)sizeof(tmp));
    for (i = n - 1; i >= 0; --i) { if (*at + 1 >= cap) return 0; out[(*at)++] = tmp[i]; }
    out[*at] = 0;
    return 1;
}
static int e_command_int(CerEngine *e, const char *prefix, int value) {
    char cmd[64];
    size_t at = 0, i;
    for (i = 0; prefix[i] && at + 1 < sizeof(cmd); ++i) cmd[at++] = prefix[i];
    cmd[at] = 0;
    if (!e_append_int(cmd, sizeof(cmd), &at, value) || at + 2 >= sizeof(cmd)) return 0;
    cmd[at++] = '\n'; cmd[at] = 0;
    return e_write_all(e->in_fd, cmd);
}

static int e_find_engine_path(char *out, size_t cap) {
    const size_t map_cap = 1024u * 1024u;
    char *buf = (char *)malloc(map_cap);
    size_t total = 0;
    int fd;
    char *hit, *line, *slash, *p;
    const char *suffix = "/libstockfish.so";
    size_t dir_len, suffix_len = strlen(suffix);
    int ok = 0;
    if (!buf) return 0;
    fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) { free(buf); return 0; }
    while (total + 1 < map_cap) {
        ssize_t n = read(fd, buf + total, map_cap - total - 1);
        if (n <= 0) break;
        total += (size_t)n;
    }
    close(fd);
    buf[total] = 0;
    hit = strstr(buf, "libsf_chess.so");
    if (!hit) goto done;
    line = hit;
    while (line > buf && line[-1] != '\n') --line;
    slash = 0;
    for (p = line; p < hit; ++p) if (*p == '/') { slash = p; break; }
    if (!slash) goto done;
    p = hit;
    while (p > slash && p[-1] != '/') --p;
    dir_len = (size_t)(p - slash - 1);
    if (dir_len + suffix_len + 1 > cap) goto done;
    memcpy(out, slash, dir_len);
    memcpy(out + dir_len, suffix, suffix_len + 1);
    ok = 1;
done:
    free(buf);
    return ok;
}

static int e_readline(CerEngine *e, char *out, size_t cap, int timeout_ms) {
    size_t n = 0;
    if (!e || e->out_fd < 0 || !out || cap < 2) return 0;
    for (;;) {
        struct pollfd pfd;
        char ch;
        ssize_t r;
        pfd.fd = e->out_fd; pfd.events = POLLIN; pfd.revents = 0;
        if (poll(&pfd, 1, timeout_ms) <= 0) return 0;
        r = read(e->out_fd, &ch, 1);
        if (r <= 0) return 0;
        if (ch == '\r') continue;
        if (ch == '\n') { out[n] = 0; return 1; }
        if (n + 1 < cap) out[n++] = ch;
    }
}
static int e_wait_for(CerEngine *e, const char *needle, int max_lines) {
    char line[CER_MAX_LINE];
    int i;
    for (i = 0; i < max_lines; ++i) {
        if (!e_readline(e, line, sizeof(line), 5000)) return 0;
        if (e_eq(line, needle)) return 1;
    }
    return 0;
}
static int e_position(CerEngine *e, const SfGame *game) {
    if (!e || !game || !e->ready) return 0;
    if (game->start_fen[0]) {
        if (!e_write_all(e->in_fd, "position fen ") || !e_write_all(e->in_fd, game->start_fen)) return 0;
    } else if (!e_write_all(e->in_fd, "position startpos")) return 0;
    if (game->history_len && (!e_write_all(e->in_fd, " moves ") || !e_write_all(e->in_fd, game->history))) return 0;
    return e_write_all(e->in_fd, "\n");
}

void cer_engine_init(CerEngine *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(*engine));
    engine->in_fd = engine->out_fd = -1;
    engine->pid = -1;
    engine->level = 1;
}
void cer_engine_stop(CerEngine *e) {
    int status = 0;
    if (!e) return;
    if (e->in_fd >= 0) { e_write_all(e->in_fd, "quit\n"); close(e->in_fd); }
    if (e->out_fd >= 0) close(e->out_fd);
    if (e->pid > 0) {
        pid_t r = waitpid(e->pid, &status, 1);
        if (r == 0) { kill(e->pid, SIGTERM); waitpid(e->pid, &status, 0); }
    }
    e->in_fd = e->out_fd = -1; e->pid = -1; e->ready = 0;
}
int cer_engine_start(CerEngine *e) {
    int to_child[2], from_child[2];
    char path[1024];
    pid_t pid;
    if (!e) return 0;
    if (e->ready) return 1;
    if (!e_find_engine_path(path, sizeof(path))) return 0;
    if (pipe(to_child) != 0 || pipe(from_child) != 0) return 0;
    pid = fork();
    if (pid < 0) return 0;
    if (pid == 0) {
        char *argv[2]; argv[0] = path; argv[1] = 0;
        dup2(to_child[0], 0); dup2(from_child[1], 1); dup2(from_child[1], 2);
        close(to_child[0]); close(to_child[1]); close(from_child[0]); close(from_child[1]);
        execv(path, argv); _exit(127);
    }
    close(to_child[0]); close(from_child[1]);
    e->in_fd = to_child[1]; e->out_fd = from_child[0]; e->pid = pid; e->ready = 0;
    if (!e_write_all(e->in_fd, "uci\n") || !e_wait_for(e, "uciok", 256) ||
        !e_write_all(e->in_fd, "setoption name Threads value 1\n") ||
        !e_write_all(e->in_fd, "setoption name Hash value 32\n") ||
        !e_write_all(e->in_fd, "isready\n") || !e_wait_for(e, "readyok", 64)) {
        cer_engine_stop(e); return 0;
    }
    e->ready = 1;
    return cer_engine_set_level(e, e->level);
}
int cer_engine_set_level(CerEngine *e, int level) {
    int skill = level <= 0 ? 2 : (level == 1 ? 7 : (level == 2 ? 14 : 20));
    if (!e) return 0;
    e->level = level < 0 ? 0 : (level > 3 ? 3 : level);
    if (!e->ready) return 0;
    return e_command_int(e, "setoption name Skill Level value ", skill);
}
int cer_engine_legal_moves(CerEngine *e, const SfGame *game, char legal[][6], int capacity) {
    char line[CER_MAX_LINE], move[6];
    int count = 0;
    if (!e || !game || !legal || capacity <= 0 || !e_position(e, game) || !e_write_all(e->in_fd, "go perft 1\n")) return -1;
    for (;;) {
        if (!e_readline(e, line, sizeof(line), 5000)) return -1;
        if (strncmp(line, "Nodes searched:", 15) == 0) break;
        if (sf_parse_perft_move(line, move) && count < capacity) e_copy(legal[count++], 6, move);
    }
    return count;
}
int cer_engine_inspect(CerEngine *e, const SfGame *game, int *in_check) {
    char line[CER_MAX_LINE];
    int i;
    if (in_check) *in_check = 0;
    if (!e_position(e, game) || !e_write_all(e->in_fd, "d\n")) return 0;
    for (i = 0; i < 96; ++i) {
        char *p;
        if (!e_readline(e, line, sizeof(line), 5000)) return 0;
        if (strncmp(line, "Checkers:", 9) == 0) {
            p = line + 9;
            while (*p == ' ' || *p == '\t') ++p;
            if (in_check) *in_check = *p != 0;
            return 1;
        }
    }
    return 0;
}

int cer_parse_info_line(const char *line, CerAnalysis *a) {
    char *p;
    int v, changed = 0;
    if (!line || !a || strncmp(line, "info ", 5) != 0) return 0;
    p = e_find_token(line, " depth ");
    if (p && e_parse_int(p, &v)) { a->depth = v; changed = 1; }
    p = e_find_token(line, " score cp ");
    if (p && e_parse_int(p, &v)) { a->eval_cp = v; a->mate_in = 0; changed = 1; }
    p = e_find_token(line, " score mate ");
    if (p && e_parse_int(p, &v)) { a->mate_in = v; changed = 1; }
    p = e_find_token(line, " pv ");
    if (p) {
        size_t i = 0;
        while (p[i] && i + 1 < sizeof(a->pv)) { a->pv[i] = p[i]; ++i; }
        a->pv[i] = 0;
        changed = 1;
    }
    return changed;
}
int cer_engine_analyze(CerEngine *e, const SfGame *game, int think_ms, CerAnalysis *a) {
    char line[CER_MAX_LINE], move[6];
    if (!e || !game || !a || !e->ready) return 0;
    memset(a, 0, sizeof(*a));
    if (!e_position(e, game) || !e_command_int(e, "go movetime ", think_ms)) return 0;
    for (;;) {
        if (!e_readline(e, line, sizeof(line), think_ms + 7000)) return 0;
        cer_parse_info_line(line, a);
        if (sf_parse_bestmove(line, move)) { e_copy(a->bestmove, sizeof(a->bestmove), move); return 1; }
        if (strncmp(line, "bestmove (none)", 15) == 0) return -1;
    }
}
int cer_engine_bestmove(CerEngine *e, const SfGame *game, int think_ms, char out[6]) {
    CerAnalysis a;
    int r = cer_engine_analyze(e, game, think_ms, &a);
    if (r == 1) e_copy(out, 6, a.bestmove);
    return r;
}
