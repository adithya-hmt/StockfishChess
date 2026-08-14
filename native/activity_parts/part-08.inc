
/* ------------------------------ Interaction ------------------------------ */
static void user_move_to(App*a,int to,char promotion){
    char move[6];int from;if(!a||a->selected<0)return;from=a->selected;
    if(promotion_count(a,from,to)>1&&!promotion&&!a->saved.auto_queen){a->pending_promo=1;a->promo_from=from;a->promo_to=to;a->overlay=OVERLAY_PROMOTION;draw(a);return;}
    if(!pick_move(a,from,to,promotion,move)){if(side_piece(a->game.board[to],a->side_to_move)&&source_has_move(a,to)){a->selected=to;set_status(a,"PIECE SELECTED");haptic(a,1);}else{set_status(a,"ILLEGAL MOVE");haptic(a,17);}draw(a);return;}
    if(a->game_mode==MODE_PUZZLE){puzzle_move(a,move);return;}
    apply_move_animated(a,move);if(a->game_over){draw(a);return;}if(refresh_game_status(a)>0&&a->game_mode==MODE_ENGINE&&!current_side_is_human(a))engine_turn(a);draw(a);
}
static void select_square(App*a,int idx){
    int n;if(!a||a->game_over||!current_side_is_human(a)||idx<0||idx>=64)return;
    if(a->selected<0){n=a->legal_count>0?a->legal_count:load_legal(a);if(n<=0){refresh_game_status(a);draw(a);return;}if(side_piece(a->game.board[idx],a->side_to_move)&&source_has_move(a,idx)){a->selected=idx;a->hint_from=a->hint_to=-1;set_status(a,"PIECE SELECTED");haptic(a,1);}else haptic(a,17);draw(a);return;}
    if(idx==a->selected){a->selected=-1;set_status(a,"YOUR MOVE");haptic(a,1);draw(a);return;}user_move_to(a,idx,0);
}
static void apply_promotion(App*a,char piece){char move[6];if(!a||!a->pending_promo)return;if(pick_move(a,a->promo_from,a->promo_to,piece,move)){a->overlay=OVERLAY_NONE;a->selected=a->promo_from;if(a->game_mode==MODE_PUZZLE)puzzle_move(a,move);else{apply_move_animated(a,move);if(!a->game_over&&refresh_game_status(a)>0&&a->game_mode==MODE_ENGINE&&!current_side_is_human(a))engine_turn(a);draw(a);}}else{a->overlay=OVERLAY_NONE;set_toast(a,"PROMOTION FAILED",2);draw(a);}}

static void set_page(App*a,int tab){a->tab=tab;a->overlay=OVERLAY_NONE;if(tab==TAB_HOME)a->screen=SCR_HOME;else if(tab==TAB_PLAY)a->screen=SCR_PLAY;else if(tab==TAB_PUZZLES)a->screen=SCR_PUZZLES;else if(tab==TAB_HISTORY)a->screen=SCR_HISTORY;else a->screen=SCR_PROFILE;draw(a);haptic(a,1);}
static void go_home(App*a){a->overlay=OVERLAY_NONE;a->screen=SCR_HOME;a->tab=TAB_HOME;save_state(a);draw(a);}
static void go_back(App*a){
    a->overlay=OVERLAY_NONE;
    if(a->screen==SCR_SETTINGS){a->screen=a->previous_screen?a->previous_screen:SCR_PROFILE;}
    else if(a->screen==SCR_GAME){a->overlay=OVERLAY_GAME_MENU;}
    else if(a->screen==SCR_PUZZLE_GAME)a->screen=SCR_PUZZLES;
    else if(a->screen==SCR_REPLAY)a->screen=SCR_HISTORY;
    else if(a->screen==SCR_ANALYSIS)a->screen=a->previous_screen?a->previous_screen:SCR_HOME;
    else if(a->screen==SCR_PLAY||a->screen==SCR_PUZZLES||a->screen==SCR_HISTORY||a->screen==SCR_PROFILE)a->screen=SCR_HOME;
    else if(a->screen==SCR_PROFILE_CREATE&&a->saved.profile_count>0)a->screen=SCR_PROFILE;
    else if(a->screen==SCR_PIN)a->screen=SCR_PROFILE;
    else a->screen=SCR_HOME;
    draw(a);
}

