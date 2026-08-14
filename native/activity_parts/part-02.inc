static int refresh_game_status(App*a);

static void save_state(App*a){
    if(!a)return;
    if(a->game_active){
        a->saved.active_game=1;a->saved.active_mode=a->game_mode;a->saved.active_level=a->level;
        a->saved.active_human_side=a->human_side;a->saved.active_time_control=a->time_control;
        a->saved.active_board_flipped=a->flipped;a->saved.active_white_ms=a->white_ms;a->saved.active_black_ms=a->black_ms;
        cer_copy_text(a->saved.active_fen,sizeof(a->saved.active_fen),a->game.start_fen);
        cer_copy_text(a->saved.active_moves,sizeof(a->saved.active_moves),a->game.history);
    }else{
        a->saved.active_game=0;a->saved.active_moves[0]=0;a->saved.active_fen[0]=0;
    }
    if(!cer_state_save(a->data_dir,&a->saved))set_toast(a,"LOCAL SAVE FAILED",2);
}

static void clear_move_ui(App*a){a->selected=-1;a->legal_count=0;a->hint_from=a->hint_to=-1;a->pending_promo=0;a->dragging=0;a->drag_moved=0;}
static int current_side_is_human(const App*a){if(a->game_mode==MODE_LOCAL||a->game_mode==MODE_PUZZLE)return 1;return a->side_to_move? a->human_side==0 : a->human_side==1;}
static int side_piece(char p,int white){return white?sf_is_white_piece(p):sf_is_black_piece(p);}
static void update_last_move(App*a,const SfGame*g){int count=sf_history_move_count(g);char m[6];a->last_from=a->last_to=-1;if(count>0&&sf_history_get_move(g,count-1,m)){a->last_from=sf_square_to_index(m);a->last_to=sf_square_to_index(m+2);}}
static int load_legal(App*a){int n;if(!a->engine.ready){a->legal_count=0;return-1;}n=cer_engine_legal_moves(&a->engine,&a->game,a->legal,MAX_LEGAL);a->legal_count=n>0?n:0;return n;}
static int source_has_move(const App*a,int idx){char sq[3];int i;sf_index_to_square(idx,sq);for(i=0;i<a->legal_count;++i)if(a->legal[i][0]==sq[0]&&a->legal[i][1]==sq[1])return 1;return 0;}
static int destination_has_move(const App*a,int from,int dest){char fs[3],ts[3];int i;sf_index_to_square(from,fs);sf_index_to_square(dest,ts);for(i=0;i<a->legal_count;++i){const char*m=a->legal[i];if(m[0]==fs[0]&&m[1]==fs[1]&&m[2]==ts[0]&&m[3]==ts[1])return 1;}return 0;}
static int promotion_count(const App*a,int from,int to){char fs[3],ts[3];int i,n=0;sf_index_to_square(from,fs);sf_index_to_square(to,ts);for(i=0;i<a->legal_count;++i){const char*m=a->legal[i];if(m[0]==fs[0]&&m[1]==fs[1]&&m[2]==ts[0]&&m[3]==ts[1]&&s_len(m)==5)++n;}return n;}
static int pick_move(const App*a,int from,int to,char promo,char out[6]){char fs[3],ts[3];const char*ptrs[MAX_LEGAL];int i;sf_index_to_square(from,fs);sf_index_to_square(to,ts);for(i=0;i<a->legal_count;++i)ptrs[i]=a->legal[i];return sf_pick_legal_move_promotion(ptrs,(size_t)a->legal_count,fs,ts,promo,out);}

static void configure_engine(App*a){
    if(!a->engine.ready){if(!cer_engine_start(&a->engine)){a->engine_error=1;return;}}
    a->engine_error=0;cer_engine_set_level(&a->engine,a->level);
}

static void restore_active_game(App*a){
    if(!a||!a->saved.active_game)return;
    if(!sf_game_from_fen_history(&a->game,a->saved.active_fen[0]?a->saved.active_fen:0,a->saved.active_moves,-1)){a->saved.active_game=0;return;}
    a->game_mode=a->saved.active_mode;a->level=a->saved.active_level;a->human_side=a->saved.active_human_side;
    a->time_control=a->saved.active_time_control;a->flipped=a->saved.active_board_flipped;
    a->side_to_move=sf_side_to_move(&a->game);a->white_ms=a->saved.active_white_ms;a->black_ms=a->saved.active_black_ms;
    a->turn_started_ms=now_ms();a->game_active=1;a->game_over=0;a->result=RESULT_NONE;a->result_reason=REASON_NONE;
    clear_move_ui(a);update_last_move(a,&a->game);a->in_check=sf_is_in_check(&a->game,a->side_to_move);a->checked_king=sf_find_king(&a->game,a->side_to_move);
    set_status(a,"GAME RESTORED");
}

