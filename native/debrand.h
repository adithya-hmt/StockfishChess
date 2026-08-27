#ifndef CHESS_DEBRAND_H
#define CHESS_DEBRAND_H

/*
 * Keep the existing rendering API and package namespace intact while removing
 * the Cerelytic identity from user-visible surfaces. This lets existing app
 * installs continue to see the same local data directory and signing lineage.
 */
static int chess_label_equals(const char *left, const char *right) {
    if (!left || !right) return left == right;
    while (*left && *right && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static const char *chess_debranded_label(const char *text) {
    if (chess_label_equals(text, "CERELYTIC") ||
        chess_label_equals(text, "CERELYTIC CHESS")) {
        return "CHESS";
    }
    if (chess_label_equals(text, "CERELYTIC STAUNTON IS ACTIVE")) {
        return "STAUNTON PIECES ARE ACTIVE";
    }
    return text;
}

static void chess_draw_text(
    CerCanvas *c, int x, int y, const char *text, int pixel_height, uint32_t color
) {
    cer_draw_text(c, x, y, chess_debranded_label(text), pixel_height, color);
}

static void chess_draw_text_center(
    CerCanvas *c, int center_x, int y, const char *text, int pixel_height, uint32_t color
) {
    cer_draw_text_center(c, center_x, y, chess_debranded_label(text), pixel_height, color);
}

static void chess_draw_brand_mark(
    CerCanvas *c, int cx, int cy, int size, uint32_t accent, uint32_t ink, uint32_t surface
) {
    (void)ink;
    cer_draw_icon(c, CER_ICON_BOARD, cx, cy, size, accent, surface);
}

static void chess_set_toast(App *a, const char *message, int tone) {
    set_toast(a, chess_debranded_label(message), tone);
}

#define cer_draw_text chess_draw_text
#define cer_draw_text_center chess_draw_text_center
#define cer_draw_brand_mark chess_draw_brand_mark
#define set_toast chess_set_toast

#endif
