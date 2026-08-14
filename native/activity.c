#define SF_FREESTANDING 1
#include "core.h"

/* Minimal Android/Unix ABI declarations. The real symbols are supplied by
 * Android's libandroid/libc at runtime. Keeping this file freestanding lets us
 * build it with Clang even when the full Android SDK/NDK is unavailable. */
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long ssize_t;
typedef int pid_t;

typedef struct ANativeWindow ANativeWindow;
typedef struct AInputQueue AInputQueue;
typedef struct AInputEvent AInputEvent;
typedef struct ALooper ALooper;
typedef struct AAssetManager AAssetManager;
typedef void JavaVM;
typedef void JNIEnv;
typedef void *jobject;

typedef struct ARect { int32_t left, top, right, bottom; } ARect;
typedef struct ANativeWindow_Buffer {
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t format;
    void *bits;
    uint32_t reserved[6];
} ANativeWindow_Buffer;

typedef struct ANativeActivity ANativeActivity;
typedef struct ANativeActivityCallbacks {
    void (*onStart)(ANativeActivity *);
    void (*onResume)(ANativeActivity *);
    void *(*onSaveInstanceState)(ANativeActivity *, size_t *);
    void (*onPause)(ANativeActivity *);
    void (*onStop)(ANativeActivity *);
    void (*onDestroy)(ANativeActivity *);
    void (*onWindowFocusChanged)(ANativeActivity *, int);
    void (*onNativeWindowCreated)(ANativeActivity *, ANativeWindow *);
    void (*onNativeWindowResized)(ANativeActivity *, ANativeWindow *);
    void (*onNativeWindowRedrawNeeded)(ANativeActivity *, ANativeWindow *);
    void (*onNativeWindowDestroyed)(ANativeActivity *, ANativeWindow *);
    void (*onInputQueueCreated)(ANativeActivity *, AInputQueue *);
    void (*onInputQueueDestroyed)(ANativeActivity *, AInputQueue *);
    void (*onContentRectChanged)(ANativeActivity *, const ARect *);
    void (*onConfigurationChanged)(ANativeActivity *);
    void (*onLowMemory)(ANativeActivity *);
} ANativeActivityCallbacks;

struct ANativeActivity {
    ANativeActivityCallbacks *callbacks;
    JavaVM *vm;
    JNIEnv *env;
    jobject clazz;
    const char *internalDataPath;
    const char *externalDataPath;
    int32_t sdkVersion;
    void *instance;
    AAssetManager *assetManager;
    const char *obbPath;
};

/* libandroid */
extern void ANativeActivity_setWindowFlags(ANativeActivity *, uint32_t, uint32_t);
extern void ANativeActivity_setWindowFormat(ANativeActivity *, int32_t);
extern int32_t ANativeWindow_getWidth(ANativeWindow *);
extern int32_t ANativeWindow_getHeight(ANativeWindow *);
extern int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t);
extern int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *);
extern int32_t ANativeWindow_unlockAndPost(ANativeWindow *);
extern ALooper *ALooper_forThread(void);
extern ALooper *ALooper_prepare(int32_t);
typedef int (*ALooper_callbackFunc)(int, int, void *);
extern void AInputQueue_attachLooper(AInputQueue *, ALooper *, int, ALooper_callbackFunc, void *);
extern void AInputQueue_detachLooper(AInputQueue *);
extern int32_t AInputQueue_getEvent(AInputQueue *, AInputEvent **);
extern int32_t AInputQueue_preDispatchEvent(AInputQueue *, AInputEvent *);
extern void AInputQueue_finishEvent(AInputQueue *, AInputEvent *, int);
extern int32_t AInputEvent_getType(const AInputEvent *);
extern int32_t AMotionEvent_getAction(const AInputEvent *);
extern float AMotionEvent_getX(const AInputEvent *, size_t);
extern float AMotionEvent_getY(const AInputEvent *, size_t);

