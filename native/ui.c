#include "ui.h"
#include "font_data.h"

static int u_min(int a, int b) { return a < b ? a : b; }
static int u_max(int a, int b) { return a > b ? a : b; }
static int u_abs(int a) { return a < 0 ? -a : a; }
static int u_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int u_pct(int base, int value) { return (base * value) / 100; }

static void u_fill_rect(CerCanvas *b, int x, int y, int w, int h, uint32_t color) {
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

static void u_fill_circle(CerCanvas *b, int cx, int cy, int r, uint32_t color) {
    int y, x, rr;
    if (r <= 0) return;
    rr = r * r;
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

static void u_fill_ellipse(CerCanvas *b, int cx, int cy, int rx, int ry, uint32_t color) {
    int y, x;
    long rxx, ryy, rhs;
    if (rx <= 0 || ry <= 0) return;
    rxx = (long)rx * rx;
    ryy = (long)ry * ry;
    rhs = rxx * ryy;
    for (y = -ry; y <= ry; ++y) {
        int yy = cy + y;
        if (yy < 0 || yy >= b->height) continue;
        for (x = -rx; x <= rx; ++x) {
            int xx = cx + x;
            if (xx < 0 || xx >= b->width) continue;
            if ((long)x*x*ryy + (long)y*y*rxx <= rhs)
                ((uint32_t *)b->bits)[yy * b->stride + xx] = color;
        }
    }
}

static void u_fill_round_rect(CerCanvas *b, int x, int y, int w, int h, int r, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    r = u_clamp(r, 0, u_min(w, h) / 2);
    if (r == 0) { u_fill_rect(b, x, y, w, h, color); return; }
    u_fill_rect(b, x + r, y, w - r*2, h, color);
    u_fill_rect(b, x, y + r, w, h - r*2, color);
    u_fill_circle(b, x + r, y + r, r, color);
    u_fill_circle(b, x + w - r - 1, y + r, r, color);
    u_fill_circle(b, x + r, y + h - r - 1, r, color);
    u_fill_circle(b, x + w - r - 1, y + h - r - 1, r, color);
}

static void u_draw_line(CerCanvas *b, int x0, int y0, int x1, int y1, int thickness, uint32_t color) {
    int dx = u_abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = -u_abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, radius = u_max(1, thickness/2);
    for (;;) {
        u_fill_circle(b, x0, y0, radius, color);
        if (x0 == x1 && y0 == y1) break;
        {
            int e2 = 2*err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}

static void u_fill_polygon(CerCanvas *b, const int *xs, const int *ys, int n, uint32_t color) {
    int min_y, max_y, y, i;
    if (!b || !xs || !ys || n < 3 || n > 24) return;
    min_y = max_y = ys[0];
    for (i=1;i<n;++i) { min_y=u_min(min_y,ys[i]); max_y=u_max(max_y,ys[i]); }
    min_y=u_max(min_y,0); max_y=u_min(max_y,b->height-1);
    for (y=min_y;y<=max_y;++y) {
        int intersections[24], count=0, a, c;
        for (i=0;i<n;++i) {
            int j=(i+1)%n;
            int y1=ys[i], y2=ys[j], x1=xs[i], x2=xs[j];
            if (y1==y2) continue;
            if ((y>=u_min(y1,y2)) && (y<u_max(y1,y2))) {
                intersections[count++] = x1 + (int)(((long)(y-y1)*(x2-x1))/(y2-y1));
            }
        }
        for (a=1;a<count;++a) {
            int v=intersections[a]; c=a-1;
            while (c>=0 && intersections[c]>v) { intersections[c+1]=intersections[c]; --c; }
            intersections[c+1]=v;
        }
        for (a=0;a+1<count;a+=2) u_fill_rect(b,intersections[a],y,intersections[a+1]-intersections[a]+1,1,color);
    }
}

static void u_draw_ring(CerCanvas *b, int cx, int cy, int outer, int inner, uint32_t color, uint32_t bg) {
    u_fill_circle(b,cx,cy,outer,color);
    u_fill_circle(b,cx,cy,inner,bg);
}


static void u_piece_base(CerCanvas *b, int x, int y, int s, uint32_t fill, uint32_t outline) {
    u_fill_round_rect(b,x+u_pct(s,17),y+u_pct(s,73),u_pct(s,66),u_pct(s,15),u_pct(s,5),outline);
    u_fill_round_rect(b,x+u_pct(s,21),y+u_pct(s,76),u_pct(s,58),u_pct(s,9),u_pct(s,3),fill);
    u_fill_round_rect(b,x+u_pct(s,24),y+u_pct(s,66),u_pct(s,52),u_pct(s,10),u_pct(s,4),outline);
    u_fill_round_rect(b,x+u_pct(s,28),y+u_pct(s,68),u_pct(s,44),u_pct(s,6),u_pct(s,2),fill);
}

static void u_draw_pawn_piece(CerCanvas *b,int x,int y,int s,uint32_t fill,uint32_t outline) {
    int xo[6]={x+u_pct(s,39),x+u_pct(s,61),x+u_pct(s,65),x+u_pct(s,71),x+u_pct(s,29),x+u_pct(s,35)};
    int yo[6]={y+u_pct(s,37),y+u_pct(s,37),y+u_pct(s,54),y+u_pct(s,67),y+u_pct(s,67),y+u_pct(s,54)};
    int xi[6]={x+u_pct(s,42),x+u_pct(s,58),x+u_pct(s,61),x+u_pct(s,66),x+u_pct(s,34),x+u_pct(s,39)};
    int yi[6]={y+u_pct(s,40),y+u_pct(s,40),y+u_pct(s,55),y+u_pct(s,64),y+u_pct(s,64),y+u_pct(s,55)};
    u_fill_polygon(b,xo,yo,6,outline); u_fill_polygon(b,xi,yi,6,fill);
    u_fill_ellipse(b,x+u_pct(s,50),y+u_pct(s,27),u_pct(s,14),u_pct(s,14),outline);
    u_fill_ellipse(b,x+u_pct(s,50),y+u_pct(s,27),u_pct(s,10),u_pct(s,10),fill);
    u_fill_round_rect(b,x+u_pct(s,35),y+u_pct(s,36),u_pct(s,30),u_pct(s,8),u_pct(s,4),outline);
    u_fill_round_rect(b,x+u_pct(s,39),y+u_pct(s,38),u_pct(s,22),u_pct(s,4),u_pct(s,2),fill);
    u_piece_base(b,x,y,s,fill,outline);
}

static void u_draw_rook_piece(CerCanvas *b,int x,int y,int s,uint32_t fill,uint32_t outline) {
    u_fill_round_rect(b,x+u_pct(s,29),y+u_pct(s,34),u_pct(s,42),u_pct(s,41),u_pct(s,3),outline);
    u_fill_round_rect(b,x+u_pct(s,34),y+u_pct(s,38),u_pct(s,32),u_pct(s,34),u_pct(s,2),fill);
    u_fill_rect(b,x+u_pct(s,23),y+u_pct(s,24),u_pct(s,54),u_pct(s,17),outline);
    u_fill_rect(b,x+u_pct(s,28),y+u_pct(s,28),u_pct(s,44),u_pct(s,10),fill);
    u_fill_rect(b,x+u_pct(s,23),y+u_pct(s,16),u_pct(s,14),u_pct(s,15),outline);
    u_fill_rect(b,x+u_pct(s,43),y+u_pct(s,16),u_pct(s,14),u_pct(s,15),outline);
    u_fill_rect(b,x+u_pct(s,63),y+u_pct(s,16),u_pct(s,14),u_pct(s,15),outline);
    u_fill_rect(b,x+u_pct(s,27),y+u_pct(s,18),u_pct(s,7),u_pct(s,11),fill);
    u_fill_rect(b,x+u_pct(s,47),y+u_pct(s,18),u_pct(s,7),u_pct(s,11),fill);
    u_fill_rect(b,x+u_pct(s,67),y+u_pct(s,18),u_pct(s,7),u_pct(s,11),fill);
    u_piece_base(b,x,y,s,fill,outline);
}

static void u_draw_bishop_piece(CerCanvas *b,int x,int y,int s,uint32_t fill,uint32_t outline) {
    int xo[8]={x+u_pct(s,50),x+u_pct(s,63),x+u_pct(s,67),x+u_pct(s,62),x+u_pct(s,72),x+u_pct(s,28),x+u_pct(s,38),x+u_pct(s,33)};
    int yo[8]={y+u_pct(s,38),y+u_pct(s,45),y+u_pct(s,58),y+u_pct(s,68),y+u_pct(s,72),y+u_pct(s,72),y+u_pct(s,68),y+u_pct(s,58)};
    int xi[8]={x+u_pct(s,50),x+u_pct(s,59),x+u_pct(s,62),x+u_pct(s,58),x+u_pct(s,65),x+u_pct(s,35),x+u_pct(s,42),x+u_pct(s,38)};
    int yi[8]={y+u_pct(s,42),y+u_pct(s,47),y+u_pct(s,57),y+u_pct(s,65),y+u_pct(s,68),y+u_pct(s,68),y+u_pct(s,65),y+u_pct(s,57)};
    u_fill_polygon(b,xo,yo,8,outline); u_fill_polygon(b,xi,yi,8,fill);
    u_fill_ellipse(b,x+u_pct(s,50),y+u_pct(s,27),u_pct(s,14),u_pct(s,18),outline);
    u_fill_ellipse(b,x+u_pct(s,50),y+u_pct(s,27),u_pct(s,10),u_pct(s,14),fill);
    u_draw_line(b,x+u_pct(s,44),y+u_pct(s,18),x+u_pct(s,56),y+u_pct(s,34),u_max(2,u_pct(s,4)),outline);
    u_fill_round_rect(b,x+u_pct(s,32),y+u_pct(s,42),u_pct(s,36),u_pct(s,8),u_pct(s,3),outline);
    u_fill_round_rect(b,x+u_pct(s,37),y+u_pct(s,44),u_pct(s,26),u_pct(s,4),u_pct(s,2),fill);
    u_piece_base(b,x,y,s,fill,outline);
}

static void u_draw_knight_piece(CerCanvas *b,int x,int y,int s,uint32_t fill,uint32_t outline) {
    int xo[13]={x+u_pct(s,25),x+u_pct(s,31),x+u_pct(s,40),x+u_pct(s,35),x+u_pct(s,42),x+u_pct(s,49),x+u_pct(s,66),x+u_pct(s,75),x+u_pct(s,70),x+u_pct(s,60),x+u_pct(s,54),x+u_pct(s,67),x+u_pct(s,75)};
    int yo[13]={y+u_pct(s,72),y+u_pct(s,55),y+u_pct(s,43),y+u_pct(s,34),y+u_pct(s,34),y+u_pct(s,18),y+u_pct(s,23),y+u_pct(s,34),y+u_pct(s,49),y+u_pct(s,47),y+u_pct(s,57),y+u_pct(s,68),y+u_pct(s,72)};
    int xi[13]={x+u_pct(s,31),x+u_pct(s,36),x+u_pct(s,45),x+u_pct(s,41),x+u_pct(s,47),x+u_pct(s,52),x+u_pct(s,63),x+u_pct(s,69),x+u_pct(s,65),x+u_pct(s,57),x+u_pct(s,50),x+u_pct(s,61),x+u_pct(s,67)};
    int yi[13]={y+u_pct(s,68),y+u_pct(s,56),y+u_pct(s,45),y+u_pct(s,38),y+u_pct(s,38),y+u_pct(s,24),y+u_pct(s,27),y+u_pct(s,35),y+u_pct(s,43),y+u_pct(s,42),y+u_pct(s,58),y+u_pct(s,65),y+u_pct(s,68)};
    u_fill_polygon(b,xo,yo,13,outline); u_fill_polygon(b,xi,yi,13,fill);
    u_fill_circle(b,x+u_pct(s,61),y+u_pct(s,33),u_max(2,u_pct(s,3)),outline);
    u_draw_line(b,x+u_pct(s,40),y+u_pct(s,36),x+u_pct(s,47),y+u_pct(s,56),u_max(2,u_pct(s,3)),outline);
    u_piece_base(b,x,y,s,fill,outline);
}

static void u_draw_queen_piece(CerCanvas *b,int x,int y,int s,uint32_t fill,uint32_t outline) {
    int xo[11]={x+u_pct(s,22),x+u_pct(s,25),x+u_pct(s,36),x+u_pct(s,40),x+u_pct(s,50),x+u_pct(s,60),x+u_pct(s,64),x+u_pct(s,75),x+u_pct(s,78),x+u_pct(s,68),x+u_pct(s,32)};
    int yo[11]={y+u_pct(s,39),y+u_pct(s,20),y+u_pct(s,33),y+u_pct(s,16),y+u_pct(s,32),y+u_pct(s,16),y+u_pct(s,33),y+u_pct(s,20),y+u_pct(s,39),y+u_pct(s,48),y+u_pct(s,48)};
    int xi[11]={x+u_pct(s,28),x+u_pct(s,29),x+u_pct(s,39),x+u_pct(s,42),x+u_pct(s,50),x+u_pct(s,58),x+u_pct(s,61),x+u_pct(s,71),x+u_pct(s,72),x+u_pct(s,64),x+u_pct(s,36)};
    int yi[11]={y+u_pct(s,38),y+u_pct(s,28),y+u_pct(s,38),y+u_pct(s,25),y+u_pct(s,38),y+u_pct(s,25),y+u_pct(s,38),y+u_pct(s,28),y+u_pct(s,38),y+u_pct(s,44),y+u_pct(s,44)};
    int bodyo[6]={x+u_pct(s,32),x+u_pct(s,68),x+u_pct(s,64),x+u_pct(s,71),x+u_pct(s,29),x+u_pct(s,36)};
    int bodyoy[6]={y+u_pct(s,44),y+u_pct(s,44),y+u_pct(s,66),y+u_pct(s,72),y+u_pct(s,72),y+u_pct(s,66)};
    int bodyi[6]={x+u_pct(s,37),x+u_pct(s,63),x+u_pct(s,59),x+u_pct(s,65),x+u_pct(s,35),x+u_pct(s,41)};
    int bodyiy[6]={y+u_pct(s,48),y+u_pct(s,48),y+u_pct(s,64),y+u_pct(s,68),y+u_pct(s,68),y+u_pct(s,64)};
    u_fill_polygon(b,xo,yo,11,outline); u_fill_polygon(b,xi,yi,11,fill);
    u_fill_circle(b,x+u_pct(s,25),y+u_pct(s,18),u_pct(s,5),outline); u_fill_circle(b,x+u_pct(s,25),y+u_pct(s,18),u_pct(s,3),fill);
    u_fill_circle(b,x+u_pct(s,40),y+u_pct(s,14),u_pct(s,5),outline); u_fill_circle(b,x+u_pct(s,40),y+u_pct(s,14),u_pct(s,3),fill);
    u_fill_circle(b,x+u_pct(s,60),y+u_pct(s,14),u_pct(s,5),outline); u_fill_circle(b,x+u_pct(s,60),y+u_pct(s,14),u_pct(s,3),fill);
    u_fill_circle(b,x+u_pct(s,75),y+u_pct(s,18),u_pct(s,5),outline); u_fill_circle(b,x+u_pct(s,75),y+u_pct(s,18),u_pct(s,3),fill);
    u_fill_polygon(b,bodyo,bodyoy,6,outline); u_fill_polygon(b,bodyi,bodyiy,6,fill);
    u_piece_base(b,x,y,s,fill,outline);
}

static void u_draw_king_piece(CerCanvas *b,int x,int y,int s,uint32_t fill,uint32_t outline) {
    int bodyo[8]={x+u_pct(s,50),x+u_pct(s,64),x+u_pct(s,61),x+u_pct(s,68),x+u_pct(s,72),x+u_pct(s,28),x+u_pct(s,32),x+u_pct(s,39)};
    int bodyoy[8]={y+u_pct(s,38),y+u_pct(s,47),y+u_pct(s,60),y+u_pct(s,69),y+u_pct(s,72),y+u_pct(s,72),y+u_pct(s,69),y+u_pct(s,60)};
    int bodyi[8]={x+u_pct(s,50),x+u_pct(s,59),x+u_pct(s,57),x+u_pct(s,62),x+u_pct(s,65),x+u_pct(s,35),x+u_pct(s,38),x+u_pct(s,43)};
    int bodyiy[8]={y+u_pct(s,43),y+u_pct(s,49),y+u_pct(s,59),y+u_pct(s,65),y+u_pct(s,68),y+u_pct(s,68),y+u_pct(s,65),y+u_pct(s,59)};
    u_fill_polygon(b,bodyo,bodyoy,8,outline); u_fill_polygon(b,bodyi,bodyiy,8,fill);
    u_fill_ellipse(b,x+u_pct(s,50),y+u_pct(s,37),u_pct(s,14),u_pct(s,10),outline);
    u_fill_ellipse(b,x+u_pct(s,50),y+u_pct(s,37),u_pct(s,10),u_pct(s,7),fill);
    u_fill_round_rect(b,x+u_pct(s,46),y+u_pct(s,11),u_pct(s,8),u_pct(s,24),u_pct(s,2),outline);
    u_fill_round_rect(b,x+u_pct(s,48),y+u_pct(s,13),u_pct(s,4),u_pct(s,20),u_pct(s,1),fill);
    u_fill_round_rect(b,x+u_pct(s,38),y+u_pct(s,17),u_pct(s,24),u_pct(s,8),u_pct(s,2),outline);
    u_fill_round_rect(b,x+u_pct(s,41),y+u_pct(s,19),u_pct(s,18),u_pct(s,4),u_pct(s,1),fill);
    u_piece_base(b,x,y,s,fill,outline);
}

void cer_draw_piece(CerCanvas *b,int x,int y,int s,char p) {
    int white=sf_is_white_piece(p);
    char type=white?p:(char)(p-'a'+'A');
    uint32_t outline=white?cer_rgba(55,53,49,255):cer_rgba(22,22,21,255);
    uint32_t fill=white?cer_rgba(246,244,232,255):cer_rgba(64,62,58,255);
    uint32_t highlight=white?cer_rgba(255,255,248,255):cer_rgba(91,88,81,255);
    u_fill_ellipse(b,x+u_pct(s,50)+u_pct(s,2),y+u_pct(s,82)+u_pct(s,2),u_pct(s,31),u_pct(s,7),cer_rgba(26,25,23,255));
    switch(type) {
        case 'P': u_draw_pawn_piece(b,x,y,s,fill,outline); break;
        case 'R': u_draw_rook_piece(b,x,y,s,fill,outline); break;
        case 'N': u_draw_knight_piece(b,x,y,s,fill,outline); break;
        case 'B': u_draw_bishop_piece(b,x,y,s,fill,outline); break;
        case 'Q': u_draw_queen_piece(b,x,y,s,fill,outline); break;
        case 'K': u_draw_king_piece(b,x,y,s,fill,outline); break;
        default: break;
    }
    if (type!='N') u_fill_ellipse(b,x+u_pct(s,43),y+u_pct(s,48),u_pct(s,3),u_pct(s,10),highlight);
}

uint32_t cer_rgba(unsigned r, unsigned g, unsigned b, unsigned a) {
    return (uint32_t)(r | (g << 8) | (b << 16) | (a << 24));
}
int cer_min(int a,int b){return a<b?a:b;}
int cer_max(int a,int b){return a>b?a:b;}
int cer_abs(int a){return a<0?-a:a;}
int cer_clamp(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
void cer_fill_rect(CerCanvas*c,int x,int y,int w,int h,uint32_t color){u_fill_rect(c,x,y,w,h,color);}
void cer_fill_circle(CerCanvas*c,int x,int y,int r,uint32_t color){u_fill_circle(c,x,y,r,color);}
void cer_fill_ellipse(CerCanvas*c,int x,int y,int rx,int ry,uint32_t color){u_fill_ellipse(c,x,y,rx,ry,color);}
void cer_fill_round_rect(CerCanvas*c,int x,int y,int w,int h,int r,uint32_t color){u_fill_round_rect(c,x,y,w,h,r,color);}
void cer_draw_line(CerCanvas*c,int x0,int y0,int x1,int y1,int t,uint32_t color){u_draw_line(c,x0,y0,x1,y1,t,color);}
void cer_draw_ring(CerCanvas*c,int x,int y,int o,int i,uint32_t color,uint32_t bg){u_draw_ring(c,x,y,o,i,color,bg);}

static unsigned component(uint32_t color,int shift){return (color>>shift)&255u;}
static uint32_t blend_color(uint32_t dst,uint32_t src,unsigned alpha){
    unsigned inv=255u-alpha;
    unsigned r=(component(src,0)*alpha+component(dst,0)*inv+127u)/255u;
    unsigned g=(component(src,8)*alpha+component(dst,8)*inv+127u)/255u;
    unsigned b=(component(src,16)*alpha+component(dst,16)*inv+127u)/255u;
    return cer_rgba(r,g,b,255);
}
static void blend_pixel(CerCanvas*c,int x,int y,uint32_t color,unsigned coverage){
    uint32_t *p;
    unsigned alpha=(component(color,24)*coverage+127u)/255u;
    if(!c||!c->bits||x<0||y<0||x>=c->width||y>=c->height||!alpha)return;
    p=(uint32_t*)c->bits+y*c->stride+x;
    *p=alpha>=255?cer_rgba(component(color,0),component(color,8),component(color,16),255):blend_color(*p,color,alpha);
}
void cer_blend_rect(CerCanvas*c,int x,int y,int w,int h,uint32_t color){
    int yy,xx; unsigned alpha=component(color,24);
    if(!alpha)return;
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>c->width)w=c->width-x; if(y+h>c->height)h=c->height-y;
    for(yy=y;yy<y+h;++yy)for(xx=x;xx<x+w;++xx)blend_pixel(c,xx,yy,color,255);
}
void cer_draw_rect_outline(CerCanvas*c,int x,int y,int w,int h,int t,int r,uint32_t color){
    int i; if(t<1)t=1;
    for(i=0;i<t;++i){
        u_draw_line(c,x+r,y+i,x+w-r-1,y+i,1,color);
        u_draw_line(c,x+r,y+h-1-i,x+w-r-1,y+h-1-i,1,color);
        u_draw_line(c,x+i,y+r,x+i,y+h-r-1,1,color);
        u_draw_line(c,x+w-1-i,y+r,x+w-1-i,y+h-r-1,1,color);
    }
    if(r>0){
        /* Rounded corners are deliberately subtle, because humans keep trying to
           turn every rectangle into a vitamin gummy. */
        u_fill_circle(c,x+r,y+r,t,color); u_fill_circle(c,x+w-r-1,y+r,t,color);
        u_fill_circle(c,x+r,y+h-r-1,t,color); u_fill_circle(c,x+w-r-1,y+h-r-1,t,color);
    }
}

static int glyph_index(char ch){
    unsigned c=(unsigned char)ch;
    if(c<CER_FONT_FIRST||c>CER_FONT_LAST)c='?';
    return (int)c-CER_FONT_FIRST;
}
int cer_text_width(const char *text,int px){
    int width=0; if(px<5)px=5;
    while(text&&*text){int idx=glyph_index(*text++);width+=(cer_font_advance[idx]*px+CER_FONT_H/2)/CER_FONT_H;}
    return width;
}
void cer_draw_text(CerCanvas*c,int x,int y,const char*text,int px,uint32_t color){
    int target_w,idx,gx,gy,sx,sy; unsigned cov;
    if(!c||!text)return; if(px<5)px=5;
    while(*text){
        idx=glyph_index(*text++);
        target_w=(CER_FONT_W*px+CER_FONT_H/2)/CER_FONT_H;
        if(target_w<1)target_w=1;
        for(gy=0;gy<px;++gy){
            sy=(gy*CER_FONT_H)/px;
            for(gx=0;gx<target_w;++gx){
                sx=(gx*CER_FONT_W)/target_w;
                cov=cer_font_bitmap[idx*CER_FONT_W*CER_FONT_H+sy*CER_FONT_W+sx];
                if(cov)blend_pixel(c,x+gx,y+gy,color,cov);
            }
        }
        x+=(cer_font_advance[idx]*px+CER_FONT_H/2)/CER_FONT_H;
    }
}
void cer_draw_text_center(CerCanvas*c,int cx,int y,const char*s,int px,uint32_t color){cer_draw_text(c,cx-cer_text_width(s,px)/2,y,s,px,color);}
void cer_draw_text_right(CerCanvas*c,int r,int y,const char*s,int px,uint32_t color){cer_draw_text(c,r-cer_text_width(s,px),y,s,px,color);}

void cer_draw_arrow(CerCanvas*c,int x0,int y0,int x1,int y1,int t,uint32_t color){
    int dx=x1-x0,dy=y1-y0,len=cer_max(1,cer_abs(dx)+cer_abs(dy));
    int hx=x1-(dx*18)/len,hy=y1-(dy*18)/len;
    int px=-(dy*12)/len,py=(dx*12)/len;
    u_draw_line(c,x0,y0,x1,y1,t,color);
    u_draw_line(c,x1,y1,hx+px,hy+py,cer_max(2,t),color);
    u_draw_line(c,x1,y1,hx-px,hy-py,cer_max(2,t),color);
}

void cer_draw_brand_mark(CerCanvas*c,int cx,int cy,int size,uint32_t accent,uint32_t ink,uint32_t surface){
    int r=size/2,t=cer_max(2,size/18);
    u_fill_circle(c,cx,cy,r,surface);
    u_draw_ring(c,cx,cy,r-t,r-3*t,accent,surface);
    /* Geometric knight/C hybrid, original enough to avoid borrowing someone
       else's horse and then acting surprised when lawyers appear. */
    {
        int xs[9]={cx-size*18/100,cx-size*2/100,cx+size*12/100,cx+size*24/100,cx+size*17/100,cx+size*5/100,cx+size*17/100,cx-size*22/100,cx-size*26/100};
        int ys[9]={cy+size*23/100,cy-size*25/100,cy-size*34/100,cy-size*15/100,cy+size*5/100,cy+size*11/100,cy+size*23/100,cy+size*23/100,cy+size*10/100};
        u_fill_polygon(c,xs,ys,9,ink);
        u_fill_circle(c,cx+size*7/100,cy-size*17/100,cer_max(1,size/35),surface);
        u_draw_line(c,cx-size*12/100,cy+size*13/100,cx+size*12/100,cy+size*13/100,cer_max(2,size/28),accent);
    }
}

void cer_draw_avatar(CerCanvas*c,int x,int y,int size,int avatar,uint32_t accent,uint32_t surface){
    uint32_t skin,shirt,ink=cer_rgba(245,247,243,255);
    int variant=avatar%5;
    skin=variant==0?cer_rgba(226,181,145,255):variant==1?cer_rgba(151,103,72,255):variant==2?cer_rgba(233,197,165,255):variant==3?cer_rgba(109,74,56,255):cer_rgba(206,150,111,255);
    shirt=variant==0?accent:variant==1?cer_rgba(72,120,148,255):variant==2?cer_rgba(156,92,119,255):variant==3?cer_rgba(118,102,161,255):cer_rgba(183,124,64,255);
    u_fill_round_rect(c,x,y,size,size,cer_max(6,size/5),surface);
    u_fill_circle(c,x+size/2,y+size*36/100,size*18/100,skin);
    u_fill_ellipse(c,x+size/2,y+size*92/100,size*36/100,size*34/100,shirt);
    if(variant&1)u_fill_round_rect(c,x+size*30/100,y+size*13/100,size*40/100,size*18/100,size/12,cer_rgba(43,35,31,255));
    else u_fill_ellipse(c,x+size/2,y+size*23/100,size*22/100,size*13/100,cer_rgba(48,37,29,255));
    u_fill_circle(c,x+size*44/100,y+size*36/100,cer_max(1,size/45),cer_rgba(35,31,29,255));
    u_fill_circle(c,x+size*57/100,y+size*36/100,cer_max(1,size/45),cer_rgba(35,31,29,255));
    u_draw_line(c,x+size*45/100,y+size*48/100,x+size*56/100,y+size*48/100,cer_max(1,size/60),ink);
    u_fill_rect(c,x,y,size/12,size,accent);
}

static void icon_home(CerCanvas*c,int cx,int cy,int s,uint32_t color){
    int xs[5]={cx-s/2,cx,cx+s/2,cx+s*2/5,cx-s*2/5};
    int ys[5]={cy-s/10,cy-s/2,cy-s/10,cy+s/2,cy+s/2};
    u_fill_polygon(c,xs,ys,5,color); u_fill_rect(c,cx-s/10,cy+s/10,s/5,s*2/5,cer_rgba(0,0,0,100));
}
static void icon_profile(CerCanvas*c,int cx,int cy,int s,uint32_t color){u_fill_circle(c,cx,cy-s/5,s/5,color);u_fill_ellipse(c,cx,cy+s/3,s*2/5,s/4,color);}
static void icon_settings(CerCanvas*c,int cx,int cy,int s,uint32_t color,uint32_t surface){
    int i; u_fill_circle(c,cx,cy,s/3,color);u_fill_circle(c,cx,cy,s/7,surface);
    for(i=0;i<8;++i){int dx=(i==0||i==4)?0:(i==1||i==3)?s/4:(i==5||i==7)?-s/4:(i==2?s/3:-s/3);int dy=(i==2||i==6)?0:(i==1||i==7)?-s/4:(i==3||i==5)?s/4:(i==0?-s/3:s/3);u_fill_circle(c,cx+dx,cy+dy,s/10,color);}
}
void cer_draw_icon(CerCanvas*c,CerIcon icon,int cx,int cy,int s,uint32_t color,uint32_t surface){
    int t=cer_max(2,s/9),h=s/2;
    switch(icon){
        case CER_ICON_HOME:icon_home(c,cx,cy,s,color);break;
        case CER_ICON_PLAY:{int xs[3]={cx-h,cx+h,cx-h};int ys[3]={cy-h,cy,cy+h};u_fill_polygon(c,xs,ys,3,color);}break;
        case CER_ICON_PUZZLE:u_fill_round_rect(c,cx-h,cy-h,s,s,s/7,color);u_fill_circle(c,cx+s/2,cy-s/5,s/6,surface);u_fill_circle(c,cx-s/5,cy-s/2,s/6,color);u_fill_circle(c,cx-s/2,cy+s/5,s/6,surface);break;
        case CER_ICON_HISTORY:u_draw_ring(c,cx,cy,s/2,s/2-t,color,surface);u_draw_line(c,cx,cy,cx,cy-s/4,t,color);u_draw_line(c,cx,cy,cx+s/4,cy,t,color);break;
        case CER_ICON_PROFILE:icon_profile(c,cx,cy,s,color);break;
        case CER_ICON_SETTINGS:icon_settings(c,cx,cy,s,color,surface);break;
        case CER_ICON_BACK:u_draw_line(c,cx+h/2,cy-h,cx-h/2,cy,t,color);u_draw_line(c,cx-h/2,cy,cx+h/2,cy+h,t,color);break;
        case CER_ICON_MENU:u_draw_line(c,cx-h,cy-s/3,cx+h,cy-s/3,t,color);u_draw_line(c,cx-h,cy,cx+h,cy,t,color);u_draw_line(c,cx-h,cy+s/3,cx+h,cy+s/3,t,color);break;
        case CER_ICON_PLUS:u_draw_line(c,cx-h,cy,cx+h,cy,t,color);u_draw_line(c,cx,cy-h,cx,cy+h,t,color);break;
        case CER_ICON_UNDO:u_draw_line(c,cx-h,cy,cx-h/3,cy-h/2,t,color);u_draw_line(c,cx-h,cy,cx-h/3,cy+h/2,t,color);u_draw_line(c,cx-h,cy,cx+h/4,cy,t,color);u_draw_line(c,cx+h/4,cy,cx+h,cy+h/3,t,color);break;
        case CER_ICON_HINT:u_draw_ring(c,cx,cy-s/7,s/3,s/3-t,color,surface);u_fill_round_rect(c,cx-s/8,cy+s/5,s/4,s/4,s/12,color);u_draw_line(c,cx,cy-h,cx,cy-s*3/4,t,color);break;
        case CER_ICON_FLIP:u_draw_ring(c,cx,cy,s/2,s/2-t,color,surface);cer_draw_arrow(c,cx-h,cy,cx,cy-h,t,color);cer_draw_arrow(c,cx+h,cy,cx,cy+h,t,color);break;
        case CER_ICON_ANALYSIS:u_draw_ring(c,cx-s/8,cy-s/8,s/3,s/3-t,color,surface);u_draw_line(c,cx+s/8,cy+s/8,cx+h,cy+h,t,color);break;
        case CER_ICON_CLOCK:u_draw_ring(c,cx,cy,s/2,s/2-t,color,surface);u_draw_line(c,cx,cy,cx,cy-s/4,t,color);u_draw_line(c,cx,cy,cx+s/5,cy+s/8,t,color);break;
        case CER_ICON_LOCK:u_fill_round_rect(c,cx-h,cy-s/8,s,s*5/8,s/10,color);u_draw_ring(c,cx,cy-s/4,s/3,s/3-t,color,surface);break;
        case CER_ICON_TROPHY:u_fill_round_rect(c,cx-s/4,cy-s/3,s/2,s/2,s/8,color);u_draw_line(c,cx-s/2,cy-s/4,cx-s/4,cy,t,color);u_draw_line(c,cx+s/2,cy-s/4,cx+s/4,cy,t,color);u_draw_line(c,cx,cy+s/5,cx,cy+h,t,color);u_draw_line(c,cx-s/3,cy+h,cx+s/3,cy+h,t,color);break;
        case CER_ICON_CHEVRON:u_draw_line(c,cx-s/4,cy-h,cx+s/4,cy,t,color);u_draw_line(c,cx+s/4,cy,cx-s/4,cy+h,t,color);break;
        case CER_ICON_MORE:u_fill_circle(c,cx-s/3,cy,t,color);u_fill_circle(c,cx,cy,t,color);u_fill_circle(c,cx+s/3,cy,t,color);break;
        case CER_ICON_BOARD:cer_draw_rect_outline(c,cx-h,cy-h,s,s,t,2,color);u_draw_line(c,cx,cy-h,cx,cy+h,t,color);u_draw_line(c,cx-h,cy,cx+h,cy,t,color);break;
        case CER_ICON_CLOSE:u_draw_line(c,cx-h,cy-h,cx+h,cy+h,t,color);u_draw_line(c,cx+h,cy-h,cx-h,cy+h,t,color);break;
        case CER_ICON_CHECK:u_draw_line(c,cx-h,cy,cx-s/8,cy+h,t,color);u_draw_line(c,cx-s/8,cy+h,cx+h,cy-h,t,color);break;
        case CER_ICON_EXPORT:u_draw_line(c,cx,cy+h,cx,cy-h,t,color);cer_draw_arrow(c,cx,cy-s/5,cx,cy-h,t,color);cer_draw_rect_outline(c,cx-h,cy,s,s/2,t,2,color);break;
        case CER_ICON_RESTART:u_draw_ring(c,cx,cy,s/2,s/2-t,color,surface);cer_draw_arrow(c,cx-h,cy,cx,cy-h,t,color);break;
        case CER_ICON_RESIGN:u_fill_rect(c,cx-s/3,cy-h,t,s,color);{int xs[3]={cx-s/3+t,cx+h,cx-s/3+t};int ys[3]={cy-h,cy-s/5,cy+s/8};u_fill_polygon(c,xs,ys,3,color);}break;
        default:break;
    }
}
