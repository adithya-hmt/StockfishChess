    set_status(a,"PUZZLE SOLVED");set_toast(a,"CLEAN TACTIC",0);save_state(a);haptic(a,16);draw(a);
}

static void show_hint(App*a){
    if(!a||a->game_over||!current_side_is_human(a)){set_toast(a,"HINT IS NOT AVAILABLE",1);draw(a);return;}
    if(a->worker_busy){set_toast(a,"STOCKFISH IS ALREADY CALCULATING",1);draw(a);return;}
    configure_engine(a);if(!a->engine.ready){set_toast(a,"STOCKFISH IS OFFLINE",2);draw(a);return;}
    set_status(a,"CALCULATING HINT");draw(a);
    if(!start_engine_worker(a,WORK_HINT,&a->game,320)){set_toast(a,"HINT WORKER FAILED",2);draw(a);}
}

static void undo_game(App*a){
    int count,plies;if(!a||a->game_over)return;if(a->worker_busy){set_toast(a,"WAIT FOR STOCKFISH",1);draw(a);return;}count=sf_history_move_count(&a->game);if(count<=0){set_toast(a,"NO MOVES TO UNDO",1);haptic(a,17);return;}
    if(a->game_mode==MODE_PUZZLE){start_puzzle(a,a->puzzle_index);return;}clock_commit(a);plies=a->game_mode==MODE_ENGINE&&count>=2?2:1;
    if(sf_undo_plies(&a->game,plies)>0){a->side_to_move=sf_side_to_move(&a->game);clear_move_ui(a);update_last_move(a,&a->game);a->in_check=sf_is_in_check(&a->game,a->side_to_move);a->checked_king=sf_find_king(&a->game,a->side_to_move);a->turn_started_ms=now_ms();set_status(a,"MOVE UNDONE");save_state(a);haptic(a,1);}else set_toast(a,"UNDO FAILED",2);draw(a);
}

static void resign_game(App*a){int winner;if(!a||a->game_over)return;if(a->worker_busy){set_toast(a,"WAIT FOR STOCKFISH",1);draw(a);return;}winner=a->side_to_move?RESULT_BLACK:RESULT_WHITE;finish_game(a,winner,REASON_RESIGN,"RESIGNATION");draw(a);}
static void restart_game(App*a){int mode;if(!a)return;if(a->worker_busy){set_toast(a,"WAIT FOR STOCKFISH",1);draw(a);return;}mode=a->game_mode;if(mode==MODE_PUZZLE){start_puzzle(a,a->puzzle_index);return;}start_game(a,mode);}

static void replay_load(App*a,int index,int ply){
    CerHistory*h;if(!a||index<0||index>=a->saved.history_count)return;h=&a->saved.history[index];a->history_index=index;a->replay_ply=i_clamp(ply,0,h->move_count);
    sf_game_from_fen_history(&a->view_game,h->start_fen[0]?h->start_fen:0,h->moves,a->replay_ply);update_last_move(a,&a->view_game);
}
static void open_analysis(App*a,const SfGame*source,int ply){
    if(!a||!source)return;if(a->worker_busy){set_toast(a,"WAIT FOR STOCKFISH",1);draw(a);return;}a->view_game=*source;a->analysis_ply=ply<0?sf_history_move_count(source):ply;
    if(ply>=0)sf_game_from_fen_history(&a->view_game,source->start_fen[0]?source->start_fen:0,source->history,ply);
    memset(&a->analysis,0,sizeof(a->analysis));a->analysis_ready=0;a->previous_screen=a->screen;a->screen=SCR_ANALYSIS;update_last_move(a,&a->view_game);draw(a);
}
static void run_analysis(App*a){if(!a)return;if(a->worker_busy){set_toast(a,"STOCKFISH IS ALREADY CALCULATING",1);draw(a);return;}configure_engine(a);if(!a->engine.ready){set_toast(a,"STOCKFISH IS OFFLINE",2);draw(a);return;}a->analysis_ready=0;set_status(a,"ANALYZING POSITION");draw(a);if(!start_engine_worker(a,WORK_ANALYSIS,&a->view_game,800)){set_toast(a,"ANALYSIS WORKER FAILED",2);draw(a);}}

static int write_export(App*a,const SfGame*g){
    char path[1024],text[4600],moves[4200];const char*cursor;size_t d,n;int fd;if(!a||!g||!a->export_dir[0])return 0;
    d=s_len(a->export_dir);if(d+30>=sizeof(path))return 0;memcpy(path,a->export_dir,d);if(d&&path[d-1]!='/')path[d++]='/';cer_copy_text(path+d,sizeof(path)-d,"Cerelytic-last-game.pgn");
    cer_copy_text(moves,sizeof(moves),g->history);text[0]=0;append_text(text,sizeof(text),"[Event \"Cerelytic Local Game\"]\n[Site \"On Device\"]\n[Result \"");
    append_text(text,sizeof(text),a->result==RESULT_WHITE?"1-0":(a->result==RESULT_BLACK?"0-1":"1/2-1/2"));append_text(text,sizeof(text),"\"]\n\n{ UCI move log } ");append_text(text,sizeof(text),moves);append_text(text,sizeof(text),"\n");
    fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return 0;cursor=text;n=s_len(cursor);while(n){ssize_t w=write(fd,cursor,n);if(w<=0){close(fd);return 0;}n-=(size_t)w;cursor+=w;}close(fd);return 1;
}

