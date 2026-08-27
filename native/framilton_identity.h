#ifndef FRAMILTON_IDENTITY_H
#define FRAMILTON_IDENTITY_H

/* Runtime identity layer for the existing generated UI fragments. */
static unsigned framilton_label_hash(const char *text) {
    unsigned hash = 2166136261u;
    if (!text) return 0u;
    while (*text) {
        hash ^= (unsigned char)*text++;
        hash *= 16777619u;
    }
    return hash;
}

static const char *framilton_label(const char *text) {
    switch (framilton_label_hash(text)) {
        case 0x4686f419u:
            return "FRAMILTON";
        case 0xcd829bf5u:
            return "FRAMILTON CHESS";
        case 0x715ed08bu:
            return "FRAMILTON STAUNTON IS ACTIVE";
        default:
            return text;
    }
}

static void framilton_draw_text(
    CerCanvas *c, int x, int y, const char *text, int pixel_height, uint32_t color
) {
    cer_draw_text(c, x, y, framilton_label(text), pixel_height, color);
}

static void framilton_draw_text_center(
    CerCanvas *c, int center_x, int y, const char *text, int pixel_height, uint32_t color
) {
    cer_draw_text_center(c, center_x, y, framilton_label(text), pixel_height, color);
}

static void framilton_draw_mark(
    CerCanvas *c, int cx, int cy, int size, uint32_t accent, uint32_t ink, uint32_t surface
) {
    (void)ink;
    cer_draw_icon(c, CER_ICON_BOARD, cx, cy, size, accent, surface);
}

static void framilton_set_toast(App *a, const char *message, int tone) {
    set_toast(a, framilton_label(message), tone);
}

#define cer_draw_text framilton_draw_text
#define cer_draw_text_center framilton_draw_text_center
#define cer_draw_brand_mark framilton_draw_mark
#define set_toast framilton_set_toast

#endif
