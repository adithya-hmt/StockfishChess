        if(idx==a->checked_king&&a->in_check)cer_blend_rect(c,sx,sy,sq,sq,a->game_over?cer_rgba(229,72,77,205):cer_rgba(242,157,73,170));
        if(a->saved.coordinates_enabled){if(col==0){coord[0]=(char)('1'+rank);cer_draw_text(c,sx+px(c->width,3),sy+px(c->width,2),coord,px(c->width,9),((row+col)&1)?light:dark);}if(row==7){coord[0]=(char)('a'+file);cer_draw_text_right(c,sx+sq-px(c->width,3),sy+sq-px(c->width,11),coord,px(c->width,9),((row+col)&1)?light:dark);}}
    }
    if(interactive&&a->selected>=0&&a->saved.legal_moves_enabled){for(idx=0;idx<64;++idx)if(destination_has_move(a,a->selected,idx)){int cx=board_screen_x(a,idx)+sq/2,cy=board_screen_y(a,idx)+sq/2;if(g->board[idx])cer_draw_ring(c,cx,cy,sq/2-px(c->width,3),sq/2-px(c->width,8),cer_rgba(78,120,55,190),cer_rgba(0,0,0,0));else cer_fill_circle(c,cx,cy,i_max(3,sq/9),cer_rgba(49,84,40,190));}}
    if(a->hint_from>=0&&a->hint_to>=0){int x0=board_screen_x(a,a->hint_from)+sq/2,y0=board_screen_y(a,a->hint_from)+sq/2,x1=board_screen_x(a,a->hint_to)+sq/2,y1=board_screen_y(a,a->hint_to)+sq/2;cer_draw_arrow(c,x0,y0,x1,y1,i_max(4,sq/9),cer_rgba(69,207,112,210));}
    for(idx=0;idx<64;++idx){char p=g->board[idx];int sx,sy;if(!p)continue;if(a->anim_active&&idx==a->anim_to)continue;if(a->dragging&&idx==a->drag_from)continue;sx=board_screen_x(a,idx);sy=board_screen_y(a,idx);cer_draw_piece(c,sx+sq/9,sy+sq/10,sq*8/10,p);}
    if(a->anim_active){int fx=board_screen_x(a,a->anim_from),fy=board_screen_y(a,a->anim_from),tx=board_screen_x(a,a->anim_to),ty=board_screen_y(a,a->anim_to);int sx=fx+(tx-fx)*a->anim_progress/100,sy=fy+(ty-fy)*a->anim_progress/100;cer_draw_piece(c,sx+sq/9,sy+sq/10,sq*8/10,a->anim_piece);}
    if(a->dragging&&a->drag_from>=0){cer_draw_piece(c,a->drag_x-sq*4/10,a->drag_y-sq*4/10,sq*8/10,g->board[a->drag_from]);}
    if(a->game_over&&a->result_reason==REASON_MATE)cer_draw_rect_outline(c,x-px(c->width,3),y-px(c->width,3),a->board_size+px(c->width,6),a->board_size+px(c->width,6),px(c->width,4),px(c->width,5),red());
}

static void draw_player_row(App*a,CerCanvas*c,Rect r,int white,int top){
    const CerProfile*p=cer_active_profile_const(&a->saved);char clock[12],sub[36];uint32_t ac=accent_for(p?p->accent:0);int avatar=px(c->width,42),active=a->side_to_move==white&&!a->game_over;
    cer_fill_round_rect(c,r.x,r.y,r.w,r.h,px(c->width,12),active?surface2():surface());
    if(top&&a->game_mode==MODE_ENGINE){cer_draw_brand_mark(c,r.x+px(c->width,25),r.y+r.h/2,px(c->width,36),green(),ink(),surface2());cer_draw_text(c,r.x+px(c->width,50),r.y+px(c->width,10),"STOCKFISH",px(c->width,15),ink());cer_copy_text(sub,sizeof(sub),level_name(a->level));append_text(sub,sizeof(sub)," / ON DEVICE");cer_draw_text(c,r.x+px(c->width,50),r.y+px(c->width,31),sub,px(c->width,10),muted());}
    else if(top&&a->game_mode==MODE_LOCAL){cer_draw_avatar(c,r.x+px(c->width,7),r.y+px(c->width,7),avatar,1,ac,surface2());cer_draw_text(c,r.x+px(c->width,56),r.y+px(c->width,11),white?"WHITE":"BLACK",px(c->width,15),ink());cer_draw_text(c,r.x+px(c->width,56),r.y+px(c->width,31),"PASS & PLAY",px(c->width,10),muted());}
    else{cer_draw_avatar(c,r.x+px(c->width,7),r.y+px(c->width,7),avatar,p?p->avatar:0,ac,surface2());cer_draw_text(c,r.x+px(c->width,56),r.y+px(c->width,11),p?p->name:"PLAYER",px(c->width,15),ink());cer_draw_text(c,r.x+px(c->width,56),r.y+px(c->width,31),white?"PLAYING WHITE":"PLAYING BLACK",px(c->width,10),muted());}
    if(a->time_control){format_clock(display_clock(a,white),clock);pill(c,make_rect(r.x+r.w-px(c->width,76),r.y+px(c->width,10),px(c->width,66),px(c->width,34)),clock,active?green_dim():surface3(),ink());}
    else pill(c,make_rect(r.x+r.w-px(c->width,58),r.y+px(c->width,13),px(c->width,48),px(c->width,29)),active?"TURN":"READY",active?green_dim():surface3(),active?mint():muted());
}