static void profile_create_finish(App*a){
    if(!a->profile_name[0])cer_copy_text(a->profile_name,sizeof(a->profile_name),"WHITE KNIGHT");
    if(cer_profile_add(&a->saved,a->profile_name,a->profile_avatar,a->profile_accent)<0){set_toast(a,"PROFILE LIMIT REACHED",2);a->screen=SCR_PROFILE;draw(a);return;}
    a->saved.onboarding_done=1;save_state(a);a->pin_digits=0;a->pin_stage=0;a->pin_entry[0]=0;a->pin_first[0]=0;a->screen=SCR_PIN;draw(a);haptic(a,16);
}
static void profile_name_key(App*a,int row,int col){char ch;size_t n;if(row<0||row>2||col<0||col>9)return;ch=KEY_ROWS[row][col];if(!ch)return;n=s_len(a->profile_name);if(n+1<sizeof(a->profile_name)){a->profile_name[n]=ch;a->profile_name[n+1]=0;haptic(a,1);}draw(a);}
static void name_delete(App*a){size_t n=s_len(a->profile_name);if(n)a->profile_name[n-1]=0;haptic(a,1);draw(a);}

static void pin_clear(App*a){a->pin_digits=0;a->pin_entry[0]=0;draw(a);}
static void pin_digit(App*a,int digit,int unlock){
    if(a->pin_digits>=4)return;a->pin_entry[a->pin_digits++]=(char)('0'+digit);a->pin_entry[a->pin_digits]=0;haptic(a,1);draw(a);if(a->pin_digits<4)return;
    sleep_ms(90);
    if(unlock){if(cer_pin_hash(a->pin_entry)==a->saved.pin_hash){pin_clear(a);a->screen=SCR_HOME;set_toast(a,"PROFILE UNLOCKED",0);haptic(a,16);draw(a);}else{pin_clear(a);set_toast(a,"WRONG PIN",2);haptic(a,17);draw(a);}return;}
    if(a->pin_stage==0){cer_copy_text(a->pin_first,sizeof(a->pin_first),a->pin_entry);a->pin_stage=1;pin_clear(a);set_toast(a,"ENTER IT ONCE MORE",0);draw(a);}
    else if(s_eq(a->pin_first,a->pin_entry)){a->saved.pin_enabled=1;a->saved.pin_hash=cer_pin_hash(a->pin_entry);a->pin_stage=0;pin_clear(a);save_state(a);a->screen=SCR_HOME;set_toast(a,"LOCAL LOCK ENABLED",0);haptic(a,16);draw(a);}
    else{a->pin_stage=0;a->pin_first[0]=0;pin_clear(a);set_toast(a,"PINS DID NOT MATCH",2);haptic(a,17);draw(a);}
}

static void resume_saved_game(App*a){
    if(!a->saved.active_game){a->screen=SCR_PLAY;a->tab=TAB_PLAY;draw(a);return;}
    restore_active_game(a);configure_engine(a);a->screen=SCR_GAME;a->tab=TAB_PLAY;refresh_game_status(a);draw(a);if(a->game_mode==MODE_ENGINE&&!current_side_is_human(a))engine_turn(a);
}
static int analysis_total(const App*a){if(a->previous_screen==SCR_REPLAY&&a->history_index>=0&&a->history_index<a->saved.history_count)return a->saved.history[a->history_index].move_count;return sf_history_move_count(&a->game);}
static void analysis_rebuild(App*a){
    const char*fen=0,*moves=0;int total=analysis_total(a);a->analysis_ply=i_clamp(a->analysis_ply,0,total);
    if(a->previous_screen==SCR_REPLAY&&a->history_index>=0&&a->history_index<a->saved.history_count){CerHistory*h=&a->saved.history[a->history_index];fen=h->start_fen[0]?h->start_fen:0;moves=h->moves;}
    else{fen=a->game.start_fen[0]?a->game.start_fen:0;moves=a->game.history;}
    sf_game_from_fen_history(&a->view_game,fen,moves,a->analysis_ply);update_last_move(a,&a->view_game);a->analysis_ready=0;memset(&a->analysis,0,sizeof(a->analysis));draw(a);
}

