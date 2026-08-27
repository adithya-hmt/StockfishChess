    if(a->screen==SCR_REPLAY){if(id==HIT_REPLAY_PREV){replay_load(a,a->history_index,a->replay_ply-1);draw(a);}else if(id==HIT_REPLAY_NEXT){replay_load(a,a->history_index,a->replay_ply+1);draw(a);}else if(id==HIT_REPLAY_FLIP){a->flipped=!a->flipped;draw(a);}else if(id==HIT_REPLAY_ANALYZE){a->previous_screen=SCR_REPLAY;open_analysis(a,&a->view_game,-1);}return;}
    if(a->screen==SCR_ANALYSIS){if(id==HIT_ANALYZE_RUN)run_analysis(a);else if(id==HIT_ANALYZE_PREV){--a->analysis_ply;analysis_rebuild(a);}else if(id==HIT_ANALYZE_NEXT){++a->analysis_ply;analysis_rebuild(a);}else if(id==HIT_ANALYZE_FLIP){a->flipped=!a->flipped;draw(a);}return;}
    if(a->screen==SCR_PROFILE){if(id==HIT_PROFILE_SETTINGS){a->previous_screen=SCR_PROFILE;a->screen=SCR_SETTINGS;draw(a);}else if(id==HIT_PROFILE_SWITCH){a->overlay=OVERLAY_PROFILE_PICKER;draw(a);}else if(id==HIT_PROFILE_ADD){if(a->saved.profile_count>=CER_MAX_PROFILES)set_toast(a,"PROFILE LIMIT REACHED",1);else{a->profile_name[0]=0;a->profile_avatar=a->saved.profile_count%5;a->profile_accent=a->saved.profile_count%5;a->screen=SCR_PROFILE_CREATE;}draw(a);}else if(id==HIT_PROFILE_LOCK){if(a->saved.pin_enabled){a->saved.pin_enabled=0;a->saved.pin_hash=0;save_state(a);set_toast(a,"LOCAL LOCK DISABLED",1);draw(a);}else{a->pin_stage=0;pin_clear(a);a->screen=SCR_PIN;draw(a);}}return;}
    if(a->screen==SCR_SETTINGS){if(id==HIT_SETTING_HAPTICS)a->saved.haptics_enabled=!a->saved.haptics_enabled;else if(id==HIT_SETTING_LEGAL)a->saved.legal_moves_enabled=!a->saved.legal_moves_enabled;else if(id==HIT_SETTING_COORDS)a->saved.coordinates_enabled=!a->saved.coordinates_enabled;else if(id==HIT_SETTING_AUTO_QUEEN)a->saved.auto_queen=!a->saved.auto_queen;else if(id==HIT_SETTING_MOTION)a->saved.reduce_motion=!a->saved.reduce_motion;else if(id==HIT_SETTING_BOARD)a->saved.board_theme=(a->saved.board_theme+1)%3;else if(id==HIT_SETTING_PIECES)set_toast(a,"FRAMILTON STAUNTON IS ACTIVE",0);else if(id==HIT_SETTING_RESET){a->overlay=OVERLAY_RESET;draw(a);return;}save_state(a);haptic(a,1);draw(a);return;}
}

static void handle_tap(App*a,int x,int y){int id;if(!a)return;id=hit_at(a,x,y);if(id!=HIT_NONE){handle_hit(a,id);return;}if(a->overlay==OVERLAY_NONE&&a->board_interactive&&(a->screen==SCR_GAME||a->screen==SCR_PUZZLE_GAME)){int idx=board_index_from_point(a,x,y);if(idx>=0)select_square(a,idx);}}
static void motion_down(App*a,int x,int y){int idx;if(!a||a->overlay!=OVERLAY_NONE||!a->board_interactive||!(a->screen==SCR_GAME||a->screen==SCR_PUZZLE_GAME)||a->game_over||!current_side_is_human(a))return;idx=board_index_from_point(a,x,y);if(idx<0)return;if(a->legal_count<=0)load_legal(a);if(side_piece(a->game.board[idx],a->side_to_move)&&source_has_move(a,idx)){a->selected=idx;a->dragging=1;a->drag_from=idx;a->drag_x=x;a->drag_y=y;a->drag_start_x=x;a->drag_start_y=y;a->drag_moved=0;set_status(a,"PIECE SELECTED");haptic(a,1);draw(a);}}
static void motion_move(App*a,int x,int y){if(!a||!a->dragging)return;a->drag_x=x;a->drag_y=y;if(i_abs(x-a->drag_start_x)+i_abs(y-a->drag_start_y)>px(a->window?ANativeWindow_getWidth(a->window):360,10))a->drag_moved=1;draw(a);}
static void motion_up(App*a,int x,int y){if(!a)return;if(a->dragging){int moved=a->drag_moved,from=a->drag_from,to=board_index_from_point(a,x,y);a->dragging=0;a->drag_moved=0;a->selected=from;if(moved&&to>=0&&to!=from)user_move_to(a,to,0);else select_square(a,from);draw(a);return;}handle_tap(a,x,y);}


