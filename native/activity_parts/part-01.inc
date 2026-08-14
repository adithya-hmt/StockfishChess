#define SF_FREESTANDING 1
#include "platform.h"
#include "core.h"
#include "state.h"
#include "engine.h"
#include "ui.h"

#define MAX_LEGAL 256
#define MAX_HITS 192
#define MAX_TOAST 80

/* Screens */
enum {
    SCR_SPLASH=0, SCR_ONBOARD, SCR_PROFILE_CREATE, SCR_PIN, SCR_UNLOCK,
    SCR_HOME, SCR_PLAY, SCR_GAME, SCR_PUZZLES, SCR_PUZZLE_GAME,
    SCR_HISTORY, SCR_REPLAY, SCR_ANALYSIS, SCR_PROFILE, SCR_SETTINGS
};
/* Game modes */
enum { MODE_ENGINE=0, MODE_LOCAL=1, MODE_PUZZLE=2 };
enum { WORK_NONE=0, WORK_MOVE=1, WORK_HINT=2, WORK_ANALYSIS=3 };
/* Results, absolute board result */
enum { RESULT_NONE=0, RESULT_WHITE=1, RESULT_BLACK=2, RESULT_DRAW=3, RESULT_ENGINE=4 };
/* End reasons */
enum { REASON_NONE=0, REASON_MATE=1, REASON_STALEMATE=2, REASON_RESIGN=3, REASON_TIMEOUT=4, REASON_MATERIAL=5, REASON_ENGINE=6 };
/* Overlays */
enum { OVERLAY_NONE=0, OVERLAY_PROMOTION, OVERLAY_GAME_MENU, OVERLAY_RESULT, OVERLAY_CONFIRM_RESIGN, OVERLAY_PROFILE_PICKER, OVERLAY_RESET };
/* Tabs */
enum { TAB_HOME=0, TAB_PLAY=1, TAB_PUZZLES=2, TAB_HISTORY=3, TAB_PROFILE=4 };

/* Hit ids. Dynamic ranges are deliberately far apart because C does not
   provide guard rails and humans apparently enjoy touching everything. */
enum {
    HIT_NONE=0, HIT_BACK=1, HIT_NEXT=2, HIT_SKIP=3, HIT_CONTINUE=4,
    HIT_PIN_SKIP=5, HIT_PIN_ENABLE=6, HIT_PIN_DELETE=7,
    HIT_HOME_HERO=10, HIT_HOME_STOCKFISH=11, HIT_HOME_LOCAL=12,
    HIT_HOME_PUZZLES=13, HIT_HOME_ANALYSIS=14, HIT_HOME_RECENT=15,
    HIT_START_GAME=20, HIT_MODE_ENGINE=21, HIT_MODE_LOCAL=22,
    HIT_SIDE_WHITE=23, HIT_SIDE_BLACK=24, HIT_SIDE_RANDOM=25,
    HIT_LEVEL_EASY=26, HIT_LEVEL_MEDIUM=27, HIT_LEVEL_HARD=28,
    HIT_TIME_NONE=29, HIT_TIME_3=30, HIT_TIME_5=31, HIT_TIME_10=32,
    HIT_TOGGLE_HINTS=33, HIT_TOGGLE_LEGAL=34,
    HIT_GAME_UNDO=40, HIT_GAME_HINT=41, HIT_GAME_FLIP=42, HIT_GAME_MENU=43,
    HIT_GAME_ANALYSIS=44, HIT_GAME_RESIGN=45, HIT_GAME_RESTART=46, HIT_GAME_EXPORT=47,
    HIT_GAME_HOME=48, HIT_GAME_REMATCH=49, HIT_CONFIRM_RESIGN=50, HIT_CANCEL=51,
    HIT_PROMO_Q=55, HIT_PROMO_R=56, HIT_PROMO_B=57, HIT_PROMO_N=58,
    HIT_REPLAY_PREV=60, HIT_REPLAY_NEXT=61, HIT_REPLAY_ANALYZE=62, HIT_REPLAY_FLIP=63,
    HIT_ANALYZE_RUN=64, HIT_ANALYZE_PREV=65, HIT_ANALYZE_NEXT=66, HIT_ANALYZE_FLIP=67,
    HIT_PROFILE_SETTINGS=70, HIT_PROFILE_ADD=71, HIT_PROFILE_LOCK=72, HIT_PROFILE_SWITCH=73,
    HIT_SETTING_HAPTICS=80, HIT_SETTING_COORDS=81, HIT_SETTING_LEGAL=82,
    HIT_SETTING_AUTO_QUEEN=83, HIT_SETTING_MOTION=84, HIT_SETTING_BOARD=85,
    HIT_SETTING_PIECES=86, HIT_SETTING_RESET=87, HIT_SETTING_SOUND=88,
    HIT_NAV_BASE=100, HIT_KEY_BASE=200, HIT_AVATAR_BASE=260,
    HIT_PUZZLE_BASE=300, HIT_HISTORY_BASE=340, HIT_PROFILE_BASE=380,
    HIT_PIN_BASE=420
};

typedef struct { int id,x,y,w,h; } Hit;
typedef struct { int x,y,w,h; } Rect;

