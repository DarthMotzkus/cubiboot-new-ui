#pragma once

#include <gctypes.h>
#include <ogc/gx.h>

// Menu color overrides, written in by cubeboot over the .data section (see set_patch_value
// in cubeboot/source/main.c). Encoded with the CFG_COLOR_* macros in settings_types.h;
// CFG_COLOR_UNSET means config.ini did not ask for anything and the stock look stands.
extern u32 theme_color;
extern u32 menu_cube_color;
extern u32 menu_box_color;
extern u32 menu_start_color;

// Which of the IPL's own six save-icon palettes the grid cubes are built from
// (a SAVE_COLOR_* value). A theme only re-tints that palette, it never replaces it.
int theme_get_cube_palette();

// Re-tint the four shades of the stock cube palette onto the configured color. `stock` holds
// the IPL's own pointers indexed by SAVE_ICON_SEL / SAVE_ICON / SAVE_EMPTY_SEL / SAVE_EMPTY;
// `out` is filled with the same indexing. Returns false when no color is configured, in
// which case `out` is untouched and the caller should keep using the IPL's pointers -- the
// stock palette must never be written through, the memory card menu shares it.
bool theme_tint_cube_colors(const GXColorS10 *stock[4], GXColorS10 out[4]);

// The two ends of the info-box gradient behind the filename/banner at the bottom of the game
// list, both derived from the single configured colour. Alpha comes back set to the stock
// values the box was designed around.
void theme_get_box_colors(GXColor *top, GXColor *bottom);

// Boot logo color: `cube_color` if it was set, otherwise whatever the theme implies.
// 0 means "leave the stock logo alone". Independent of theme_init.
u32 theme_get_boot_cube_color();

// Recolour the big block "PRESS START" on the pre-boot screen. Call immediately before
// draw_start_anim, every frame -- see theme.c for why that is the safe moment. No-op when no
// colour is configured, and on any IPL revision whose addresses have not been verified.
void theme_recolor_start_blocks();