static int worker_callback(int fd,int events,void*data){
    App*a=(App*)data;char signal[16];int kind,result;uint64_t hash;(void)events;(void)read(fd,signal,sizeof(signal));if(!a)return 1;
    kind=a->worker_kind;result=a->worker_result;hash=a->worker_position_hash;
    if(a->worker_joinable){(void)pthread_join(a->worker_thread,0);a->worker_joinable=0;}
    a->worker_busy=0;a->worker_kind=WORK_NONE;
    if(kind==WORK_MOVE){
        if(hash!=sf_board_hash(&a->game)||a->game_over||a->game_mode!=MODE_ENGINE||current_side_is_human(a)){draw(a);return 1;}
        if(result==1){a->engine_error=0;apply_move_animated(a,a->worker_move);if(!a->game_over)refresh_game_status(a);haptic(a,1);}
        else if(result==-1)refresh_game_status(a);
        else if(!a->engine_error){a->engine_error=1;set_status(a,"RESTARTING STOCKFISH");draw(a);cer_engine_stop(&a->engine);cer_engine_init(&a->engine);configure_engine(a);a->engine_error=1;if(a->engine.ready&&start_engine_worker(a,WORK_MOVE,&a->game,400))return 1;finish_game(a,RESULT_ENGINE,REASON_ENGINE,"ENGINE STOPPED");}
        else finish_game(a,RESULT_ENGINE,REASON_ENGINE,"ENGINE STOPPED");
    }else if(kind==WORK_HINT){
        if(hash==sf_board_hash(&a->game)&&result==1){a->hint_from=sf_square_to_index(a->worker_analysis.bestmove);a->hint_to=sf_square_to_index(a->worker_analysis.bestmove+2);set_status(a,"HINT READY");set_toast(a,"GREEN ARROW SHOWS THE IDEA",0);haptic(a,6);}else if(hash==sf_board_hash(&a->game))set_toast(a,"HINT FAILED",2);
    }else if(kind==WORK_ANALYSIS){
        if(a->screen==SCR_ANALYSIS&&hash==sf_board_hash(&a->view_game)){a->analysis=a->worker_analysis;a->analysis_ready=result==1;set_status(a,result==1?"ANALYSIS READY":"NO LEGAL MOVE");haptic(a,result==1?6:17);}
    }
    draw(a);return 1;
}
static void worker_init(App*a){
    if(!a)return;a->notify_pipe[0]=a->notify_pipe[1]=-1;a->worker_busy=0;a->worker_joinable=0;a->worker_kind=WORK_NONE;
    if(!a->looper)a->looper=ALooper_forThread();if(!a->looper)a->looper=ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    if(!a->looper||pipe(a->notify_pipe)!=0)return;
    if(ALooper_addFd(a->looper,a->notify_pipe[0],4,POLLIN,worker_callback,a)<0){close(a->notify_pipe[0]);close(a->notify_pipe[1]);a->notify_pipe[0]=a->notify_pipe[1]=-1;}
}
static void worker_stop(App*a){
    if(!a)return;if(a->worker_joinable){(void)pthread_join(a->worker_thread,0);a->worker_joinable=0;}a->worker_busy=0;a->worker_kind=WORK_NONE;
    if(a->looper&&a->notify_pipe[0]>=0)ALooper_removeFd(a->looper,a->notify_pipe[0]);if(a->notify_pipe[0]>=0)close(a->notify_pipe[0]);if(a->notify_pipe[1]>=0)close(a->notify_pipe[1]);a->notify_pipe[0]=a->notify_pipe[1]=-1;
}