/* libc */
extern int open(const char *, int, ...);
extern ssize_t read(int, void *, size_t);
extern ssize_t write(int, const void *, size_t);
extern int close(int);
extern int pipe(int [2]);
extern pid_t fork(void);
extern int dup2(int, int);
extern int execv(const char *, char *const []);
extern void _exit(int);
extern pid_t waitpid(pid_t, int *, int);
extern int kill(pid_t, int);
extern void *malloc(size_t);
extern void free(void *);
extern void *memset(void *, int, size_t);
extern void *memcpy(void *, const void *, size_t);
extern size_t strlen(const char *);
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern char *strstr(const char *, const char *);
extern char *strchr(const char *, int);

struct pollfd { int fd; short events; short revents; };
extern int poll(struct pollfd *, unsigned long, int);

#define WINDOW_FORMAT_RGBA_8888 1
#define AINPUT_EVENT_TYPE_MOTION 2
#define AMOTION_EVENT_ACTION_MASK 0xff
#define AMOTION_EVENT_ACTION_UP 1
#define ALOOPER_PREPARE_ALLOW_NON_CALLBACKS 1
#define POLLIN 0x0001
#define POLLHUP 0x0010
#define O_RDONLY 0
#define SIGTERM 15

#define MAX_LEGAL 256
#define MAX_LINE 2048

static size_t s_len(const char *s) { return s ? strlen(s) : 0; }
static int s_eq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }

static uint32_t rgba(unsigned r, unsigned g, unsigned b, unsigned a) {
    return (uint32_t)(r | (g << 8) | (b << 16) | (a << 24));
}

typedef struct {
    int in_fd;
    int out_fd;
    pid_t pid;
    int ready;
} Engine;

typedef struct {
    ANativeActivity *activity;
    ANativeWindow *window;
    AInputQueue *input;
    ALooper *looper;
    Engine engine;
    SfGame game;
    char legal[MAX_LEGAL][6];
    int legal_count;
    int selected;
    int white_turn;
    int level;
    int think_ms;
    char status[64];
} App;

static App g_app;

static void text_copy(char *dst, size_t cap, const char *src) {
    size_t n = 0;
    if (!dst || cap == 0) return;
    if (src) while (src[n] && n + 1 < cap) { dst[n] = src[n]; ++n; }
    dst[n] = 0;
}

static void set_status(App *a, const char *s) { text_copy(a->status, sizeof(a->status), s); }

static int write_all(int fd, const char *s) {
    size_t left = s_len(s);
    while (left) {
        ssize_t n = write(fd, s, left);
        if (n <= 0) return 0;
        s += n;
        left -= (size_t)n;
    }
    return 1;
}

static int find_engine_path(char *out, size_t cap) {
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
static int engine_readline(Engine *e, char *out, size_t cap, int timeout_ms) {
    size_t n = 0;
    if (!e || e->out_fd < 0 || !out || cap < 2) return 0;
    for (;;) {
        struct pollfd pfd;
        char ch;
        ssize_t r;
        pfd.fd = e->out_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, timeout_ms) <= 0) return 0;
        if (pfd.revents & POLLHUP) {
            r = read(e->out_fd, &ch, 1);
            if (r <= 0) return 0;
        } else {
            r = read(e->out_fd, &ch, 1);
            if (r <= 0) return 0;
        }
        if (ch == '\r') continue;
        if (ch == '\n') { out[n] = 0; return 1; }
        if (n + 1 < cap) out[n++] = ch;
    }
}

static int engine_wait_for(Engine *e, const char *needle, int max_lines) {
    char line[MAX_LINE];
    int i;
    for (i = 0; i < max_lines; ++i) {
        if (!engine_readline(e, line, sizeof(line), 5000)) return 0;
        if (s_eq(line, needle)) return 1;
    }
    return 0;
}

static void engine_stop(Engine *e) {
    int status = 0;
    if (!e) return;
    if (e->in_fd >= 0) { write_all(e->in_fd, "quit\n"); close(e->in_fd); }
    if (e->out_fd >= 0) close(e->out_fd);
    if (e->pid > 0) {
        pid_t r = waitpid(e->pid, &status, 1);
        if (r == 0) { kill(e->pid, SIGTERM); waitpid(e->pid, &status, 0); }
    }
    e->in_fd = e->out_fd = -1;
    e->pid = -1;
    e->ready = 0;
}

