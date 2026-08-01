#include "picolibc.h"
#include "structs.h"

#include "attr.h"
#include "util.h"
#include "settings_types.h"

#include "ipl.h"
#include "menu.h"
#include "theme.h"

// Color overrides from config.ini. cubeboot patches these in by symbol name, so they have to
// stay non-static and in .data.
__attribute_data__ u32 theme_color = CFG_COLOR_UNSET;
__attribute_data__ u32 menu_cube_color = CFG_COLOR_UNSET;
__attribute_data__ u32 menu_box_color = CFG_COLOR_UNSET;
__attribute_data__ u32 menu_start_color = CFG_COLOR_UNSET;

// The stock look, kept both as the fallback and as the reference the derivation is
// calibrated against. These are the values custom_gameselect_menu used to inline.
#define STOCK_BOX_TOP_RGB    0x6e00b3
#define STOCK_BOX_BOTTOM_RGB 0x800057
#define STOCK_BOX_TOP_ALPHA    0xc8
#define STOCK_BOX_BOTTOM_ALPHA 0xb4

// GRRLIB's HSL helpers work on packed 0xRRGGBBAA, config values are 24-bit RGB.
#define RGB24_TO_RGBA(rgb) ((((u32)(rgb)) << 8) | 0xFF)
#define RGBA_TO_RGB24(rgba) ((((u32)(rgba)) >> 8) & 0x00FFFFFFu)

// A hue rotation plus a saturation/lightness scale. Applying a tint to the color it was
// derived from reproduces the target exactly, so a whole palette can be moved onto a new
// color while keeping its internal relationships -- which shade is the bright one, which is
// the dimmed one -- instead of being flattened to a single value.
typedef struct {
    s16 hue_shift;
    float sat_mult;
    float lum_mult;
} tint_t;

static u8 clamp_u8(float v) {
    if (v <= 0.0f) return 0;
    if (v >= 255.0f) return 0xFF;
    return (u8)(v + 0.5f);
}

static tint_t make_tint(u32 ref_rgb, u32 target_rgb) {
    u32 ref = GRRLIB_RGBToHSL(RGB24_TO_RGBA(ref_rgb));
    u32 target = GRRLIB_RGBToHSL(RGB24_TO_RGBA(target_rgb));

    tint_t tint;
    tint.hue_shift = (s16)H(target) - (s16)H(ref);
    // A greyscale or black reference has no hue or brightness to scale from, so fall back to
    // passing the channel through untouched rather than dividing by zero.
    tint.sat_mult = S(ref) != 0 ? (float)S(target) / (float)S(ref) : 1.0f;
    tint.lum_mult = L(ref) != 0 ? (float)L(target) / (float)L(ref) : 1.0f;
    return tint;
}

static u32 apply_tint(const tint_t *tint, u32 rgb) {
    u32 hsl = GRRLIB_RGBToHSL(RGB24_TO_RGBA(rgb));

    u8 hue = (u8)(((s16)H(hsl) + tint->hue_shift) & 0xFF); // hue wraps
    u8 sat = clamp_u8((float)S(hsl) * tint->sat_mult);
    u8 lum = clamp_u8((float)L(hsl) * tint->lum_mult);

    return RGBA_TO_RGB24(GRRLIB_HSLToRGB(HSLA(hue, sat, lum, 0xFF)));
}

// TEV register colors are s10, so a stock shade can legitimately sit above 0xFF. Normalise it
// into 8-bit for the hue work and hand back how far it overshot, so the scale can be put back
// afterwards and an overbright shade stays overbright instead of clipping to plain white.
static float normalize_color10(const GXColorS10 *src, rgb_color *out) {
    s16 peak = src->r;
    if (src->g > peak) peak = src->g;
    if (src->b > peak) peak = src->b;

    float overdrive = peak > 0xFF ? (float)peak / 255.0f : 1.0f;

    out->parts.r = clamp_u8((float)src->r / overdrive);
    out->parts.g = clamp_u8((float)src->g / overdrive);
    out->parts.b = clamp_u8((float)src->b / overdrive);
    out->parts.a = 0xFF;

    return overdrive;
}