static void clock_commit(App*a){
    int64_t n,delta;if(!a||!a->time_control||a->game_over)return;n=now_ms();
    if(!a->turn_started_ms){a->turn_started_ms=n;return;}delta=n-a->turn_started_ms;if(delta<0)delta=0;
    if(a->side_to_move)a->white_ms-=delta;else a->black_ms-=delta;a->turn_started_ms=n;
    if(a->white_ms<=0){a->white_ms=0;a->result=RESULT_BLACK;a->result_reason=REASON_TIMEOUT;}
    else if(a->black_ms<=0){a->black_ms=0;a->result=RESULT_WHITE;a->result_reason=REASON_TIMEOUT;}
}
static int64_t display_clock(const App*a,int white){int64_t ms=white?a->white_ms:a->black_ms;if(a->time_control&&!a->game_over&&a->side_to_move==white&&a->turn_started_ms){int64_t d=now_ms()-a->turn_started_ms;if(d>0)ms-=d;}return ms<0?0:ms;}

static int profile_result(const App*a,int result){if(result==RESULT_DRAW)return 2;if(a->game_mode!=MODE_ENGINE)return 2;if((a->human_side==0&&result==RESULT_WHITE)||(a->human_side==1&&result==RESULT_BLACK))return 1;return 3;}
static void finish_game(App*a,int result,int reason,const char*label){
    CerHistory h;CerProfile*p;if(!a||a->game_over)return;
    a->game_over=1;a->game_active=0;a->result=result;a->result_reason=reason;set_status(a,label);
    memset(&h,0,sizeof(h));cer_copy_text(h.moves,sizeof(h.moves),a->game.history);cer_copy_text(h.start_fen,sizeof(h.start_fen),a->game.start_fen);
    h.profile_index=a->saved.active_profile;h.result=profile_result(a,result);h.result_reason=reason;h.mode=a->game_mode;h.level=a->level;h.human_side=a->human_side;
    h.move_count=sf_history_move_count(&a->game);h.white_ms=a->white_ms;h.black_ms=a->black_ms;cer_history_add(&a->saved,&h);
    p=cer_active_profile(&a->saved);if(p&&a->game_mode==MODE_ENGINE)cer_profile_record_result(p,h.result,a->level);
    a->overlay=OVERLAY_RESULT;a->saved.active_game=0;save_state(a);
    haptic(a,reason==REASON_MATE?16:(h.result==1?6:(h.result==3?17:1)));
}

static int refresh_game_status(App*a){
    int n;if(!a)return-1;a->side_to_move=sf_side_to_move(&a->game);a->in_check=sf_is_in_check(&a->game,a->side_to_move);a->checked_king=sf_find_king(&a->game,a->side_to_move);
    n=load_legal(a);if(n<0){set_status(a,"ENGINE UNAVAILABLE");return-1;}
    if(n==0){if(a->in_check)finish_game(a,a->side_to_move?RESULT_BLACK:RESULT_WHITE,REASON_MATE,"CHECKMATE");else finish_game(a,RESULT_DRAW,REASON_STALEMATE,"STALEMATE");return 0;}
    if(sf_is_insufficient_material(&a->game)){finish_game(a,RESULT_DRAW,REASON_MATERIAL,"INSUFFICIENT MATERIAL");return 0;}
    set_status(a,a->in_check?"CHECK":(current_side_is_human(a)?"YOUR MOVE":"STOCKFISH THINKING"));return n;
}

static void animate_move(App*a,int from,int to,char piece){int p,frames=a->saved.reduce_motion?1:7;a->anim_active=1;a->anim_from=from;a->anim_to=to;a->anim_piece=piece;for(p=0;p<=frames;++p){a->anim_progress=frames?100*p/frames:100;draw(a);if(p<frames)sleep_ms(a->saved.reduce_motion?1:16);}a->anim_active=0;}

static void apply_move_animated(App*a,const char move[6]){
    int from=sf_square_to_index(move),to=sf_square_to_index(move+2);char piece=a->game.board[from];
    clock_commit(a);if(a->result_reason==REASON_TIMEOUT){finish_game(a,a->result,REASON_TIMEOUT,"TIMEOUT");return;}
    if(!sf_apply_uci(&a->game,move)){set_status(a,"MOVE COULD NOT BE APPLIED");haptic(a,17);return;}
    a->last_from=from;a->last_to=to;clear_move_ui(a);animate_move(a,from,to,piece);a->side_to_move=sf_side_to_move(&a->game);a->turn_started_ms=now_ms();save_state(a);
}

static void*engine_worker_main(void*data){
    App*a=(App*)data;char signal=1;CerAnalysis analysis;int result=0;
    memset(&analysis,0,sizeof(analysis));
    if(a->worker_kind==WORK_MOVE)result=cer_engine_bestmove(&a->engine,&a->worker_game,a->worker_think_ms,a->worker_move);
    else result=cer_engine_analyze(&a->engine,&a->worker_game,a->worker_think_ms,&analysis);
    a->worker_analysis=analysis;a->worker_result=result;
    if(a->notify_pipe[1]>=0)(void)write(a->notify_pipe[1],&signal,1);
    return 0;
}