static int engine_start(Engine *e) {
    int to_child[2], from_child[2];
    char path[1024];
    pid_t pid;
    if (!e || !find_engine_path(path, sizeof(path))) return 0;
    if (pipe(to_child) != 0 || pipe(from_child) != 0) return 0;
    pid = fork();
    if (pid < 0) return 0;
    if (pid == 0) {
        char *argv[2];
        argv[0] = path; argv[1] = 0;
        dup2(to_child[0], 0);
        dup2(from_child[1], 1);
        dup2(from_child[1], 2);
        close(to_child[0]); close(to_child[1]);
        close(from_child[0]); close(from_child[1]);
        execv(path, argv);
        _exit(127);
    }
    close(to_child[0]);
    close(from_child[1]);
    e->in_fd = to_child[1];
    e->out_fd = from_child[0];
    e->pid = pid;
    e->ready = 0;
    if (!write_all(e->in_fd, "uci\n") || !engine_wait_for(e, "uciok", 256)) { engine_stop(e); return 0; }
    if (!write_all(e->in_fd, "setoption name Threads value 1\n") ||
        !write_all(e->in_fd, "setoption name Hash value 16\n") ||
        !write_all(e->in_fd, "setoption name Skill Level value 10\n") ||
        !write_all(e->in_fd, "isready\n") || !engine_wait_for(e, "readyok", 64)) {
        engine_stop(e); return 0;
    }
    e->ready = 1;
    return 1;
}

static int engine_position(Engine *e, const SfGame *g) {
    if (!write_all(e->in_fd, "position startpos")) return 0;
    if (g->history_len) {
        if (!write_all(e->in_fd, " moves ") || !write_all(e->in_fd, g->history)) return 0;
    }
    return write_all(e->in_fd, "\n");
}

static int engine_legal_moves(App *a) {
    char line[MAX_LINE], move[6];
    int count = 0;
    if (!a->engine.ready || !engine_position(&a->engine, &a->game) || !write_all(a->engine.in_fd, "go perft 1\n")) return -1;
    for (;;) {
        if (!engine_readline(&a->engine, line, sizeof(line), 5000)) return -1;
        if (strncmp(line, "Nodes searched:", 15) == 0) break;
        if (sf_parse_perft_move(line, move) && count < MAX_LEGAL) {
            text_copy(a->legal[count], sizeof(a->legal[count]), move);
            ++count;
        }
    }
    a->legal_count = count;
    return count;
}

static int level_skill(int level) { return level == 0 ? 3 : (level == 1 ? 10 : 18); }
static int level_time(int level) { return level == 0 ? 250 : (level == 1 ? 650 : 1200); }
static const char *level_name(int level) { return level == 0 ? "EASY" : (level == 1 ? "MEDIUM" : "HARD"); }

static int engine_set_level(App *a) {
    const char *cmd = a->level == 0 ? "setoption name Skill Level value 3\n" :
                      (a->level == 1 ? "setoption name Skill Level value 10\n" : "setoption name Skill Level value 18\n");
    a->think_ms = level_time(a->level);
    (void)level_skill(a->level);
    return a->engine.ready ? write_all(a->engine.in_fd, cmd) : 0;
}

static int engine_bestmove(App *a, char out[6]) {
    char line[MAX_LINE], move[6];
    const char *go = a->level == 0 ? "go movetime 250\n" :
                     (a->level == 1 ? "go movetime 650\n" : "go movetime 1200\n");
    if (!a->engine.ready || !engine_position(&a->engine, &a->game) || !write_all(a->engine.in_fd, go)) return 0;
    for (;;) {
        if (!engine_readline(&a->engine, line, sizeof(line), 8000)) return 0;
        if (sf_parse_bestmove(line, move)) { text_copy(out, 6, move); return 1; }
        if (strncmp(line, "bestmove (none)", 15) == 0) return -1;
    }
}

