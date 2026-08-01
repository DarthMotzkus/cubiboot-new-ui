#pragma once

typedef enum {
    MENU_GRID_SQUARE_ICONS,
    MENU_GRID_BANNERS,
    MENU_GRID_SMALL_BANNERS
} menu_grid_type_t;

// Colors travel from config.ini into the injected menu as a plain u32 (set_patch_value only
// moves words). 0 has to keep meaning "key absent", so a configured value carries a tag in
// the top byte instead of living in the low 24 bits alone -- that keeps `000000` usable as
// black, and lets one key accept either a hex color or a stock palette name.
#define CFG_COLOR_UNSET         0u
#define CFG_COLOR_TAG_RGB       0x01000000u
#define CFG_COLOR_TAG_PALETTE   0x02000000u

#define CFG_COLOR_RGB(rgb)      (CFG_COLOR_TAG_RGB | ((u32)(rgb) & 0x00FFFFFFu))
#define CFG_COLOR_PALETTE(idx)  (CFG_COLOR_TAG_PALETTE | ((u32)(idx) & 0x00FFFFFFu))

#define CFG_COLOR_IS_RGB(v)     (((v) & CFG_COLOR_TAG_RGB) != 0)
#define CFG_COLOR_IS_PALETTE(v) (((v) & CFG_COLOR_TAG_PALETTE) != 0)
#define CFG_COLOR_VALUE(v)      ((v) & 0x00FFFFFFu)
