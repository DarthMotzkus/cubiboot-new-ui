#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>

#include "sd.h"
#include "halt.h"
#include "helpers.h"

#include "ini.h"
#include "settings.h"

settings_t settings;

// Accepts the on/off spelling this option is documented with, as well as the
// 1/0 style the older settings use.
static u32 ini_get_bool(ini_t *conf, const char *key, u32 fallback) {
    const char *raw = ini_get(conf, "cubeboot", key);
    if (raw == NULL) return fallback;

    if (strcasecmp(raw, "on") == 0 || strcasecmp(raw, "true") == 0 ||
        strcasecmp(raw, "yes") == 0 || strcmp(raw, "1") == 0) {
        return 1;
    }

    if (strcasecmp(raw, "off") == 0 || strcasecmp(raw, "false") == 0 ||
        strcasecmp(raw, "no") == 0 || strcmp(raw, "0") == 0) {
        return 0;
    }

    iprintf("Ignoring unreadable %s = %s\n", key, raw);
    return fallback;
}

// The six stock save-icon shades baked into the IPL, in the order of its own color table
// (SAVE_COLOR_* in patches/source/menu.h). Only menu_cube_color accepts these -- they are
// whole 4-shade palettes, not single colors.
static const char *save_palette_names[] = {
    "blue", "green", "yellow", "orange", "red", "purple",
};

// Colors are hex RGB ("ff9801"), `random`, or -- when allow_palette is set -- one of the
// stock palette names above. Returns CFG_COLOR_UNSET when the key is missing or unreadable,
// which leaves the stock look alone.
static u32 ini_get_color(ini_t *conf, const char *key, int allow_palette) {
    const char *raw = ini_get(conf, "cubeboot", key);
    if (raw == NULL) return CFG_COLOR_UNSET;

    if (strcasecmp(raw, "random") == 0) {
        u32 color = CFG_COLOR_RGB(generate_random_color());
        iprintf("Found %s = random -> #%06x\n", key, CFG_COLOR_VALUE(color));
        return color;
    }

    if (allow_palette) {
        for (u32 i = 0; i < (sizeof(save_palette_names) / sizeof(char *)); i++) {
            if (strcasecmp(raw, save_palette_names[i]) == 0) {
                iprintf("Found %s = %s (stock palette %u)\n", key, raw, i);
                return CFG_COLOR_PALETTE(i);
            }
        }
    }

    u32 rgb = 0;
    if (sscanf(raw, "%x", &rgb) != 1) {
        iprintf("Ignoring unreadable %s = %s\n", key, raw);
        return CFG_COLOR_UNSET;
    }

    iprintf("Found %s = #%06x\n", key, rgb & 0x00FFFFFF);
    return CFG_COLOR_RGB(rgb);
}

char *buttons_names[] = {
    "left",  // LEFT	0x0001
    "right", // RIGHT	0x0002
    "down",  // DOWN	0x0004
    "up",    // UP		0x0008
    "_",     // Z		0x0010 (NOT ALLOWED) used to switch sound
    "_",     // R		0x0020 (NOT ALLOWED) used by bootloader
    "_",     // L		0x0040 (NOT ALLOWED) used by bootloader
    "_",     // origin  0x0080 (NOT ALLOWED) used by pad library
    "_",     // A		0x0100 (NOT ALLOWED) used to force menu
    "b",     // B		0x0200
    "_",     // X		0x0400 (NOT ALLOWED) used by bootloader
    "y",     // Y		0x0800
    "start", // START	0x1000
};