/* Tiny 5x7 font. Each row uses the low five bits. */
static const unsigned char *glyph(char c) {
#define G(name,a,b,c,d,e,f,g) static const unsigned char name[7]={a,b,c,d,e,f,g}
    G(A,14,17,17,31,17,17,17); G(B,30,17,17,30,17,17,30);
    G(C,14,17,16,16,16,17,14); G(D,30,17,17,17,17,17,30);
    G(E,31,16,16,30,16,16,31); G(F,31,16,16,30,16,16,16);
    G(G,14,17,16,23,17,17,14); G(H,17,17,17,31,17,17,17);
    G(I,31,4,4,4,4,4,31); G(J,7,2,2,2,18,18,12);
    G(K,17,18,20,24,20,18,17); G(L,16,16,16,16,16,16,31);
    G(M,17,27,21,21,17,17,17); G(N,17,25,21,19,17,17,17);
    G(O,14,17,17,17,17,17,14); G(P,30,17,17,30,16,16,16);
    G(Q,14,17,17,17,21,18,13); G(R,30,17,17,30,20,18,17);
    G(S,15,16,16,14,1,1,30); G(T,31,4,4,4,4,4,4);
    G(U,17,17,17,17,17,17,14); G(V,17,17,17,17,17,10,4);
    G(W,17,17,17,21,21,21,10); G(X,17,17,10,4,10,17,17);
    G(Y,17,17,10,4,4,4,4); G(Z,31,1,2,4,8,16,31);
    G(COLON,0,4,4,0,4,4,0); G(DASH,0,0,0,31,0,0,0); G(DOT,0,0,0,0,0,4,4);
    G(ZERO,14,17,19,21,25,17,14); G(ONE,4,12,4,4,4,4,14); G(TWO,14,17,1,2,4,8,31);
    G(THREE,30,1,1,14,1,1,30); G(FOUR,2,6,10,18,31,2,2); G(FIVE,31,16,16,30,1,1,30);
    G(SIX,14,16,16,30,17,17,14); G(SEVEN,31,1,2,4,8,8,8); G(EIGHT,14,17,17,14,17,17,14);
    G(NINE,14,17,17,15,1,1,14);
    static const unsigned char SPACE[7]={0,0,0,0,0,0,0};
#undef G
    switch (c) {
        case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D;
        case 'E': return E; case 'F': return F; case 'G': return G; case 'H': return H;
        case 'I': return I; case 'J': return J; case 'K': return K; case 'L': return L;
        case 'M': return M; case 'N': return N; case 'O': return O; case 'P': return P;
        case 'Q': return Q; case 'R': return R; case 'S': return S; case 'T': return T;
        case 'U': return U; case 'V': return V; case 'W': return W; case 'X': return X;
        case 'Y': return Y; case 'Z': return Z; case ':': return COLON; case '-': return DASH;
        case '.': return DOT; case '0': return ZERO; case '1': return ONE; case '2': return TWO;
        case '3': return THREE; case '4': return FOUR; case '5': return FIVE; case '6': return SIX;
        case '7': return SEVEN; case '8': return EIGHT; case '9': return NINE; default: return SPACE;
    }
}

static void fill_rect(ANativeWindow_Buffer *b, int x, int y, int w, int h, uint32_t color) {
    int yy, xx;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > b->width) w = b->width - x;
    if (y + h > b->height) h = b->height - y;
    if (w <= 0 || h <= 0) return;
    for (yy = y; yy < y + h; ++yy) {
        uint32_t *row = (uint32_t *)b->bits + yy * b->stride;
        for (xx = x; xx < x + w; ++xx) row[xx] = color;
    }
}

static void fill_circle(ANativeWindow_Buffer *b, int cx, int cy, int r, uint32_t color) {
    int y, x;
    int rr = r * r;
    for (y = -r; y <= r; ++y) {
        int yy = cy + y;
        if (yy < 0 || yy >= b->height) continue;
        for (x = -r; x <= r; ++x) {
            int xx = cx + x;
            if (xx < 0 || xx >= b->width) continue;
            if (x*x + y*y <= rr) ((uint32_t *)b->bits)[yy * b->stride + xx] = color;
        }
    }
}