static void reset_all_local_data(App*a){cer_state_defaults(&a->saved);cer_state_save(a->data_dir,&a->saved);a->game_active=0;a->overlay=OVERLAY_NONE;a->onboarding_page=0;a->profile_name[0]=0;a->screen=SCR_ONBOARD;set_toast(a,"LOCAL DATA RESET",1);draw(a);}

static void handle_overlay_hit(App*a,int id){
    if(id==HIT_CANCEL){a->overlay=OVERLAY_NONE;draw(a);return;}
    if(a->overlay==OVERLAY_PROMOTION){if(id==HIT_PROMO_Q)apply_promotion(a,'q');else if(id==HIT_PROMO_R)apply_promotion(a,'r');else if(id==HIT_PROMO_B)apply_promotion(a,'b');else if(id==HIT_PROMO_N)apply_promotion(a,'n');return;}
    if(a->overlay==OVERLAY_CONFIRM_RESIGN){if(id==HIT_CONFIRM_RESIGN){a->overlay=OVERLAY_NONE;resign_game(a);}return;}
    if(a->overlay==OVERLAY_RESET){if(id==HIT_SETTING_RESET)reset_all_local_data(a);return;}
    if(a->overlay==OVERLAY_PROFILE_PICKER){if(id>=HIT_PROFILE_BASE&&id<HIT_PROFILE_BASE+a->saved.profile_count){a->saved.active_profile=id-HIT_PROFILE_BASE;a->overlay=OVERLAY_NONE;save_state(a);set_toast(a,"PROFILE SWITCHED",0);draw(a);}return;}
    if(a->overlay==OVERLAY_GAME_MENU){if(id==HIT_GAME_RESTART){a->overlay=OVERLAY_NONE;restart_game(a);}else if(id==HIT_GAME_EXPORT){a->overlay=OVERLAY_NONE;{int ok=write_export(a,&a->game);set_toast(a,ok?"EXPORTED TO APP FILES":"EXPORT FAILED",ok?0:2);}draw(a);}else if(id==HIT_GAME_ANALYSIS){a->overlay=OVERLAY_NONE;open_analysis(a,&a->game,-1);}else if(id==HIT_GAME_RESIGN){a->overlay=OVERLAY_CONFIRM_RESIGN;draw(a);}return;}
    if(a->overlay==OVERLAY_RESULT){if(id==HIT_GAME_REMATCH){a->overlay=OVERLAY_NONE;restart_game(a);}else if(id==HIT_GAME_ANALYSIS){a->overlay=OVERLAY_NONE;open_analysis(a,&a->game,-1);}else if(id==HIT_GAME_EXPORT){int ok=write_export(a,&a->game);set_toast(a,ok?"EXPORTED TO APP FILES":"EXPORT FAILED",ok?0:2);draw(a);}else if(id==HIT_GAME_HOME)go_home(a);return;}
}