static void render_splash(App*a,CerCanvas*c){int mark=px(c->width,122),cy=c->height/2-px(c->width,78);(void)a;cer_draw_brand_mark(c,c->width/2,cy,mark,green(),ink(),surface());cer_draw_text_center(c,c->width/2,cy+px(c->width,78),"FRAMILTON",px(c->width,31),ink());cer_draw_text_center(c,c->width/2,cy+px(c->width,116),"CHESS, ENTIRELY ON YOUR DEVICE",px(c->width,12),muted());cer_fill_round_rect(c,c->width/2-px(c->width,36),cy+px(c->width,158),px(c->width,72),px(c->width,5),px(c->width,3),green());}

static void render_onboard(App*a,CerCanvas*c){
    static const char*titles[3]={"PLAY WITHOUT THE CLOUD","STOCKFISH, RIGHT HERE","YOUR BOARD. YOUR DATA."};
    static const char*body[3]={"Profiles, games, puzzles and settings stay on this device. No account server. No tracking theater.","Challenge local Stockfish, tune its strength and get hints without sending a position anywhere.","Save games, replay every move, analyze locally and protect profiles with a device-local PIN."};
    int m=page_margin(c->width),top=safe_top(c->width)+px(c->width,28),i;Rect hero,next,skip;CerIcon icon=a->onboarding_page==0?CER_ICON_LOCK:(a->onboarding_page==1?CER_ICON_ANALYSIS:CER_ICON_HISTORY);
    cer_draw_brand_mark(c,m+px(c->width,22),top+px(c->width,20),px(c->width,42),green(),ink(),surface());cer_draw_text(c,m+px(c->width,52),top+px(c->width,10),"FRAMILTON CHESS",px(c->width,18),ink());
    hero=make_rect(m,top+px(c->width,82),c->width-2*m,px(c->width,310));card_outline(c,hero,px(c->width,22),surface(),stroke());
    cer_fill_circle(c,c->width/2,hero.y+px(c->width,94),px(c->width,49),green_dim());cer_draw_icon(c,icon,c->width/2,hero.y+px(c->width,94),px(c->width,46),mint(),green_dim());
    cer_draw_text_center(c,c->width/2,hero.y+px(c->width,170),titles[a->onboarding_page],px(c->width,22),ink());
    /* body deliberately split into compact lines rather than asking a tiny raster font to perform miracles. */
    if(a->onboarding_page==0){cer_draw_text_center(c,c->width/2,hero.y+px(c->width,216),"PROFILES, GAMES AND PUZZLES",px(c->width,13),muted());cer_draw_text_center(c,c->width/2,hero.y+px(c->width,238),"STAY ON THIS DEVICE.",px(c->width,13),muted());cer_draw_text_center(c,c->width/2,hero.y+px(c->width,270),"NO TRACKERS. NO ACCOUNT SERVER.",px(c->width,12),green());}
    else if(a->onboarding_page==1){cer_draw_text_center(c,c->width/2,hero.y+px(c->width,216),"STOCKFISH RUNS OFFLINE",px(c->width,13),muted());cer_draw_text_center(c,c->width/2,hero.y+px(c->width,238),"WITH LEVELS, HINTS AND ANALYSIS.",px(c->width,13),muted());cer_draw_text_center(c,c->width/2,hero.y+px(c->width,270),"YOUR POSITION NEVER LEAVES.",px(c->width,12),green());}
    else{cer_draw_text_center(c,c->width/2,hero.y+px(c->width,216),"SAVE, REPLAY AND ANALYZE",px(c->width,13),muted());cer_draw_text_center(c,c->width/2,hero.y+px(c->width,238),"EVERY LOCAL GAME.",px(c->width,13),muted());cer_draw_text_center(c,c->width/2,hero.y+px(c->width,270),"OPTIONAL LOCAL PIN PROTECTION.",px(c->width,12),green());}
    (void)body;
    for(i=0;i<3;++i)cer_fill_circle(c,c->width/2+(i-1)*px(c->width,18),hero.y+hero.h+px(c->width,30),px(c->width,i==a->onboarding_page?5:3),i==a->onboarding_page?green():surface3());
    next=make_rect(m,c->height-px(c->width,92),c->width-2*m,px(c->width,54));button(c,next,a->onboarding_page==2?"CREATE LOCAL PROFILE":"CONTINUE",CER_ICON_CHEVRON,1);hit_add(a,HIT_NEXT,next);
    skip=make_rect(c->width-m-px(c->width,72),top-px(c->width,2),px(c->width,72),px(c->width,38));cer_draw_text_center(c,skip.x+skip.w/2,skip.y+px(c->width,11),"SKIP",px(c->width,13),muted());hit_add(a,HIT_SKIP,skip);
}