static void tint_color10(const tint_t *tint, const GXColorS10 *src, GXColorS10 *dst) {
    rgb_color in;
    float overdrive = normalize_color10(src, &in);

    rgb_color out;
    out.color = RGB24_TO_RGBA(apply_tint(tint, RGBA_TO_RGB24(in.color)));

    float limit = 255.0f * overdrive;
    if (limit > 1023.0f) limit = 1023.0f; // s10 ceiling

    float r = (float)out.parts.r * overdrive;
    float g = (float)out.parts.g * overdrive;
    float b = (float)out.parts.b * overdrive;

    dst->r = (s16)((r > limit ? limit : r) + 0.5f);
    dst->g = (s16)((g > limit ? limit : g) + 0.5f);
    dst->b = (s16)((b > limit ? limit : b) + 0.5f);
    dst->a = src->a; // alpha encodes the shade's role (selected vs dimmed), not its hue
}

// Resolve one item: its own override wins, then the theme, then nothing.
static bool resolve_color(u32 item, u32 *rgb_out) {
    if (CFG_COLOR_IS_RGB(item)) {
        *rgb_out = CFG_COLOR_VALUE(item);
        return true;
    }

    if (CFG_COLOR_IS_RGB(theme_color)) {
        *rgb_out = CFG_COLOR_VALUE(theme_color);
        return true;
    }

    return false;
}

int theme_get_cube_palette() {
    if (CFG_COLOR_IS_PALETTE(menu_cube_color)) {
        u32 index = CFG_COLOR_VALUE(menu_cube_color);
        if (index <= SAVE_COLOR_PURPLE) return (int)index;
        OSReport("Ignoring out of range cube palette %u\n", index);
    }

    return SAVE_COLOR_PURPLE;
}

bool theme_tint_cube_colors(const GXColorS10 *stock[4], GXColorS10 out[4]) {
    u32 target_rgb;
    if (!resolve_color(menu_cube_color, &target_rgb)) return false;

    // SAVE_ICON is the plain has-a-banner shade -- the one the eye reads as "the cube
    // color" -- so it is what the configured value maps onto exactly.
    const GXColorS10 *reference = stock[SAVE_ICON];
    if (reference == NULL) return false;

    // Normalised the same way tint_color10 will normalise it, so an overbright stock shade
    // yields the same hue here as it does there.
    rgb_color ref;
    normalize_color10(reference, &ref);

    tint_t tint = make_tint(RGBA_TO_RGB24(ref.color), target_rgb);

    for (int i = 0; i < 4; i++) {
        if (stock[i] == NULL) return false;
        tint_color10(&tint, stock[i], &out[i]);
    }

    OSReport("Menu cubes tinted to #%06x\n", target_rgb);
    return true;
}

// One configured colour drives the whole panel: it is the strong end, and the far end is that
// same colour dimmed by however much the stock gradient dims.
//
// Measuring stock: top 6e00b3 is H=196 S=255 L=89, bottom 800057 is H=226 S=255 L=64. So the
// far end keeps the saturation and drops to ~72% of the lightness. Stock also swings the hue
// +30 (about +42 degrees, purple -> magenta), and that part is deliberately dropped: rotating
// the same swing onto another hue turns the gradient into a clash, with orange fading to lime.
// Hue and saturation therefore stay put and only the lightness falls off.
static u32 derive_box_far_end(u32 near_rgb) {
    u32 stock_near = GRRLIB_RGBToHSL(RGB24_TO_RGBA(STOCK_BOX_TOP_RGB));
    u32 stock_far = GRRLIB_RGBToHSL(RGB24_TO_RGBA(STOCK_BOX_BOTTOM_RGB));
    u32 hsl = GRRLIB_RGBToHSL(RGB24_TO_RGBA(near_rgb));

    float sat_mult = S(stock_near) != 0 ? (float)S(stock_far) / (float)S(stock_near) : 1.0f;
    float lum_mult = L(stock_near) != 0 ? (float)L(stock_far) / (float)L(stock_near) : 1.0f;

    u8 sat = clamp_u8((float)S(hsl) * sat_mult);
    u8 lum = clamp_u8((float)L(hsl) * lum_mult);

    return RGBA_TO_RGB24(GRRLIB_HSLToRGB(HSLA(H(hsl), sat, lum, 0xFF)));
}

