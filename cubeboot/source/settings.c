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

// The six stock save-icon shades baked into the IPL, in the order of its own color table
// (SAVE_COLOR_* in patches/source/menu.h). Only menu_cube_color accepts these -- they are
// whole 4-shade palettes, not single colors.
static const char *save_palette_names[] = {
    "blue", "green", "yellow", "orange", "red", "purple",
};

// on/off switches. Written as `on`/`off` in the file, with 1/0, yes/no and true/false taken
// as well so a config carried over from another tool still reads. Anything else, or a
// missing key, leaves the setting at its default -- a typo must not silently flip a switch.
static u32 ini_get_bool(ini_t *conf, const char *key, u32 fallback) {
    const char *raw = ini_get(conf, "cubeboot", key);
    if (raw == NULL) return fallback;

    if (strcasecmp(raw, "on") == 0 || strcasecmp(raw, "yes") == 0 ||
        strcasecmp(raw, "true") == 0 || strcmp(raw, "1") == 0) {
        iprintf("Found %s = on\n", key);
        return 1;
    }

    if (strcasecmp(raw, "off") == 0 || strcasecmp(raw, "no") == 0 ||
        strcasecmp(raw, "false") == 0 || strcmp(raw, "0") == 0) {
        iprintf("Found %s = off\n", key);
        return 0;
    }

    iprintf("Ignoring unreadable %s = %s (keeping %s)\n", key, raw, fallback ? "on" : "off");
    return fallback;
}

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
    // Long texts scroll by default, after holding still for 2 seconds, one character
    // every 10 frames.
    settings.text_scroll_enabled = 1;
    settings.text_scroll_delay_s = 2;
    settings.big_titles_scroll_speed = 10;
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

    // Physical discs boot through Swiss by default: that is what carries IGR (so the reset
    // combo comes back here instead of to the stock IPL) and what makes an out-of-region
    // disc boot at all. Turning it off hands the disc to the console's own apploader, which
    // is the stock experience minus both of those.
    settings.swiss_on_dvd_boot = ini_get_bool(conf, "swiss_on_dvd_boot", 1);

    settings.progressive_enabled = ini_get_bool(conf, "force_progressive", 0);
    settings.force_widescreen = ini_get_bool(conf, "force_widescreen", 0);

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

    // Auto-scroll of long menu texts. Not ini_get_bool on purpose: a number here is the
    // delay in seconds before a long title starts scrolling, so `0` and `1` mean seconds,
    // not off/on. `on`/`yes`/`true` keep the default delay; `off`/`no`/`false` disable it.
    // Anything else, or a missing key, keeps the defaults set above.
    const char *text_scroll_raw = ini_get(conf, "cubeboot", "text_scroll");
    if (text_scroll_raw != NULL) {
        char *num_end = NULL;
        long scroll_secs = strtol(text_scroll_raw, &num_end, 10);
        if (num_end != text_scroll_raw && *num_end == '\0' && scroll_secs >= 0) {
            if (scroll_secs > 600) scroll_secs = 600; // keep seconds*fps inside the menu's u16 frame timer
            settings.text_scroll_enabled = 1;
            settings.text_scroll_delay_s = (u32)scroll_secs;
            iprintf("Found text_scroll = %us delay\n", settings.text_scroll_delay_s);
        } else if (strcasecmp(text_scroll_raw, "on") == 0 || strcasecmp(text_scroll_raw, "yes") == 0 ||
                   strcasecmp(text_scroll_raw, "true") == 0) {
            settings.text_scroll_enabled = 1;
            iprintf("Found text_scroll = on\n");
        } else if (strcasecmp(text_scroll_raw, "off") == 0 || strcasecmp(text_scroll_raw, "no") == 0 ||
                   strcasecmp(text_scroll_raw, "false") == 0) {
            settings.text_scroll_enabled = 0;
            iprintf("Found text_scroll = off\n");
        } else {
            iprintf("Ignoring unreadable text_scroll = %s (keeping %s, %us)\n", text_scroll_raw,
                    settings.text_scroll_enabled ? "on" : "off", settings.text_scroll_delay_s);
        }
    }

    // Title marquee pace, in frames per character step: 1 is the fastest, larger numbers
    // are slower. Out-of-range or unreadable values keep the default.
    u32 big_titles_scroll_speed = 0;
    if (ini_sget(conf, "cubeboot", "big_titles_scroll_speed", "%u", &big_titles_scroll_speed)) {
        if (big_titles_scroll_speed >= 1 && big_titles_scroll_speed <= 255) {
            settings.big_titles_scroll_speed = big_titles_scroll_speed;
            iprintf("Found big_titles_scroll_speed = %u\n", big_titles_scroll_speed);
        } else {
            iprintf("Ignoring out-of-range big_titles_scroll_speed = %u (keeping %u)\n",
                    big_titles_scroll_speed, settings.big_titles_scroll_speed);
        }
    }

    settings.show_watermark = ini_get_bool(conf, "show_watermark", 0);
    settings.disable_mcp_select = ini_get_bool(conf, "disable_mcp_select", 0);
    settings.remember_last_game = ini_get_bool(conf, "remember_last_game", 0);

    // which storage the loader and the menu read from, most wanted first
    const char *device_order = ini_get(conf, "cubeboot", "device_order");
    if (device_order != NULL) {
        iprintf("Found device_order = %s\n", device_order);
        settings.device_order = (char*)device_order;
    }

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