static void draw_char(ANativeWindow_Buffer *b, int x, int y, char c, int scale, uint32_t color) {
    const unsigned char *g = glyph(c);
    int row, col;
    for (row = 0; row < 7; ++row) for (col = 0; col < 5; ++col)
        if (g[row] & (1u << (4-col))) fill_rect(b, x + col*scale, y + row*scale, scale, scale, color);
}

static int text_width(const char *s, int scale) { return (int)s_len(s) * 6 * scale; }
static void draw_text(ANativeWindow_Buffer *b, int x, int y, const char *s, int scale, uint32_t color) {
    while (s && *s) { draw_char(b, x, y, *s, scale, color); x += 6*scale; ++s; }
}
static void draw_text_center(ANativeWindow_Buffer *b, int cx, int y, const char *s, int scale, uint32_t color) {
    draw_text(b, cx - text_width(s, scale)/2, y, s, scale, color);
}

static void board_layout(const ANativeWindow_Buffer *b, int *bx, int *by, int *sq, int *button_y) {
    int margin = b->width / 40;
    int top = b->height / 12;
    int max_board = b->width - margin*2;
    int by_space = b->height - top - (b->height/7);
    int s;
    if (by_space < max_board) max_board = by_space;
    s = max_board / 8;
    if (s < 24) s = 24;
    *sq = s;
    *bx = (b->width - s*8)/2;
    *by = top;
    *button_y = top + s*8 + (b->height/28);
}

static int selected_has_dest(App *a, int dest) {
    char from[3], to[3], picked[6];
    const char *ptrs[MAX_LEGAL];
    int i;
    if (a->selected < 0) return 0;
    sf_index_to_square(a->selected, from);
    sf_index_to_square(dest, to);
    for (i=0;i<a->legal_count;++i) ptrs[i]=a->legal[i];
    return sf_pick_legal_move(ptrs, (size_t)a->legal_count, from, to, picked);
}

static int source_has_move(App *a, int idx) {
    char sq[3];
    int i;
    sf_index_to_square(idx, sq);
    for (i=0;i<a->legal_count;++i) if (a->legal[i][0]==sq[0] && a->legal[i][1]==sq[1]) return 1;
    return 0;
}

static void draw(App *a) {
    ANativeWindow_Buffer b;
    ARect dirty;
    int bx, by, sq, button_y;
    int r, f, i;
    uint32_t bg = rgba(38,36,33,255), light = rgba(238,238,210,255), dark = rgba(118,150,86,255);
    uint32_t ink = rgba(245,245,245,255), muted = rgba(190,190,190,255);
    if (!a || !a->window) return;
    dirty.left = dirty.top = 0; dirty.right = dirty.bottom = 0;
    if (ANativeWindow_lock(a->window, &b, &dirty) != 0 || !b.bits) return;
    fill_rect(&b, 0, 0, b.width, b.height, bg);
    board_layout(&b, &bx, &by, &sq, &button_y);
    draw_text_center(&b, b.width/2, b.height/55, "STOCKFISH CHESS", b.width > 700 ? 5 : 3, ink);
    draw_text_center(&b, b.width/2, b.height/55 + (b.width > 700 ? 48 : 32), "YOU ARE WHITE", b.width > 700 ? 3 : 2, muted);

    for (r=0;r<8;++r) for (f=0;f<8;++f) {
        int rank = 7-r;
        int idx = rank*8+f;
        int x = bx+f*sq, y=by+r*sq;
        uint32_t c = ((r+f)&1) ? dark : light;
        if (idx == a->selected) c = rgba(246,246,105,255);
        fill_rect(&b,x,y,sq,sq,c);
        if (a->selected >= 0 && selected_has_dest(a, idx))
            fill_circle(&b, x+sq/2, y+sq/2, sq/10, rgba(70,70,70,255));
        if (a->game.board[idx]) {
            char p = a->game.board[idx];
            int white = sf_is_white_piece(p);
            char letter = white ? p : (char)(p-'a'+'A');
            int rad = sq*31/100;
            int sc = sq/15; if (sc < 2) sc=2;
            fill_circle(&b,x+sq/2,y+sq/2,rad, white ? rgba(248,248,240,255) : rgba(55,54,52,255));
            fill_circle(&b,x+sq/2,y+sq/2,rad-3, white ? rgba(225,224,210,255) : rgba(75,73,70,255));
            draw_char(&b,x+sq/2-(5*sc)/2,y+sq/2-(7*sc)/2,letter,sc,white?rgba(45,44,42,255):rgba(245,245,240,255));
        }
    }

    draw_text_center(&b,b.width/2,by+sq*8+(b.height/80),a->status,b.width>700?3:2,ink);
    {
        int gap=b.width/40, bh=b.height/18; if (bh<56) bh=56;
        int bw=(b.width-gap*3)/2;
        int y=button_y;
        fill_rect(&b,gap,y,bw,bh,rgba(75,73,70,255));
        fill_rect(&b,gap*2+bw,y,bw,bh,rgba(75,73,70,255));
        draw_text_center(&b,gap+bw/2,y+bh/2-7*(b.width>700?2:1),"NEW GAME",b.width>700?2:1,ink);
        draw_text_center(&b,gap*2+bw+bw/2,y+bh/2-7*(b.width>700?2:1),level_name(a->level),b.width>700?2:1,ink);
    }
    draw_text_center(&b,b.width/2,b.height-(b.height/38),"OFFLINE STOCKFISH",b.width>700?2:1,muted);
    for (i=0;i<1;++i) { (void)i; }
    ANativeWindow_unlockAndPost(a->window);
}

