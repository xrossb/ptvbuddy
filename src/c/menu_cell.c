#include "menu_cell.h"

// Number of entries in a palettized bitmap's palette, or 0 for other formats.
static uint8_t palette_size(GBitmapFormat format) {
    switch (format) {
    case GBitmapFormat1BitPalette:
        return 2;
    case GBitmapFormat2BitPalette:
        return 4;
    case GBitmapFormat4BitPalette:
        return 16;
    default:
        return 0;
    }
}

// Swaps every opaque palette entry for its complement, leaving alpha (and therefore the icon's
// antialiased edges) alone. Applying this twice is a no-op, so a draw can bracket its call with it.
static void invert_palette(GBitmap* bitmap) {
    GColor* palette = gbitmap_get_palette(bitmap);
    uint8_t size = palette_size(gbitmap_get_format(bitmap));

    if (!palette) {
        return;
    }

    for (uint8_t i = 0; i < size; i++) {
        // Transparent entries are the icon's background; inverting them would paint it solid.
        if (palette[i].a == 0) {
            continue;
        }
        // Channels are 2 bits wide, so 3 is white and 0 is black.
        palette[i].r = 3 - palette[i].r;
        palette[i].g = 3 - palette[i].g;
        palette[i].b = 3 - palette[i].b;
    }
}

void menu_cell_draw_with_icon(
    GContext* ctx, const Layer* cell_layer, const char* title, const char* subtitle, GBitmap* icon
) {
    // The cell layer reports its own highlight status rather than asking the menu layer which row
    // is selected, because draw_row also runs for rows the scroll animation is passing over.
    if (icon && menu_cell_layer_is_highlighted(cell_layer)) {
        invert_palette(icon);
        menu_cell_basic_draw(ctx, cell_layer, title, subtitle, icon);
        invert_palette(icon);
        return;
    }

    menu_cell_basic_draw(ctx, cell_layer, title, subtitle, icon);
}