static int start_engine_worker(App*a,int kind,const SfGame*game,int think_ms){
    if(!a||!game||a->worker_busy||a->notify_pipe[1]<0)return 0;
    a->worker_kind=kind;a->worker_game=*game;a->worker_think_ms=think_ms;
    a->worker_position_hash=sf_board_hash(game);a->worker_result=0;a->worker_move[0]=0;
    memset(&a->worker_analysis,0,sizeof(a->worker_analysis));a->worker_busy=1;a->worker_joinable=1;
    if(pthread_create(&a->worker_thread,0,engine_worker_main,a)!=0){a->worker_busy=0;a->worker_joinable=0;a->worker_kind=WORK_NONE;return 0;}
    return 1;
}

static void engine_turn(App*a){
    if(!a||a->game_over||a->game_mode!=MODE_ENGINE||current_side_is_human(a))return;
    if(a->worker_busy)return;
    configure_engine(a);
    if(!a->engine.ready){finish_game(a,RESULT_ENGINE,REASON_ENGINE,"ENGINE STOPPED");draw(a);return;}
    set_status(a,"STOCKFISH THINKING");draw(a);
    if(!start_engine_worker(a,WORK_MOVE,&a->game,level_think_ms(a->level))){set_status(a,"ENGINE WORKER FAILED");set_toast(a,"COULD NOT START STOCKFISH",2);draw(a);}
}

static void start_game(App*a,int mode){
    int64_t start_ms;if(!a)return;a->setup_mode=mode;a->game_mode=mode;a->level=a->setup_level;a->time_control=a->setup_time;
    a->human_side=mode==MODE_LOCAL?2:(a->setup_side==2?(a->saved.next_sequence&1):a->setup_side);a->flipped=a->human_side==1;
    sf_game_reset(&a->game);a->side_to_move=1;a->game_active=1;a->game_over=0;a->result=RESULT_NONE;a->result_reason=REASON_NONE;a->overlay=OVERLAY_NONE;
    a->in_check=0;a->checked_king=-1;a->last_from=a->last_to=-1;clear_move_ui(a);
    start_ms=a->time_control==0?0:(int64_t)(a->time_control==1?3:(a->time_control==2?5:10))*60*1000;a->white_ms=a->black_ms=start_ms;a->turn_started_ms=now_ms();
    configure_engine(a);if(!a->engine.ready&&mode==MODE_ENGINE){a->game_mode=MODE_LOCAL;a->human_side=2;set_toast(a,"STOCKFISH UNAVAILABLE - PASS & PLAY",1);}
    a->screen=SCR_GAME;a->tab=TAB_PLAY;refresh_game_status(a);save_state(a);draw(a);if(a->game_mode==MODE_ENGINE&&!current_side_is_human(a))engine_turn(a);
}

static void start_puzzle(App*a,int index){
    const Puzzle*p;if(!a)return;index=i_clamp(index,0,PUZZLE_COUNT-1);p=&PUZZLES[index];a->puzzle_index=index;a->puzzle_complete=0;
    a->game_mode=MODE_PUZZLE;a->level=2;a->time_control=0;a->human_side=2;a->game_active=0;a->game_over=0;a->result=RESULT_NONE;a->result_reason=REASON_NONE;a->overlay=OVERLAY_NONE;
    clear_move_ui(a);if(!sf_game_load_fen(&a->game,p->fen)){sf_game_reset(&a->game);set_toast(a,"PUZZLE DATA ERROR",2);}
    a->side_to_move=sf_side_to_move(&a->game);a->flipped=!a->side_to_move;a->last_from=a->last_to=-1;a->in_check=sf_is_in_check(&a->game,a->side_to_move);a->checked_king=sf_find_king(&a->game,a->side_to_move);
    configure_engine(a);load_legal(a);set_status(a,"FIND THE BEST MOVE");a->screen=SCR_PUZZLE_GAME;draw(a);
}

static void puzzle_move(App*a,const char move[6]){
    CerProfile*p;if(!s_eq(move,PUZZLES[a->puzzle_index].solution)){set_status(a,"TRY AGAIN");set_toast(a,"THAT MOVE MISSES THE TACTIC",2);haptic(a,17);clear_move_ui(a);load_legal(a);draw(a);return;}
    apply_move_animated(a,move);a->puzzle_complete=1;a->saved.puzzle_solved_mask|=1<<a->puzzle_index;p=cer_active_profile(&a->saved);
    if(p){++p->puzzles_solved;++p->puzzle_streak;if(p->puzzle_streak>p->best_puzzle_streak)p->best_puzzle_streak=p->puzzle_streak;}
