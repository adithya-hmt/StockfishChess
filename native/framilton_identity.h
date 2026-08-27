#ifndef FRAMILTON_IDENTITY_H
#define FRAMILTON_IDENTITY_H

/* Framilton uses the existing board icon as the in-app brand mark. */
static void framilton_draw_mark(
    CerCanvas *c, int cx, int cy, int size, uint32_t accent, uint32_t ink, uint32_t surface
) {
    (void)ink;
    cer_draw_icon(c, CER_ICON_BOARD, cx, cy, size, accent, surface);
}

#define cer_draw_brand_mark framilton_draw_mark

#endif