typedef struct {
    const char *title;
    const char *subtitle;
    const char *fen;
    const char *solution;
    int rating;
} Puzzle;

static const Puzzle PUZZLES[] = {
    {"FOOL'S MATE","BLACK TO MOVE - MATE IN 1","rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2","d8h4",650},
    {"SCHOLAR'S SHOT","WHITE TO MOVE - MATE IN 1","r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4","h5f7",850},
    {"QUEEN AND KING","WHITE TO MOVE - MATE IN 1","7k/5Q2/6K1/8/8/8/8/8 w - - 0 1","f7g7",900},
    {"ROOK CORRIDOR","WHITE TO MOVE - MATE IN 1","7k/8/6K1/8/8/8/8/5R2 w - - 0 1","f1f8",1050},
    {"KNIGHT FORK","WHITE TO MOVE - FORK KING AND ROOK","r3k3/8/8/1N6/8/8/8/4K3 w - - 0 1","b5c7",1150}
};
#define PUZZLE_COUNT ((int)(sizeof(PUZZLES)/sizeof(PUZZLES[0])))

typedef struct {
    ANativeActivity *activity;
    ANativeWindow *window;
    AInputQueue *input;
    ALooper *looper;
    CerEngine engine;
    CerPersisted saved;
    SfGame game;
    SfGame view_game;
    CerAnalysis analysis;
    char data_dir[1024];
    char export_dir[1024];

    int screen, previous_screen, tab, onboarding_page, overlay;
    int setup_mode, setup_side, setup_level, setup_time;
    int game_mode, human_side, level, time_control;
    int side_to_move, flipped, game_active, game_over, result, result_reason;
    int engine_error;
    int worker_busy, worker_kind, worker_result, worker_joinable;
    char worker_move[6];
    CerAnalysis worker_analysis;
    SfGame worker_game;
    int worker_think_ms;
    uint64_t worker_position_hash;
    pthread_t worker_thread;
    int notify_pipe[2];
    int64_t splash_until_ms;
    int selected, legal_count;
    char legal[MAX_LEGAL][6];
    int hint_from, hint_to, last_from, last_to;
    int in_check, checked_king;
    int pending_promo, promo_from, promo_to;
    int dragging, drag_from, drag_x, drag_y, drag_start_x, drag_start_y, drag_moved;
    int anim_active, anim_from, anim_to, anim_progress;
    char anim_piece;
    int64_t white_ms, black_ms, turn_started_ms;
    char status[64];

    int puzzle_index, puzzle_complete;
    int history_index, replay_ply;
    int analysis_ply, analysis_ready;
    char profile_name[CER_NAME_CAP];
    int profile_avatar, profile_accent;
    char pin_entry[5], pin_first[5];
    int pin_digits, pin_stage;

    char toast[MAX_TOAST];
    int toast_kind;
    int64_t toast_until_ms;
    Hit hits[MAX_HITS];
    int hit_count;
    int board_x, board_y, board_sq, board_size, board_interactive;
    int timer_pipe[2];
    pthread_t timer_thread;
    int timer_running;
} App;

static App g_app;

/* Original Cerelytic palette. Stored RGBA in little-endian Android buffers. */
static uint32_t bg(void){return cer_rgba(18,20,18,255);} 
static uint32_t bg_deep(void){return cer_rgba(12,14,13,255);} 
static uint32_t surface(void){return cer_rgba(31,35,31,255);} 
static uint32_t surface2(void){return cer_rgba(40,45,40,255);} 
static uint32_t surface3(void){return cer_rgba(53,59,52,255);} 
static uint32_t stroke(void){return cer_rgba(66,73,65,255);} 
static uint32_t ink(void){return cer_rgba(244,247,242,255);} 
static uint32_t muted(void){return cer_rgba(169,178,167,255);} 
static uint32_t green(void){return cer_rgba(112,184,74,255);} 
static uint32_t green_dim(void){return cer_rgba(47,87,47,255);} 
static uint32_t mint(void){return cer_rgba(118,216,167,255);} 
static uint32_t gold(void){return cer_rgba(221,190,80,255);} 
static uint32_t amber(void){return cer_rgba(242,157,73,255);} 
static uint32_t red(void){return cer_rgba(229,72,77,255);} 
static uint32_t red_dim(void){return cer_rgba(118,38,45,255);} 
static uint32_t ivory(void){return cer_rgba(238,235,210,255);} 
static uint32_t moss(void){return cer_rgba(120,155,89,255);} 