static void handle_hit(App*a,int id){
    if(!a)return;if(a->overlay!=OVERLAY_NONE){handle_overlay_hit(a,id);return;}
    if(id==HIT_BACK){go_back(a);return;}
    if(id>=HIT_NAV_BASE&&id<HIT_NAV_BASE+5){set_page(a,id-HIT_NAV_BASE);return;}
    if(a->screen==SCR_ONBOARD){if(id==HIT_SKIP){a->onboarding_page=2;a->screen=SCR_PROFILE_CREATE;draw(a);}else if(id==HIT_NEXT){if(a->onboarding_page<2){++a->onboarding_page;draw(a);}else{a->screen=SCR_PROFILE_CREATE;draw(a);}}return;}
    if(a->screen==SCR_PROFILE_CREATE){if(id==HIT_CONTINUE)profile_create_finish(a);else if(id==HIT_PIN_DELETE)name_delete(a);else if(id>=HIT_KEY_BASE&&id<HIT_KEY_BASE+30)profile_name_key(a,(id-HIT_KEY_BASE)/10,(id-HIT_KEY_BASE)%10);else if(id>=HIT_AVATAR_BASE&&id<HIT_AVATAR_BASE+5){a->profile_avatar=id-HIT_AVATAR_BASE;haptic(a,1);draw(a);}else if(id>=HIT_AVATAR_BASE+10&&id<HIT_AVATAR_BASE+15){a->profile_accent=id-(HIT_AVATAR_BASE+10);haptic(a,1);draw(a);}return;}
    if(a->screen==SCR_PIN||a->screen==SCR_UNLOCK){int unlock=a->screen==SCR_UNLOCK;if(id==HIT_PIN_DELETE)pin_clear(a);else if(id>=HIT_PIN_BASE&&id<=HIT_PIN_BASE+9)pin_digit(a,id-HIT_PIN_BASE,unlock);else if(id==HIT_PIN_SKIP){a->saved.pin_enabled=0;save_state(a);a->screen=SCR_HOME;draw(a);}return;}
    if(a->screen==SCR_HOME){if(id==HIT_PROFILE_SETTINGS){a->previous_screen=SCR_HOME;a->screen=SCR_SETTINGS;draw(a);}else if(id==HIT_HOME_HERO)resume_saved_game(a);else if(id==HIT_HOME_STOCKFISH){a->setup_mode=MODE_ENGINE;a->screen=SCR_PLAY;a->tab=TAB_PLAY;draw(a);}else if(id==HIT_HOME_LOCAL){a->setup_mode=MODE_LOCAL;a->screen=SCR_PLAY;a->tab=TAB_PLAY;draw(a);}else if(id==HIT_HOME_PUZZLES){a->screen=SCR_PUZZLES;a->tab=TAB_PUZZLES;draw(a);}return;}
    if(a->screen==SCR_PLAY){if(id==HIT_PROFILE_SETTINGS){a->previous_screen=SCR_PLAY;a->screen=SCR_SETTINGS;draw(a);}else if(id==HIT_MODE_ENGINE){a->setup_mode=MODE_ENGINE;draw(a);}else if(id==HIT_MODE_LOCAL){a->setup_mode=MODE_LOCAL;draw(a);}else if(id==HIT_SIDE_WHITE){a->setup_side=0;draw(a);}else if(id==HIT_SIDE_BLACK){a->setup_side=1;draw(a);}else if(id==HIT_SIDE_RANDOM){a->setup_side=2;draw(a);}else if(id==HIT_LEVEL_EASY){a->setup_level=0;draw(a);}else if(id==HIT_LEVEL_MEDIUM){a->setup_level=1;draw(a);}else if(id==HIT_LEVEL_HARD){a->setup_level=2;draw(a);}else if(id>=HIT_TIME_NONE&&id<=HIT_TIME_10){a->setup_time=id-HIT_TIME_NONE;draw(a);}else if(id==HIT_TOGGLE_LEGAL){a->saved.legal_moves_enabled=!a->saved.legal_moves_enabled;save_state(a);draw(a);}else if(id==HIT_START_GAME)start_game(a,a->setup_mode);return;}
    if(a->screen==SCR_GAME||a->screen==SCR_PUZZLE_GAME){if(id==HIT_GAME_UNDO)undo_game(a);else if(id==HIT_GAME_HINT)show_hint(a);else if(id==HIT_GAME_FLIP){a->flipped=!a->flipped;save_state(a);haptic(a,1);draw(a);}else if(id==HIT_GAME_ANALYSIS)open_analysis(a,&a->game,-1);else if(id==HIT_GAME_MENU){a->overlay=OVERLAY_GAME_MENU;draw(a);}return;}
    if(a->screen==SCR_PUZZLES){if(id==HIT_PROFILE_SETTINGS){a->previous_screen=SCR_PUZZLES;a->screen=SCR_SETTINGS;draw(a);}else if(id>=HIT_PUZZLE_BASE&&id<HIT_PUZZLE_BASE+PUZZLE_COUNT)start_puzzle(a,id-HIT_PUZZLE_BASE);return;}
    if(a->screen==SCR_HISTORY){if(id==HIT_PROFILE_SETTINGS){a->previous_screen=SCR_HISTORY;a->screen=SCR_SETTINGS;draw(a);}else if(id>=HIT_HISTORY_BASE&&id<HIT_HISTORY_BASE+a->saved.history_count){a->history_index=id-HIT_HISTORY_BASE;replay_load(a,a->history_index,a->saved.history[a->history_index].move_count);a->screen=SCR_REPLAY;draw(a);}return;}
