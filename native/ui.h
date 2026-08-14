#ifndef CERELYTIC_UI_H
#define CERELYTIC_UI_H
#include "platform.h"
#include "core.h"

typedef struct {
    int width;
    int height;
    int stride;
    void *bits;
} CerCanvas;

typedef enum {
    CER_ICON_HOME = 1,
    CER_ICON_PLAY,
    CER_ICON_PUZZLE,
    CER_ICON_HISTORY,
    CER_ICON_PROFILE,
    CER_ICON_SETTINGS,
    CER_ICON_BACK,
    CER_ICON_MENU,
    CER_ICON_PLUS,
    CER_ICON_UNDO,
    CER_ICON_HINT,
    CER_ICON_FLIP,
    CER_ICON_ANALYSIS,
    CER_ICON_CLOCK,
    CER_ICON_LOCK,
    CER_ICON_TROPHY,
    CER_ICON_CHEVRON,
    CER_ICON_MORE,
    CER_ICON_BOARD,
    CER_ICON_CLOSE,
    CER_ICON_CHECK,
    CER_ICON_EXPORT,
    CER_ICON_RESTART,
    CER_ICON_RESIGN
} CerIcon;

uint32_t cer_rgba(unsigned r, unsigned g, unsigned b, unsigned a);
int cer_min(int a, int b);
int cer_max(int a, int b);
int cer_abs(int a);
int cer_clamp(int value, int low, int high);
void cer_fill_rect(CerCanvas *c, int x, int y, int w, int h, uint32_t color);
void cer_fill_circle(CerCanvas *c, int cx, int cy, int radius, uint32_t color);
void cer_fill_ellipse(CerCanvas *c, int cx, int cy, int rx, int ry, uint32_t color);
void cer_fill_round_rect(CerCanvas *c, int x, int y, int w, int h, int radius, uint32_t color);
void cer_draw_line(CerCanvas *c, int x0, int y0, int x1, int y1, int thickness, uint32_t color);
void cer_draw_ring(CerCanvas *c, int cx, int cy, int outer, int inner, uint32_t color, uint32_t background);
void cer_draw_rect_outline(CerCanvas *c, int x, int y, int w, int h, int thickness, int radius, uint32_t color);
void cer_blend_rect(CerCanvas *c, int x, int y, int w, int h, uint32_t color);
int cer_text_width(const char *text, int pixel_height);
void cer_draw_text(CerCanvas *c, int x, int y, const char *text, int pixel_height, uint32_t color);
void cer_draw_text_center(CerCanvas *c, int center_x, int y, const char *text, int pixel_height, uint32_t color);
void cer_draw_text_right(CerCanvas *c, int right_x, int y, const char *text, int pixel_height, uint32_t color);
void cer_draw_piece(CerCanvas *c, int x, int y, int size, char piece);
void cer_draw_avatar(CerCanvas *c, int x, int y, int size, int avatar, uint32_t accent, uint32_t surface);
void cer_draw_brand_mark(CerCanvas *c, int cx, int cy, int size, uint32_t accent, uint32_t ink, uint32_t surface);
void cer_draw_icon(CerCanvas *c, CerIcon icon, int cx, int cy, int size, uint32_t color, uint32_t surface);
void cer_draw_arrow(CerCanvas *c, int x0, int y0, int x1, int y1, int thickness, uint32_t color);

#endif