static size_t s_len(const char *s){return s?strlen(s):0;}
static int s_eq(const char*a,const char*b){return a&&b&&strcmp(a,b)==0;}
static int i_min(int a,int b){return a<b?a:b;}
static int i_max(int a,int b){return a>b?a:b;}
static int i_abs(int a){return a<0?-a:a;}
static int i_clamp(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
static Rect make_rect(int x,int y,int w,int h){Rect r;r.x=x;r.y=y;r.w=w;r.h=h;return r;}
static int64_t now_ms(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)!=0)return 0;return(int64_t)t.tv_sec*1000+(int64_t)t.tv_nsec/1000000;}
static void sleep_ms(int ms){struct timespec t;if(ms<=0)return;t.tv_sec=ms/1000;t.tv_nsec=(long)(ms%1000)*1000000L;(void)nanosleep(&t,0);}
static void append_text(char*dst,size_t cap,const char*src){size_t at=s_len(dst),i=0;if(!dst||!cap||at>=cap)return;while(src&&src[i]&&at+1<cap)dst[at++]=src[i++];dst[at]=0;}
static void int_text(int value,char*out,size_t cap){char tmp[24];int n=0,i=0;unsigned v;if(!out||!cap)return;if(value<0){if(i+1<(int)cap)out[i++]='-';v=(unsigned)(-value);}else v=(unsigned)value;do{tmp[n++]=(char)('0'+v%10u);v/=10u;}while(v&&n<(int)sizeof(tmp));while(n>0&&i+1<(int)cap)out[i++]=tmp[--n];out[i]=0;}
static void format_clock(int64_t ms,char out[12]){int total,mins,secs;char n[12];if(ms<0)ms=0;total=(int)(ms/1000);mins=total/60;secs=total%60;int_text(mins,out,12);append_text(out,12,":");if(secs<10)append_text(out,12,"0");int_text(secs,n,sizeof(n));append_text(out,12,n);}
static void format_score(int cp,int mate,char out[24]){char n[16];out[0]=0;if(mate){append_text(out,24,"MATE ");int_text(mate,n,sizeof(n));append_text(out,24,n);return;}if(cp>=0)append_text(out,24,"+");int_text(cp/100,n,sizeof(n));append_text(out,24,n);append_text(out,24,".");{int rem=i_abs(cp%100);if(rem<10)append_text(out,24,"0");int_text(rem,n,sizeof(n));append_text(out,24,n);}}
static const char* level_name(int l){return l<=0?"CALM":(l==1?"SHARP":"RUTHLESS");}
static int level_think_ms(int l){return l<=0?220:(l==1?520:1050);}
static uint32_t accent_for(int i){static const unsigned r[5]={112,78,187,214,80};static const unsigned g[5]={184,153,126,166,145};static const unsigned b[5]={74,198,203,76,214};i=i_clamp(i,0,4);return cer_rgba(r[i],g[i],b[i],255);}
static void set_status(App*a,const char*s){cer_copy_text(a->status,sizeof(a->status),s);}
static void set_toast(App*a,const char*s,int kind){cer_copy_text(a->toast,sizeof(a->toast),s);a->toast_kind=kind;a->toast_until_ms=now_ms()+2600;}

static int px(int width,int dp){return i_max(1,width*dp/360);}
static int safe_top(int width){return px(width,18);}
static int topbar_h(int width){return px(width,58)+safe_top(width);}
static int nav_h(int width){return px(width,76);}
static int page_margin(int width){return px(width,16);}

static void hit_reset(App*a){a->hit_count=0;a->board_interactive=0;}
static void hit_add(App*a,int id,Rect r){Hit*h;if(!a||a->hit_count>=MAX_HITS||r.w<=0||r.h<=0)return;h=&a->hits[a->hit_count++];h->id=id;h->x=r.x;h->y=r.y;h->w=r.w;h->h=r.h;}
static int hit_at(const App*a,int x,int y){int i;for(i=a->hit_count-1;i>=0;--i){const Hit*h=&a->hits[i];if(x>=h->x&&x<h->x+h->w&&y>=h->y&&y<h->y+h->h)return h->id;}return HIT_NONE;}

static void haptic(App*a,int feedback){
    JNIEnv*env;jclass cls;jmethodID get_window,get_decor,perform;jobject window,decor;
    if(!a||!a->activity||!a->saved.haptics_enabled)return;
    env=a->activity->env;if(!env||!(*env))return;
    cls=(*env)->GetObjectClass(env,a->activity->clazz);if(!cls)return;
    get_window=(*env)->GetMethodID(env,cls,"getWindow","()Landroid/view/Window;");
    if(!get_window){(*env)->DeleteLocalRef(env,cls);return;}
    window=(*env)->CallObjectMethod(env,a->activity->clazz,get_window);
    if(!window){(*env)->DeleteLocalRef(env,cls);return;}
    {jclass wc=(*env)->GetObjectClass(env,window);get_decor=wc?(*env)->GetMethodID(env,wc,"getDecorView","()Landroid/view/View;"):0;if(wc)(*env)->DeleteLocalRef(env,wc);}
    decor=get_decor?(*env)->CallObjectMethod(env,window,get_decor):0;
    if(decor){jclass vc=(*env)->GetObjectClass(env,decor);perform=vc?(*env)->GetMethodID(env,vc,"performHapticFeedback","(I)Z"):0;if(perform)(*env)->CallBooleanMethod(env,decor,perform,feedback);if(vc)(*env)->DeleteLocalRef(env,vc);(*env)->DeleteLocalRef(env,decor);}
    (*env)->DeleteLocalRef(env,window);(*env)->DeleteLocalRef(env,cls);
}

static void draw(App*a);
static void engine_turn(App*a);
static void apply_move_animated(App*a,const char move[6]);