static const char KEY_ROWS[3][11]={{'Q','W','E','R','T','Y','U','I','O','P',0},{'A','S','D','F','G','H','J','K','L',0,0},{'Z','X','C','V','B','N','M',0,0,0,0}};
static void render_profile_create(App*a,CerCanvas*c){
    int m=page_margin(c->width),top=topbar_h(c->width)+px(c->width,8),i,j;Rect name,avatar,key,next;
    draw_topbar(a,c,"LOCAL PROFILE",a->saved.profile_count>0,0);
    cer_draw_text(c,m,top,"WHO IS PLAYING?",px(c->width,25),ink());cer_draw_text(c,m,top+px(c->width,34),"No email. No password server. Just a profile on this device.",px(c->width,11),muted());
    name=make_rect(m,top+px(c->width,68),c->width-2*m,px(c->width,58));card_outline(c,name,px(c->width,13),surface2(),a->profile_name[0]?green_dim():stroke());cer_draw_text(c,name.x+px(c->width,16),name.y+px(c->width,18),a->profile_name[0]?a->profile_name:"PROFILE NAME",px(c->width,18),a->profile_name[0]?ink():muted());
    cer_draw_text(c,m,top+px(c->width,147),"CHOOSE AN AVATAR",px(c->width,14),muted());
    for(i=0;i<5;++i){int s=px(c->width,52),gap=(c->width-2*m-5*s)/4;avatar=make_rect(m+i*(s+gap),top+px(c->width,174),s,s);cer_draw_avatar(c,avatar.x,avatar.y,s,i,accent_for(a->profile_accent),surface2());if(i==a->profile_avatar)cer_draw_rect_outline(c,avatar.x-px(c->width,3),avatar.y-px(c->width,3),avatar.w+px(c->width,6),avatar.h+px(c->width,6),px(c->width,3),px(c->width,15),green());hit_add(a,HIT_AVATAR_BASE+i,avatar);}
    cer_draw_text(c,m,top+px(c->width,246),"ACCENT",px(c->width,14),muted());for(i=0;i<5;++i){Rect ac=make_rect(m+i*px(c->width,46),top+px(c->width,270),px(c->width,34),px(c->width,34));cer_fill_circle(c,ac.x+ac.w/2,ac.y+ac.h/2,px(c->width,13),accent_for(i));if(i==a->profile_accent)cer_draw_ring(c,ac.x+ac.w/2,ac.y+ac.h/2,px(c->width,17),px(c->width,14),ink(),bg());hit_add(a,HIT_AVATAR_BASE+10+i,ac);}
    for(i=0;i<3;++i){int count=i==0?10:(i==1?9:7),kw=px(c->width,29),gap=px(c->width,4),roww=count*kw+(count-1)*gap,start=(c->width-roww)/2,ky=top+px(c->width,325)+i*px(c->width,43);for(j=0;j<count;++j){char label[2]={KEY_ROWS[i][j],0};key=make_rect(start+j*(kw+gap),ky,kw,px(c->width,36));cer_fill_round_rect(c,key.x,key.y,key.w,key.h,px(c->width,7),surface2());cer_draw_text_center(c,key.x+key.w/2,key.y+px(c->width,10),label,px(c->width,14),ink());hit_add(a,HIT_KEY_BASE+i*10+j,key);}}
    key=make_rect(m,top+px(c->width,458),px(c->width,92),px(c->width,38));button(c,key,"DELETE",CER_ICON_BACK,0);hit_add(a,HIT_PIN_DELETE,key);
    next=make_rect(m,c->height-px(c->width,78),c->width-2*m,px(c->width,52));button(c,next,"CONTINUE",CER_ICON_CHEVRON,1);hit_add(a,HIT_CONTINUE,next);
}

static void draw_pin_dots(CerCanvas*c,int y,int digits){int i;for(i=0;i<4;++i){cer_fill_circle(c,c->width/2+(i-2)*px(c->width,34)+px(c->width,17),y,px(c->width,7),i<digits?green():surface3());}}
static void render_pin(App*a,CerCanvas*c,int unlock){
    int m=page_margin(c->width),top=topbar_h(c->width)+px(c->width,20),i;Rect key,skip;draw_topbar(a,c,unlock?"UNLOCK PROFILE":"LOCAL APP LOCK",!unlock,0);
    cer_fill_circle(c,c->width/2,top+px(c->width,68),px(c->width,45),green_dim());cer_draw_icon(c,CER_ICON_LOCK,c->width/2,top+px(c->width,68),px(c->width,40),mint(),green_dim());
    cer_draw_text_center(c,c->width/2,top+px(c->width,135),unlock?"ENTER YOUR PIN":(a->pin_stage?"CONFIRM YOUR PIN":"ADD A 4-DIGIT PIN"),px(c->width,23),ink());
    cer_draw_text_center(c,c->width/2,top+px(c->width,170),unlock?"Stored and checked only on this device.":"Optional. This is local privacy, not cloud authentication.",px(c->width,11),muted());draw_pin_dots(c,top+px(c->width,218),a->pin_digits);