void load_settings() {
    memset(&settings, 0, sizeof(settings));
    // Default the menu layout in code so the banner grid works even without a
    // config.ini being present/readable; config.ini can still override it below.
    settings.menu_grid_type = MENU_GRID_SMALL_BANNERS;
    int config_size = get_file_size("/config.ini");
    if (config_size == SD_FAIL) return;

    void *config_buf = memalign(32, config_size + 1);
    if (config_buf == NULL) {
        prog_halt("Could not allocate buffer for config file\n");
        return;
    }

    if (load_file_buffer("/config.ini", config_buf) != SD_OK) {
        prog_halt("Could not find config file\n");
        return;
    }

    ((char*)config_buf)[config_size] = '\0';

    // iprintf("DUMP:\n");
    // iprintf("%s[END]\n\n", (char*)config_buf);

    ini_t *conf = ini_load(config_buf, config_size);

    // cube color
    const char *cube_color_raw = ini_get(conf, "cubeboot", "cube_color");
    if (cube_color_raw != NULL) {
        if (strcmp(cube_color_raw, "random") == 0) {
            settings.cube_color = generate_random_color();
        } else {
            int vars = sscanf(cube_color_raw, "%x", &settings.cube_color);
            if (vars == EOF) settings.cube_color = 0;
            iprintf("Found cube_color = #%x\n", settings.cube_color);
        }
    }

    // menu colors -- theme_color is the umbrella; everything below overrides it per item.
    // The menu resolves the fallbacks itself (see patches/source/theme.c), so an unset key
    // stays unset here.
    settings.theme_color = ini_get_color(conf, "theme_color", 0);
    settings.menu_cube_color = ini_get_color(conf, "menu_cube_color", 1);
    settings.menu_box_color = ini_get_color(conf, "menu_box_color", 0);
    settings.menu_start_color = ini_get_color(conf, "menu_start_color", 0);

    // cube logo
    const char *cube_logo = ini_get(conf, "cubeboot", "cube_logo");
    if (cube_logo != NULL) {
        iprintf("Found cube_logo = %s\n", cube_logo);
        settings.cube_logo = (char*)cube_logo;
    }

    // default folder
    const char *default_folder = ini_get(conf, "cubeboot", "default_folder");
    if (default_folder != NULL) {
        iprintf("Found default_folder = %s\n", default_folder);
        settings.default_folder = (char*)default_folder;
    }

    // default program
    const char *default_program = ini_get(conf, "cubeboot", "default_program");
    if (default_program != NULL) {
        iprintf("Found default_program = %s\n", default_program);
        settings.default_program = (char*)default_program;
    }

    // swiss enable
    int force_swiss_default = 0;
    if (!ini_sget(conf, "cubeboot", "force_swiss_default", "%d", &force_swiss_default)) {
        settings.force_swiss_default = 0;
    } else {
        iprintf("Found force_swiss_default = %d\n", force_swiss_default);
        settings.force_swiss_default = force_swiss_default;
    }

    // progressive enable
    int progressive_enabled = 0;
    if (!ini_sget(conf, "cubeboot", "force_progressive", "%d", &progressive_enabled)) {
        settings.progressive_enabled = 0;
    } else {
        iprintf("Found progressive_enabled = %d\n", progressive_enabled);
        settings.progressive_enabled = progressive_enabled;
    }

    // preboot delay
    u32 preboot_delay_ms = 0;
    if (!ini_sget(conf, "cubeboot", "preboot_delay_ms", "%u", &preboot_delay_ms)) {
        settings.preboot_delay_ms = 0;
    } else {
        iprintf("Found preboot_delay_ms = %u\n", preboot_delay_ms);
        settings.preboot_delay_ms = preboot_delay_ms;
    }

    // postboot delay
    u32 postboot_delay_ms = 0;
    if (!ini_sget(conf, "cubeboot", "postboot_delay_ms", "%u", &postboot_delay_ms)) {
        settings.postboot_delay_ms = 0;
    } else {
        iprintf("Found postboot_delay_ms = %u\n", postboot_delay_ms);
        settings.postboot_delay_ms = postboot_delay_ms;
    }

    // show_watermark
    int show_watermark = 0;
    if (!ini_sget(conf, "cubeboot", "show_watermark", "%d", &show_watermark)) {
        settings.show_watermark = 0;
    } else {
        iprintf("Found show_watermark = %d\n", show_watermark);
        settings.show_watermark = show_watermark;
    }

    // disable_mcp_select
    int disable_mcp_select = 0;
    if (!ini_sget(conf, "cubeboot", "disable_mcp_select", "%d", &disable_mcp_select)) {
        settings.disable_mcp_select = 0;
    } else {
        iprintf("Found disable_mcp_select = %d\n", disable_mcp_select);
        settings.disable_mcp_select = disable_mcp_select;
    }

    // remember last played game
    int remember_last_game = 0;
    if (!ini_sget(conf, "cubeboot", "remember_last_game", "%d", &remember_last_game)) {
        settings.remember_last_game = 0;
    } else {
        iprintf("Found remember_last_game = %d\n", remember_last_game);
        settings.remember_last_game = remember_last_game;
    }

    // read games straight off the SD card inside a GC Loader style ODE
    settings.load_from_ode_sd = ini_get_bool(conf, "load_from_ode_sd", 0);
    iprintf("Found load_from_ode_sd = %d\n", settings.load_from_ode_sd);

    // button presses
    for (int i = 0; i < (sizeof(buttons_names) / sizeof(char *)); i++) {
        char *button_name = buttons_names[i];

        // ignore disabled buttons
        if (*button_name == '_') {
            continue;
        }

        char button_config_name[255];
        sprintf(button_config_name, "button_%s", button_name);

        const char *dol_path = ini_get(conf, "cubeboot", button_config_name);
        if (dol_path != NULL) {
            iprintf("Found %s = %s\n", button_config_name, dol_path);

            settings.boot_buttons[i] = (char*)dol_path;
        }
    }

    // menu grid type
    settings.menu_grid_type = MENU_GRID_SMALL_BANNERS;
    const char *menu_grid_type = ini_get(conf, "cubeboot", "menu_grid_type");
    if (menu_grid_type != NULL) {
        if (strcmp(menu_grid_type, "square_icons") == 0) {
            settings.menu_grid_type = MENU_GRID_SQUARE_ICONS;
        } else if (strcmp(menu_grid_type, "banners") == 0) {
            settings.menu_grid_type = MENU_GRID_BANNERS;
        } else if (strcmp(menu_grid_type, "small_banners") == 0) {
            settings.menu_grid_type = MENU_GRID_SMALL_BANNERS;
        }
    }

    // // must stay allocated!!
    // free(config_buf);
}