static void new_game(App *a) {
    sf_game_reset(&a->game);
    a->selected = -1;
    a->legal_count = 0;
    a->white_turn = 1;
    if (a->engine.ready) { write_all(a->engine.in_fd,"ucinewgame\n"); write_all(a->engine.in_fd,"isready\n"); engine_wait_for(&a->engine,"readyok",64); engine_set_level(a); }
    set_status(a, a->engine.ready ? "YOUR MOVE" : "ENGINE ERROR");
}

static void select_or_move(App *a, int idx) {
    int i;
    char from[3], to[3], picked[6], reply[6];
    const char *ptrs[MAX_LEGAL];
    if (!a->engine.ready || !a->white_turn) return;
    if (a->selected < 0) {
        int n = engine_legal_moves(a);
        if (n <= 0) { set_status(a, n==0?"GAME OVER":"ENGINE ERROR"); draw(a); return; }
        if (sf_is_white_piece(a->game.board[idx]) && source_has_move(a, idx)) { a->selected=idx; set_status(a,"SELECT MOVE"); }
        draw(a); return;
    }
    if (idx == a->selected) { a->selected=-1; set_status(a,"YOUR MOVE"); draw(a); return; }
    sf_index_to_square(a->selected,from); sf_index_to_square(idx,to);
    for (i=0;i<a->legal_count;++i) ptrs[i]=a->legal[i];
    if (!sf_pick_legal_move(ptrs,(size_t)a->legal_count,from,to,picked)) {
        if (sf_is_white_piece(a->game.board[idx]) && source_has_move(a,idx)) { a->selected=idx; set_status(a,"SELECT MOVE"); }
        else set_status(a,"ILLEGAL MOVE");
        draw(a); return;
    }
    if (!sf_apply_uci(&a->game,picked)) { set_status(a,"MOVE ERROR"); draw(a); return; }
    a->selected=-1; a->legal_count=0; a->white_turn=0; set_status(a,"STOCKFISH THINKING"); draw(a);
    {
        int br=engine_bestmove(a,reply);
        if (br==1 && sf_apply_uci(&a->game,reply)) { a->white_turn=1; set_status(a,"YOUR MOVE"); }
        else if (br==-1) set_status(a,"YOU WIN");
        else set_status(a,"ENGINE ERROR");
    }
    draw(a);
}