static void draw_background(CerCanvas*c){
    int y,band=i_max(1,c->height/80);cer_fill_rect(c,0,0,c->width,c->height,bg_deep());
    for(y=0;y<c->height;y+=band){unsigned lift=(unsigned)(20*y/i_max(1,c->height));cer_fill_rect(c,0,y,c->width,band,cer_rgba(14+lift/3,16+lift/2,15+lift/3,255));}
    cer_fill_circle(c,c->width+px(c->width,40),px(c->width,150),px(c->width,130),cer_rgba(65,102,56,35));
    cer_fill_circle(c,-px(c->width,50),c->height-px(c->width,80),px(c->width,100),cer_rgba(64,128,100,22));
}
static void card(CerCanvas*c,Rect r,int radius,uint32_t color){cer_fill_round_rect(c,r.x,r.y+px(c->width,3),r.w,r.h,radius,cer_rgba(0,0,0,85));cer_fill_round_rect(c,r.x,r.y,r.w,r.h,radius,color);}
static void card_outline(CerCanvas*c,Rect r,int radius,uint32_t color,uint32_t line){card(c,r,radius,color);cer_draw_rect_outline(c,r.x,r.y,r.w,r.h,px(c->width,1),radius,line);}
static void pill(CerCanvas*c,Rect r,const char*label,uint32_t color,uint32_t text){cer_fill_round_rect(c,r.x,r.y,r.w,r.h,r.h/2,color);cer_draw_text_center(c,r.x+r.w/2,r.y+(r.h-px(c->width,12))/2,label,px(c->width,12),text);}
static void section_title(CerCanvas*c,int x,int y,const char*title,const char*action){cer_draw_text(c,x,y,title,px(c->width,17),ink());if(action)cer_draw_text_right(c,c->width-x,y+px(c->width,2),action,px(c->width,13),green());}
static Rect button(CerCanvas*c,Rect r,const char*label,CerIcon icon,int primary){
    uint32_t fill=primary?green():surface2(),txt=primary?ink():ink();card(c,r,px(c->width,13),fill);
    if(icon){int is=px(c->width,20);cer_draw_icon(c,icon,r.x+px(c->width,24),r.y+r.h/2,is,txt,fill);cer_draw_text(c,r.x+px(c->width,44),r.y+(r.h-px(c->width,17))/2,label,px(c->width,17),txt);}
    else cer_draw_text_center(c,r.x+r.w/2,r.y+(r.h-px(c->width,17))/2,label,px(c->width,17),txt);
    return r;
}
static Rect chip(CerCanvas*c,Rect r,const char*label,int active){uint32_t fill=active?green_dim():surface2(),line=active?green():stroke(),txt=active?ink():muted();cer_fill_round_rect(c,r.x,r.y,r.w,r.h,r.h/2,fill);cer_draw_rect_outline(c,r.x,r.y,r.w,r.h,px(c->width,1),r.h/2,line);cer_draw_text_center(c,r.x+r.w/2,r.y+(r.h-px(c->width,13))/2,label,px(c->width,13),txt);return r;}
static void toggle(CerCanvas*c,Rect r,int on){uint32_t fill=on?green():surface3();int knob=r.h-px(c->width,6);cer_fill_round_rect(c,r.x,r.y,r.w,r.h,r.h/2,fill);cer_fill_circle(c,on?r.x+r.w-r.h/2:r.x+r.h/2,r.y+r.h/2,knob/2,ink());}
static void draw_topbar(App*a,CerCanvas*c,const char*title,int back,int settings){
    int y=safe_top(c->width),m=page_margin(c->width),icon=px(c->width,22),mark=px(c->width,34);Rect r;
    cer_fill_rect(c,0,0,c->width,topbar_h(c->width),cer_rgba(18,20,18,245));
    if(back){r=make_rect(m,y+px(c->width,8),px(c->width,42),px(c->width,42));cer_draw_icon(c,CER_ICON_BACK,r.x+r.w/2,r.y+r.h/2,icon,ink(),bg());hit_add(a,HIT_BACK,r);}
    else{cer_draw_brand_mark(c,m+mark/2,y+px(c->width,10)+mark/2,mark,green(),ink(),surface());}
    cer_draw_text(c,m+(back?px(c->width,52):mark+px(c->width,10)),y+px(c->width,18),title,px(c->width,20),ink());
    if(settings){r=make_rect(c->width-m-px(c->width,42),y+px(c->width,8),px(c->width,42),px(c->width,42));cer_draw_icon(c,CER_ICON_SETTINGS,r.x+r.w/2,r.y+r.h/2,icon,muted(),bg());hit_add(a,HIT_PROFILE_SETTINGS,r);}
    cer_fill_rect(c,0,topbar_h(c->width)-1,c->width,1,cer_rgba(255,255,255,12));
}
static void draw_bottom_nav(App*a,CerCanvas*c,int selected){
    static const char*labels[5]={"HOME","PLAY","PUZZLES","HISTORY","PROFILE"};
    static const CerIcon icons[5]={CER_ICON_HOME,CER_ICON_PLAY,CER_ICON_PUZZLE,CER_ICON_HISTORY,CER_ICON_PROFILE};
    int i,h=nav_h(c->width),y=c->height-h,item=c->width/5;
    cer_fill_rect(c,0,y,c->width,h,cer_rgba(27,31,27,250));cer_fill_rect(c,0,y,c->width,1,stroke());
    for(i=0;i<5;++i){int cx=i*item+item/2;uint32_t color=i==selected?green():muted();Rect r=make_rect(i*item,y,item,h);cer_draw_icon(c,icons[i],cx,y+px(c->width,25),px(c->width,21),color,surface());cer_draw_text_center(c,cx,y+px(c->width,48),labels[i],px(c->width,10),color);if(i==selected)cer_fill_round_rect(c,cx-px(c->width,13),y+px(c->width,5),px(c->width,26),px(c->width,3),px(c->width,2),green());hit_add(a,HIT_NAV_BASE+i,r);}
}
static void draw_toast(App*a,CerCanvas*c,int bottom_nav){
    Rect r;int w,y;uint32_t line;if(!a->toast[0]||now_ms()>=a->toast_until_ms)return;w=i_min(c->width-page_margin(c->width)*2,cer_text_width(a->toast,px(c->width,13))+px(c->width,38));y=c->height-(bottom_nav?nav_h(c->width):0)-px(c->width,62);r=make_rect((c->width-w)/2,y,w,px(c->width,43));line=a->toast_kind==2?red():(a->toast_kind==1?amber():green());cer_fill_round_rect(c,r.x,r.y,r.w,r.h,px(c->width,13),cer_rgba(14,16,14,245));cer_fill_round_rect(c,r.x,r.y,px(c->width,5),r.h,px(c->width,3),line);cer_draw_text_center(c,r.x+r.w/2+px(c->width,2),r.y+px(c->width,14),a->toast,px(c->width,13),ink());
}
static void draw_profile_badge(CerCanvas*c,const CerProfile*p,int x,int y,int size){char rating[16];uint32_t ac=accent_for(p?p->accent:0);cer_draw_avatar(c,x,y,size,p?p->avatar:0,ac,surface2());if(p){int_text(p->rating,rating,sizeof(rating));cer_draw_text(c,x+size+px(c->width,10),y+px(c->width,3),p->name,px(c->width,17),ink());cer_draw_text(c,x+size+px(c->width,10),y+px(c->width,26),rating,px(c->width,13),muted());}}

