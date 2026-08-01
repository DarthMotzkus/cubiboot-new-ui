#include <gctypes.h>

#include "const.h"
#include "settings_types.h"

typedef struct settings {
    u32 cube_color;
    // Menu colors, in the CFG_COLOR_* encoding from settings_types.h. theme_color is the
    // umbrella the rest fall back to; the others are per-item overrides.
    u32 theme_color;
    u32 menu_cube_color;
    u32 menu_box_color;
    u32 menu_start_color;
    char *cube_logo;
    char *default_folder;
    u32 force_swiss_default;
    u32 show_watermark;
    u32 disable_mcp_select;
    u32 remember_last_game;
    u32 progressive_enabled;
    u32 preboot_delay_ms;
    u32 postboot_delay_ms;
    u32 load_from_ode_sd;
    char *default_program;
    char *boot_buttons[MAX_BUTTONS];
    menu_grid_type_t menu_grid_type;
} settings_t;

extern char *buttons_names[];
extern settings_t settings;

void load_settings();