static int timer_callback(int fd,int events,void*data){
    App*a=(App*)data;char tick[16];(void)events;(void)read(fd,tick,sizeof(tick));
    if(!a)return 1;
    if(a->time_control&&a->game_active&&!a->game_over){
        clock_commit(a);
        if(a->result_reason==REASON_TIMEOUT)finish_game(a,a->result,REASON_TIMEOUT,"TIMEOUT");
        draw(a);
    }else if(a->toast[0]&&now_ms()>=a->toast_until_ms){a->toast[0]=0;draw(a);}
    return 1;
}
static void*timer_thread_main(void*data){App*a=(App*)data;char tick=1;while(a&&a->timer_running){sleep_ms(250);if(a->timer_running&&a->timer_pipe[1]>=0)(void)write(a->timer_pipe[1],&tick,1);}return 0;}
static void timer_start(App*a){if(!a)return;a->timer_pipe[0]=a->timer_pipe[1]=-1;a->timer_thread=0;a->timer_running=0;if(!a->looper)a->looper=ALooper_forThread();if(!a->looper)a->looper=ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);if(!a->looper||pipe(a->timer_pipe)!=0)return;if(ALooper_addFd(a->looper,a->timer_pipe[0],3,POLLIN,timer_callback,a)<0){close(a->timer_pipe[0]);close(a->timer_pipe[1]);a->timer_pipe[0]=a->timer_pipe[1]=-1;return;}a->timer_running=1;if(pthread_create(&a->timer_thread,0,timer_thread_main,data)!=0){a->timer_running=0;ALooper_removeFd(a->looper,a->timer_pipe[0]);close(a->timer_pipe[0]);close(a->timer_pipe[1]);a->timer_pipe[0]=a->timer_pipe[1]=-1;a->timer_thread=0;}}
static void timer_stop(App*a){if(!a)return;a->timer_running=0;if(a->timer_thread)(void)pthread_join(a->timer_thread,0);if(a->looper&&a->timer_pipe[0]>=0)ALooper_removeFd(a->looper,a->timer_pipe[0]);if(a->timer_pipe[0]>=0)close(a->timer_pipe[0]);if(a->timer_pipe[1]>=0)close(a->timer_pipe[1]);a->timer_pipe[0]=a->timer_pipe[1]=-1;a->timer_thread=0;}

/* ------------------------------ Android lifecycle ------------------------------ */
static int input_callback(int fd,int events,void*data){
    App*a=(App*)data;AInputEvent*ev;(void)fd;(void)events;if(!a||!a->input)return 1;
    while(AInputQueue_getEvent(a->input,&ev)>=0){int handled=0;if(AInputQueue_preDispatchEvent(a->input,ev))continue;if(AInputEvent_getType(ev)==AINPUT_EVENT_TYPE_MOTION){int action=AMotionEvent_getAction(ev)&AMOTION_EVENT_ACTION_MASK;int x=(int)AMotionEvent_getX(ev,0),y=(int)AMotionEvent_getY(ev,0);if(action==AMOTION_EVENT_ACTION_DOWN)motion_down(a,x,y);else if(action==AMOTION_EVENT_ACTION_MOVE)motion_move(a,x,y);else if(action==AMOTION_EVENT_ACTION_UP)motion_up(a,x,y);else if(action==AMOTION_EVENT_ACTION_CANCEL){a->dragging=0;draw(a);}handled=1;}AInputQueue_finishEvent(a->input,ev,handled);}return 1;
}
static void choose_start_screen(App*a){if(!a->saved.onboarding_done)a->screen=SCR_ONBOARD;else if(a->saved.active_profile<0||a->saved.profile_count<=0)a->screen=SCR_PROFILE_CREATE;else if(a->saved.pin_enabled)a->screen=SCR_UNLOCK;else a->screen=SCR_HOME;}
static void on_start(ANativeActivity*activity){draw((App*)activity->instance);}
static void on_resume(ANativeActivity*activity){App*a=(App*)activity->instance;if(a)a->turn_started_ms=now_ms();draw(a);}
static void on_pause(ANativeActivity*activity){App*a=(App*)activity->instance;if(a){clock_commit(a);if(a->result_reason==REASON_TIMEOUT&&!a->game_over)finish_game(a,a->result,REASON_TIMEOUT,"TIMEOUT");save_state(a);}}
static void on_stop(ANativeActivity*activity){App*a=(App*)activity->instance;if(a)save_state(a);}
static void on_window_created(ANativeActivity*activity,ANativeWindow*window){App*a=(App*)activity->instance;a->window=window;ANativeWindow_setBuffersGeometry(window,0,0,WINDOW_FORMAT_RGBA_8888);a->screen=SCR_SPLASH;draw(a);sleep_ms(a->saved.reduce_motion?80:430);choose_start_screen(a);draw(a);}
static void on_window_resized(ANativeActivity*activity,ANativeWindow*window){(void)window;draw((App*)activity->instance);}
static void on_window_redraw(ANativeActivity*activity,ANativeWindow*window){(void)window;draw((App*)activity->instance);}
static void on_window_destroyed(ANativeActivity*activity,ANativeWindow*window){App*a=(App*)activity->instance;if(a&&a->window==window)a->window=0;}
static void on_input_created(ANativeActivity*activity,AInputQueue*q){App*a=(App*)activity->instance;a->input=q;a->looper=ALooper_forThread();if(!a->looper)a->looper=ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);if(a->looper)AInputQueue_attachLooper(q,a->looper,1,input_callback,a);}
static void on_input_destroyed(ANativeActivity*activity,AInputQueue*q){App*a=(App*)activity->instance;if(a&&a->input==q){AInputQueue_detachLooper(q);a->input=0;}}
static void on_destroy(ANativeActivity*activity){App*a=(App*)activity->instance;if(a){timer_stop(a);worker_stop(a);clock_commit(a);save_state(a);cer_engine_stop(&a->engine);activity->instance=0;}}