static int board_screen_x(const App*a,int idx){int file=idx%8;return a->board_x+(a->flipped?(7-file):file)*a->board_sq;}
static int board_screen_y(const App*a,int idx){int rank=idx/8;return a->board_y+(a->flipped?rank:(7-rank))*a->board_sq;}
static int board_index_from_point(const App*a,int x,int y){int col,row,file,rank;if(!a||x<a->board_x||y<a->board_y||x>=a->board_x+a->board_size||y>=a->board_y+a->board_size)return-1;col=(x-a->board_x)/a->board_sq;row=(y-a->board_y)/a->board_sq;if(a->flipped){file=7-col;rank=row;}else{file=col;rank=7-row;}return rank*8+file;}
static void board_colors(const App*a,uint32_t*light,uint32_t*dark){if(a->saved.board_theme==1){*light=cer_rgba(230,220,198,255);*dark=cer_rgba(151,119,91,255);}else if(a->saved.board_theme==2){*light=cer_rgba(214,224,229,255);*dark=cer_rgba(91,126,151,255);}else{*light=ivory();*dark=moss();}}
static void draw_board(App*a,CerCanvas*c,const SfGame*g,int x,int y,int size,int interactive){
    int row,col,idx,sq=size/8;uint32_t light,dark;char coord[2]={0,0};
    a->board_x=x;a->board_y=y;a->board_sq=sq;a->board_size=sq*8;a->board_interactive=interactive;
    board_colors(a,&light,&dark);cer_fill_round_rect(c,x-px(c->width,5),y-px(c->width,5),a->board_size+px(c->width,10),a->board_size+px(c->width,10),px(c->width,9),cer_rgba(7,8,7,160));
    for(row=0;row<8;++row)for(col=0;col<8;++col){int file=a->flipped?7-col:col,rank=a->flipped?row:7-row;idx=rank*8+file;uint32_t sc=((row+col)&1)?dark:light;int sx=x+col*sq,sy=y+row*sq;
        cer_fill_rect(c,sx,sy,sq,sq,sc);
        if(idx==a->last_from||idx==a->last_to)cer_blend_rect(c,sx,sy,sq,sq,cer_rgba(225,194,74,135));
        if(idx==a->selected)cer_blend_rect(c,sx,sy,sq,sq,cer_rgba(117,204,70,145));