void theme_get_box_colors(GXColor *top, GXColor *bottom) {
    u32 top_rgb = STOCK_BOX_TOP_RGB;
    u32 bottom_rgb = STOCK_BOX_BOTTOM_RGB;

    u32 configured;
    if (resolve_color(menu_box_color, &configured)) {
        top_rgb = configured;
        bottom_rgb = derive_box_far_end(configured);
    }

    rgb_color color;

    color.color = RGB24_TO_RGBA(top_rgb);
    copy_color(color, top);
    top->a = STOCK_BOX_TOP_ALPHA;

    color.color = RGB24_TO_RGBA(bottom_rgb);
    copy_color(color, bottom);
    bottom->a = STOCK_BOX_BOTTOM_ALPHA;
}

// ---------------------------------------------------------------------------------------
// The big block "PRESS START" on the pre-boot screen.
//
// It is drawn by the stock BIOS's draw_start_anim, which takes only an alpha byte -- there is
// no colour parameter to intercept. Disassembling it (all seven revisions) shows it loops over
// `count` blocks and, for each one, builds two GXColorS10 on its stack out of two parallel
// GXColor arrays, writes them into the block model's mat[0].tev_color[0..1], draws, and puts
// the material's original pointers back.
//
// Two facts from that disassembly make recolouring safe:
//
//  * Each array is reached through one tiny `lis/addi/blr` getter, and those getters are
//    called from draw_start_anim and nowhere else in the whole code region. So writing the
//    arrays cannot disturb anything else in the BIOS.
//  * Only R, G and B are read as colour. Byte 3 of the first array is a per-block intensity
//    that gets folded into the model alpha (`lbz r3,3(r26)` -> `sth r0,116(r24)`), which is
//    what drives the fly-in and fade. Leaving that byte alone keeps the animation intact.
//
// So: write R/G/B absolutely, every frame, immediately before the draw. Absolute writes mean
// a tint never compounds on itself, and doing it per frame means it does not matter when the
// BIOS populates the arrays or how `count` grows during the animation.
typedef struct {
    u32 color_a;    // GXColor[]: rgb = block colour, byte 3 = intensity (do not touch)
    u32 color_b;    // GXColor[]: rgb only
    u32 count;      // u32: how many blocks are live this frame
    u32 capacity;   // entries between the two arrays -- a paranoia bound on count
} start_blocks_t;

// Addresses decoded from the getters in each dump, then cross-checked: every revision was
// identified by its r2/r13 pair (matching get_ipl_revision) *and* by CRC32 of its code region
// against bios_table[] in cubeboot/source/ipl.c.
//
// These seven are every revision cubiboot can run on. get_ipl_revision() also knows
// NTSC 1.0-002, PAL 1.0-002, DEV 1.0 and TDEV 1.1 -- the NPDP/dev-kit BIOSes -- but none of
// those is in bios_table[], so load_ipl halts with "Bad IPL image" long before the menu is
// injected, and there is no linker script supplying their relocs either. The default: below
// is therefore unreachable on real hardware; it exists so an unrecognised image degrades to
// the stock colour instead of writing to a guessed address.
static bool get_start_blocks(start_blocks_t *out) {
    switch (get_ipl_revision()) {
    case IPL_NTSC_10_001:
        out->color_a = 0x814a0a44; out->color_b = 0x814a20e4; out->count = 0x8145d908;
        out->capacity = 1448; return true;
    case IPL_NTSC_11_001:
        out->color_a = 0x814c5d30; out->color_b = 0x814c74a0; out->count = 0x81481750;
        out->capacity = 1500; return true;
    case IPL_NTSC_12_001:
        out->color_a = 0x814c7310; out->color_b = 0x814c8a80; out->count = 0x81483828;
        out->capacity = 1500; return true;
    case IPL_NTSC_12_101:
        out->color_a = 0x814c77b0; out->color_b = 0x814c8f20; out->count = 0x81483ca8;
        out->capacity = 1500; return true;
    case IPL_PAL_10_001:
        out->color_a = 0x814fa28c; out->color_b = 0x814fc410; out->count = 0x814ad5f0;
        out->capacity = 2145; return true;
    case IPL_MPAL_11:
        out->color_a = 0x814c09f0; out->color_b = 0x814c2160; out->count = 0x8147c410;
        out->capacity = 1500; return true;
    case IPL_PAL_12_101:
        out->color_a = 0x814fba4c; out->color_b = 0x814fdbd0; out->count = 0x814af8e8;
        out->capacity = 2145; return true;
    default:
        return false;
    }
}