#ifdef SF_UI_PREVIEW
static void preview_seed(App*a){
    memset(a,0,sizeof(*a));cer_state_defaults(&a->saved);cer_profile_add(&a->saved,"WHITE KNIGHT",2,0);a->saved.onboarding_done=1;a->setup_mode=MODE_ENGINE;a->setup_side=0;a->setup_level=1;a->setup_time=2;a->level=1;a->human_side=0;a->game_mode=MODE_ENGINE;a->time_control=2;a->white_ms=287000;a->black_ms=300000;a->side_to_move=1;a->selected=-1;a->hint_from=a->hint_to=-1;a->last_from=a->last_to=-1;a->history_index=0;sf_game_reset(&a->game);sf_apply_uci(&a->game,"e2e4");sf_apply_uci(&a->game,"e7e5");sf_apply_uci(&a->game,"g1f3");sf_apply_uci(&a->game,"b8c6");a->side_to_move=sf_side_to_move(&a->game);update_last_move(a,&a->game);set_status(a,"YOUR MOVE");a->game_active=1;a->tab=TAB_HOME;
    a->saved.profiles[0].rating=1482;a->saved.profiles[0].games=27;a->saved.profiles[0].wins=16;a->saved.profiles[0].losses=8;a->saved.profiles[0].draws=3;a->saved.profiles[0].puzzles_solved=12;a->saved.profiles[0].puzzle_streak=4;a->saved.profiles[0].best_puzzle_streak=7;
    {CerHistory h;memset(&h,0,sizeof(h));cer_copy_text(h.moves,sizeof(h.moves),a->game.history);h.result=1;h.mode=MODE_ENGINE;h.level=1;h.human_side=0;h.move_count=4;cer_history_add(&a->saved,&h);}
}

__attribute__((visibility("default")))
void sf_render_preview_screen(uint32_t*pixels,int width,int height,int stride,int screen){App a;CerCanvas c;preview_seed(&a);a.screen=screen;if(screen==SCR_GAME){a.flipped=0;a.tab=TAB_PLAY;}else if(screen==SCR_PUZZLES)a.tab=TAB_PUZZLES;else if(screen==SCR_HISTORY)a.tab=TAB_HISTORY;else if(screen==SCR_PROFILE)a.tab=TAB_PROFILE;else if(screen==SCR_SETTINGS)a.previous_screen=SCR_PROFILE;else if(screen==SCR_PUZZLE_GAME){a.puzzle_index=1;sf_game_load_fen(&a.game,PUZZLES[1].fen);a.side_to_move=sf_side_to_move(&a.game);a.flipped=!a.side_to_move;set_status(&a,"FIND THE BEST MOVE");}else if(screen==99){a.screen=SCR_GAME;a.flipped=0;sf_game_load_fen(&a.game,"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");a.side_to_move=1;a.game_over=1;a.game_active=0;a.result=RESULT_BLACK;a.result_reason=REASON_MATE;a.in_check=1;a.checked_king=sf_find_king(&a.game,1);a.overlay=OVERLAY_RESULT;set_status(&a,"CHECKMATE");}c.width=width;c.height=height;c.stride=stride;c.bits=pixels;render_screen(&a,&c);}
__attribute__((visibility("default")))
void sf_render_preview(uint32_t*pixels,int width,int height,int stride){sf_render_preview_screen(pixels,width,height,stride,SCR_HOME);}
#endif