static void handle_touch(App *a, float fx, float fy) {
    ANativeWindow_Buffer geom;
    int bx,by,sq,button_y;
    int x=(int)fx,y=(int)fy;
    if (!a->window) return;
    memset(&geom,0,sizeof(geom));
    geom.width=ANativeWindow_getWidth(a->window);
    geom.height=ANativeWindow_getHeight(a->window);
    if (geom.width<=0 || geom.height<=0) return;
    board_layout(&geom,&bx,&by,&sq,&button_y);
    if (y>=by && y<by+sq*8 && x>=bx && x<bx+sq*8) {
        int file=(x-bx)/sq, screen_rank=(y-by)/sq, rank=7-screen_rank;
        select_or_move(a,rank*8+file); return;
    }
    if (y>=button_y && y<button_y+(geom.height/18<56?56:geom.height/18)) {
        if (x<geom.width/2) new_game(a);
        else { a->level=(a->level+1)%3; engine_set_level(a); set_status(a,"LEVEL CHANGED"); }
        draw(a);
    }
}

static int input_callback(int fd, int events, void *data) {
    App *a=(App*)data;
    AInputEvent *ev=0;
    (void)fd; (void)events;
    while (a && a->input && AInputQueue_getEvent(a->input,&ev)>=0) {
        int handled=0;
        if (AInputQueue_preDispatchEvent(a->input,ev)) continue;
        if (AInputEvent_getType(ev)==AINPUT_EVENT_TYPE_MOTION) {
            int action=AMotionEvent_getAction(ev)&AMOTION_EVENT_ACTION_MASK;
            if (action==AMOTION_EVENT_ACTION_UP) { handle_touch(a,AMotionEvent_getX(ev,0),AMotionEvent_getY(ev,0)); handled=1; }
        }
        AInputQueue_finishEvent(a->input,ev,handled);
    }
    return 1;
}

static void on_window_created(ANativeActivity *activity, ANativeWindow *window) {
    App *a=(App*)activity->instance;
    a->window=window;
    ANativeWindow_setBuffersGeometry(window,0,0,WINDOW_FORMAT_RGBA_8888);
    draw(a);
}
static void on_window_resized(ANativeActivity *activity, ANativeWindow *window) { (void)window; draw((App*)activity->instance); }
static void on_window_redraw(ANativeActivity *activity, ANativeWindow *window) { (void)window; draw((App*)activity->instance); }
static void on_window_destroyed(ANativeActivity *activity, ANativeWindow *window) { App *a=(App*)activity->instance; if(a->window==window)a->window=0; }
static void on_input_created(ANativeActivity *activity,AInputQueue *q) {
    App *a=(App*)activity->instance; a->input=q;
    a->looper=ALooper_forThread(); if(!a->looper)a->looper=ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    if(a->looper)AInputQueue_attachLooper(q,a->looper,1,input_callback,a);
}
static void on_input_destroyed(ANativeActivity *activity,AInputQueue *q) { App *a=(App*)activity->instance; if(a->input==q){AInputQueue_detachLooper(q);a->input=0;} }
static void on_destroy(ANativeActivity *activity) { App *a=(App*)activity->instance; engine_stop(&a->engine); }

__attribute__((visibility("default")))
void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    App *a=&g_app;
    (void)savedState; (void)savedStateSize;
    memset(a,0,sizeof(*a));
    a->activity=activity; a->engine.in_fd=-1; a->engine.out_fd=-1; a->engine.pid=-1; a->selected=-1; a->level=1; a->think_ms=650;
    activity->instance=a;
    activity->callbacks->onDestroy=on_destroy;
    activity->callbacks->onNativeWindowCreated=on_window_created;
    activity->callbacks->onNativeWindowResized=on_window_resized;
    activity->callbacks->onNativeWindowRedrawNeeded=on_window_redraw;
    activity->callbacks->onNativeWindowDestroyed=on_window_destroyed;
    activity->callbacks->onInputQueueCreated=on_input_created;
    activity->callbacks->onInputQueueDestroyed=on_input_destroyed;
    ANativeActivity_setWindowFlags(activity,0x00000480u,0); /* fullscreen + keep screen on */
    ANativeActivity_setWindowFormat(activity,WINDOW_FORMAT_RGBA_8888);
    sf_game_reset(&a->game); a->white_turn=1;
    set_status(a,"STARTING STOCKFISH");
    if(engine_start(&a->engine)){engine_set_level(a);set_status(a,"YOUR MOVE");}else set_status(a,"ENGINE ERROR");
}