// The stock shades, captured from the first block before anything is written over it. Used as
// the tint reference so the two arrays keep their original relationship to each other, the
// same self-calibration the grid cubes use.
__attribute_data__ static bool start_stock_captured = false;
__attribute_data__ static u32 start_stock_a = 0;
__attribute_data__ static u32 start_stock_b = 0;

void theme_recolor_start_blocks() {
    u32 target_rgb;
    if (!resolve_color(menu_start_color, &target_rgb)) return;

    start_blocks_t blocks;
    if (!get_start_blocks(&blocks)) return;

    u32 count = *(volatile u32*)blocks.count;
    if (count == 0 || count > blocks.capacity) return; // nothing live, or not what we expect

    GXColor *color_a = (GXColor*)blocks.color_a;
    GXColor *color_b = (GXColor*)blocks.color_b;

    if (!start_stock_captured) {
        rgb_color a, b;
        a.parts.r = color_a[0].r; a.parts.g = color_a[0].g; a.parts.b = color_a[0].b;
        b.parts.r = color_b[0].r; b.parts.g = color_b[0].g; b.parts.b = color_b[0].b;
        a.parts.a = b.parts.a = 0xFF;

        start_stock_a = RGBA_TO_RGB24(a.color);
        start_stock_b = RGBA_TO_RGB24(b.color);
        start_stock_captured = true;
        OSReport("PRESS START stock shades = #%06x / #%06x (%u blocks)\n",
                 start_stock_a, start_stock_b, count);
    }

    tint_t tint = make_tint(start_stock_a, target_rgb);
    rgb_color out_a, out_b;
    out_a.color = RGB24_TO_RGBA(target_rgb);
    out_b.color = RGB24_TO_RGBA(apply_tint(&tint, start_stock_b));

    for (u32 i = 0; i < count; i++) {
        // Byte 3 of color_a is the block's intensity, not colour -- writing it would freeze
        // the fly-in. color_b's alpha is never read (draw_start_anim hardcodes 0xFF).
        color_a[i].r = out_a.parts.r;
        color_a[i].g = out_a.parts.g;
        color_a[i].b = out_a.parts.b;

        color_b[i].r = out_b.parts.r;
        color_b[i].g = out_b.parts.g;
        color_b[i].b = out_b.parts.b;
    }
}

u32 theme_get_boot_cube_color() {
    extern u32 cube_color;

    // cube_color predates the theme and stays raw 24-bit RGB with 0 meaning unset, so a
    // config that only sets it keeps behaving exactly as before.
    if (cube_color != 0) return cube_color;

    if (CFG_COLOR_IS_RGB(theme_color)) {
        u32 rgb = CFG_COLOR_VALUE(theme_color);
        // Pure black would read as "unset" downstream; nudge it so a black theme still
        // recolors the logo instead of silently leaving it stock.
        return rgb != 0 ? rgb : 0x010101;
    }

    return 0;
}
