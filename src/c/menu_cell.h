#pragma once

#include <pebble.h>

// menu_cell_basic_draw, but an `icon` stays legible on the selected row.
//
// A highlighted cell gets a black background (see menu_layer_set_highlight_colors), which swallows
// the black icons the resource compiler produces from our PNGs. This draws the icon with its
// palette inverted instead, and puts it back afterwards.
//
// Pass `icon` as NULL for cells without one; behaviour matches menu_cell_basic_draw.
void menu_cell_draw_with_icon(
    GContext* ctx, const Layer* cell_layer, const char* title, const char* subtitle, GBitmap* icon
);