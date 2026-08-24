// The stock bottom prompt bar -- "(B) ... Cancel" and friends.
//
// The bar is IPL data, not something Cubiboot draws: one GLH0 group per language, holding an
// element table (the round pills, the button glyphs, the animated analog stick, the "..."
// separators) and a label table (position, size, and an offset to the label's string). The
// group header describes all three tables, so one pointer reaches everything:
//
//   +0x00 'GLH0'   +0x04 -> elements (16B each, count at +0x10, first is 'bac1')
//   +0x08 -> labels (36B each, count at +0x12, first is 'txt1')   +0x0c -> string pool
//
// The group sits at a different address in every IPL revision, so it is found by shape rather
// than by a table of seven addresses: nothing else in the image is a GLH0 whose element table
// starts with 'bac1' and whose label table starts with 'txt1'. An NTSC image holds two (the
// Japanese bar first, then the English one) and a PAL image holds one carrying a different
// language in each slot, so every match is corrected, not just the first.
//
// Which label record a given screen draws is not knowable from the data -- the pairing is in
// code that runs per screen. That is why nothing here is keyed to a record: the labels are
// matched by their TEXT, and each one is moved by exactly the same delta as the glyph it
// belongs to. Whatever the label box's placement rules are, moving both ends by one delta
// preserves them, so the button-to-text distance stays the one the IPL shipped.

#include <string.h>

#include "prompt.h"
#include "attr.h"
#include "structs.h"
#include "os.h"
#include "reloc.h" // OSReport

extern void (*prep_text_mode)();
extern void (*setup_tex_draw)(s32 unk0, s32 unk1, s32 unk2);
extern void (*draw_named_tex)(u32 type, void *blob, GXColor *color, s16 x, s16 y);
void draw_text(char *s, s16 size, u16 x, u16 y, GXColor *color);

// The IPL image as cubeboot lays it down: the whole 2 MB at 0x81300000.
#define IPL_LO 0x81300000u
#define IPL_HI 0x81500000u

// --- layout ---------------------------------------------------------------------------
// Element-space x of each glyph, evenly spaced, in the order the row reads: the analog
// stick, then A, then B. The stock bar puts B at 0x0f40 and A at 0x1940 with the stick off
// to the left, which leaves no room for three of them, so all three are placed here.
// The game list draws its stick and its A at the very positions the BIOS sub-screens use for
// theirs, so the two rows read identically. STICK_X is where those screens put their stick
// (bac1); the A comes from buta itself, after the swap above.
#define STICK_X 0x0520

// The label sits this far below its glyph in every group; lifting by exactly that levels
// the two. The dots used to stand between glyph and label, so with them gone the label
// closes up by the width of one dot element.
// Vertical trim on every label, measured off screenshots rather than reasoned about: the
// bar's own gap (0x10) lifted the text clear above the glyph's centre and half of it was
// still high, so the text is nudged DOWN from where the IPL had it. One pixel is about 0x0f
// in this space, which is the step this was moved by.
#define LABEL_DROP  0x07
#define LABEL_CLOSE 0x0140


typedef struct {
    u32 type;
    u32 elems_offset;
    u32 labels_offset;
    u32 pool_offset;
    u16 elem_count;
    u16 label_count;
} prompt_group;

// One label record. Only the fields touched here are named; the alignment and spacing bytes
// in between are left exactly as the IPL wrote them.
typedef struct {
    u32 tag;
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u8  unk0c[0x12];
    u16 length;
    u32 str_offset; // from the start of THIS record, not from the pool
} prompt_label;

_Static_assert(sizeof(prompt_label) == 0x24, "label record must be 36 bytes");

// The counter the IPL's own prompt bar pulses from: a frame counter whose low byte it walks
// as a triangle (BS2 reads it, takes it modulo 256 and splits at 127). Reading the same one
// is what puts Cubiboot's A button, its Z pill and the stick in step with the stock B
// instead of each breathing at its own rate.
__attribute_reloc__ u16 *prompt_pulse;

// The two colours the current screen tints its prompts with -- the A's green and the B's
// magenta. They are per-screen state, and the game list IS the Game Play screen as far as
// the IPL is concerned, so reading them here gives the A exactly the green the BIOS screens
// draw instead of a hand-picked one. Six signed halfwords at +420: RGB, then RGB.
__attribute_reloc__ u8 **prompt_slot_colors;
// Where the two colours sit inside that struct. NTSC 1.0 keeps them at 364 and every later
// revision at 420, which is why reading 420 on a 1.0 came back as something that was not a
// colour at all.
__attribute_reloc__ u8 *prompt_slot_color_off;

// 1 on an NTSC image, 2 on a PAL one. Never zero, whatever it comes to mean: cubeboot's
// relocation walk reads a zero-valued symbol as a broken reloc and halts the boot.
__attribute_reloc__ u8 *prompt_region;
#define PROMPT_REGION_PAL 2

// The vertical placement carries between the two regions, the horizontal does not. On PAL the
// stick and the A sit eight pixels further right than NTSC puts them, and their labels fifteen
// further left -- which lands the labels seven left of the NTSC row, since they follow their
// glyph. Both were trimmed against screenshots, a pixel being about 0x0f here.
#define PAL_GLYPH_DX   0x78
#define PAL_LABEL_DX (-0xe1)
static inline s16 pal_glyph_dx(void) {
    return (u32)prompt_region == PROMPT_REGION_PAL ? PAL_GLYPH_DX : 0;
}
// The A and its label carry their own trim, ten pixels right of the row on PAL and six on
// NTSC. The stick and Selection do not follow it.
#define PAL_A_EXTRA_DX  0x96
#define NTSC_A_EXTRA_DX 0x5a
static inline s16 a_dx(void) {
    return (u32)prompt_region == PROMPT_REGION_PAL
         ? (s16)(pal_glyph_dx() + PAL_A_EXTRA_DX)
         : NTSC_A_EXTRA_DX;
}
static inline s16 pal_label_dx(void) {
    return (u32)prompt_region == PROMPT_REGION_PAL ? PAL_LABEL_DX : 0;
}


// Used only when the read above does not come back with a colour (NTSC 1.0).
#define A_FALLBACK_R 0x35
#define A_FALLBACK_G 0xC1
#define A_FALLBACK_B 0x5C

__attribute_data__ static prompt_group *bar = NULL;

blob_element *blob_find_element(void *blob, u32 tag) {
    blob_element *e = (blob_element*)((u8*)blob + *(u32*)((u8*)blob + 4));
    u16 count = *(u16*)((u8*)blob + 0x10);
    for (u16 i = 0; i < count; i++, e++)
        if (e->tag == tag) return e;
    return NULL;
}

// 0..255 and back down again, over the same 256 counts the stock bar uses, so anything drawn
// with this is in phase with the B the IPL is drawing beside it.
static u8 pulse_alpha(u8 ui_alpha) {
    // The stock bar's own curve, read out of BS2: a triangle between 100 and 255 over 256
    // ticks of this counter, starting at 255 and falling. Reproduced exactly -- an invented
    // swing is what left the A and the Z breathing out of step with the B.
    u32 c = *prompt_pulse;
    u32 half = c & 0x7f;
    u32 a = (c & 0xff) <= 127 ? 255 - (half * 155) / 127
                              : 100 + (half * 155) / 127;
    return (u8)((a * ui_alpha) / 255);
}

// Of the screen's two prompt colours, the A's is the green one. Picked by which has more
// green than red rather than by index, so it cannot come out as the B's magenta.
static bool is_green(const s16 *c) {
    for (int i = 0; i < 3; i++)
        if (c[i] < 0 || c[i] > 255) return false;      // not a colour at all

    return c[1] >= 0x40 && c[1] > c[0] && c[1] > c[2]; // and it is the green one
}

static void slot_color_green(GXColor *out) {
    u8 *base = *prompt_slot_colors;
    if (base != NULL) {
        s16 *c = (s16*)(base + (u32)prompt_slot_color_off);
        s16 *pick = NULL;

        // Of the screen's two prompt colours, take the A's: the green one. Picked by content
        // rather than by index, so it cannot come out as the B's magenta -- and validated,
        // because this variable is found per revision and on NTSC 1.0 the address landed on
        // something that is not a colour (the A came out unpainted and flickering).
        if (is_green(&c[0])) pick = &c[0];
        else if (is_green(&c[3])) pick = &c[3];

        if (pick != NULL) {
            out->r = (u8)pick[0];
            out->g = (u8)pick[1];
            out->b = (u8)pick[2];
            return;
        }
    }

    // Nothing usable there: the A button's own green, so the glyph is still painted.
    out->r = A_FALLBACK_R;
    out->g = A_FALLBACK_G;
    out->b = A_FALLBACK_B;
}

static prompt_label *label_at(prompt_group *g, int i) {
    return (prompt_label*)((u8*)g + g->labels_offset + i * sizeof(prompt_label));
}

static char *label_string(prompt_label *l) {
    return (char*)((u8*)l + l->str_offset);
}

// Replace a label's text in place. Only ever called with a shorter string, so the strings
// packed after it -- each reached by its own offset -- stay where they are.
static void label_retext(prompt_label *l, const char *text) {
    u16 len = (u16)strlen(text);
    if (len > l->length) return;
    memcpy(label_string(l), text, len + 1);
    l->length = len;
}

static void elem_move(prompt_group *g, u32 tag, s16 dx) {
    blob_element *e = blob_find_element(g, tag);
    if (e != NULL) e->base_x = (u16)(e->base_x + dx);
}

static bool group_matches(prompt_group *g) {
    if (g->type != make_type('G','L','H','0')) return false;
    if (g->elems_offset == 0 || g->labels_offset <= g->elems_offset) return false;
    if (g->pool_offset <= g->labels_offset || g->pool_offset > 0x4000) return false;
    if (*(u32*)((u8*)g + g->elems_offset)  != make_type('b','a','c','1')) return false;
    if (*(u32*)((u8*)g + g->labels_offset) != make_type('t','x','t','1')) return false;
    return g->elem_count > 0 && g->label_count > 0;
}

static bool group_fix(prompt_group *g) {
    bool english = false;

    // The dots go first: they are the space the labels close up over.
    static const char digits[] = "123456";
    for (int i = 0; digits[i] != '\0'; i++) {
        blob_element *e = blob_find_element(g, make_type('d','o','t',digits[i]));
        if (e != NULL) e->width = e->height = 0;
    }

    // Nothing else here moves any glyph. The bar carries two A/B pairs -- one the game list
    // draws, one the BIOS screens draw -- and which label record goes with which pair is
    // decided in per-screen code, not in this data. Repositioning on a guess is what put a
    // stray pill in the corner of the Options and Calendar screens and swapped Confirm with
    // Cancel: the pills are drawn every frame regardless of the screen, so a moved one shows
    // up everywhere. The game list's own pair is moved in prompt_bar_layout_list(), by tag,
    // and only that pair.
    for (int i = 0; i < g->label_count; i++) {
        prompt_label *l = label_at(g, i);
        char *s = label_string(l);

        if (strcmp(s, "Menu Selection") == 0) {
            label_retext(l, "Selection");
            english = true;
        } else if (strcmp(s, "Cancel") == 0) {
            label_retext(l, "Back");
            english = true;
        } else if (strcmp(s, "Confirm") == 0) {
            english = true;
        }

        // Every slot had dots, so every label closes up by their width and lifts to sit
        // level with its glyph.
        l->y += LABEL_DROP;
        l->x = (u16)(l->x - LABEL_CLOSE);
    }

    return english;
}

// The strings the screens actually draw do not come from a group's own pool -- that is only
// a default. They live in a separate text table (the one holding Cancel/Select/Finish/
// Change), which is why renaming the pool moved the labels but left them reading Cancel.
// Rather than chase that table, every standalone "Cancel" in the image is rewritten. The
// terminator is required so "Cancelar" on a PAL image is not turned into "Backlar", and the
// bytes freed are zeroed so a renderer working from a length draws nothing extra.
static int retext_image(const char *from, const char *to) {
    int len = (int)strlen(from);
    int hits = 0;

    for (u32 p = IPL_LO; p < IPL_HI - 32; p++) {
        char *at = (char*)p;
        if (at[0] != from[0] || at[len] != '\0') continue;
        if (memcmp(at, from, len) != 0) continue;

        memset(at, 0, len);
        memcpy(at, to, strlen(to));
        hits++;
    }

    return hits;
}

void prompt_bar_init(void) {
    prompt_group *first = NULL;
    prompt_group *english = NULL;
    int found = 0;

    for (u32 p = IPL_LO; p < IPL_HI - 0x4000; p += 4) {
        prompt_group *g = (prompt_group*)p;
        if (!group_matches(g)) continue;

        found++;
        if (group_fix(g) && english == NULL) english = g;
        if (first == NULL) first = g;

        p += g->pool_offset; // nothing to find inside a group already handled
    }

    bar = english != NULL ? english : first;
    int renamed = retext_image("Cancel", "Back");
    (void)renamed; // only read by the report below, which compiles out
    OSReport("prompt bar: %d group(s) at %08x, %d label(s) renamed\n", found, (u32)bar, renamed);
}

// Where the bar's two prompt pairs live, mapped from hardware shots rather than guessed:
//
//   butb + dot2 + txt2   the pair Cubiboot's screens draw (game list, Press START, No Disc)
//   butc + dot5 + txt5   the B of the BIOS screens (Game Play, Options, Calendar)
//   butd + dot6 + txt6   their A
//
// A label sits 0x500 to the right of its glyph in every pair, which is the offset the two
// prompts drawn on the game list are placed at as well.

static void pair_move(u32 glyph, u32 pill, u32 dot, u32 label_tag, s16 dx) {
    elem_move(bar, glyph, dx);
    elem_move(bar, pill, dx);
    elem_move(bar, dot, dx);
    for (int i = 0; i < bar->label_count; i++) {
        prompt_label *l = label_at(bar, i);
        if (l->tag == label_tag) l->x = (u16)(l->x + dx);
    }
}

void prompt_bar_layout(void) {
    if (bar == NULL) return;

    blob_element *a = blob_find_element(bar, make_type('b','u','t','a'));
    blob_element *b = blob_find_element(bar, make_type('b','u','t','b'));
    blob_element *c = blob_find_element(bar, make_type('b','u','t','c'));
    blob_element *d = blob_find_element(bar, make_type('b','u','t','d'));
    if (a == NULL || b == NULL || c == NULL || d == NULL) return;

    // Set 2 -- butb/txt2 and buta/txt3 -- is the pair the BIOS sub-screens draw (the memory
    // card file list, Sound, the calendar adjust) and the pair Cubiboot's own screens draw.
    // Swapping the two puts A before B on all of them at once, and the row keeps the span
    // the IPL gave it, so nothing needs centring afterwards.
    s16 span2 = (s16)(a->base_x - b->base_x);
    pair_move(make_type('b','u','t','b'), make_type('b','a','c','2'),
              make_type('d','o','t','2'), make_type('t','x','t','2'), span2);
    pair_move(make_type('b','u','t','a'), make_type('b','a','c','3'),
              make_type('d','o','t','3'), make_type('t','x','t','3'), (s16)-span2);

    // Set 1 -- butc/txt5 and butd/txt6 -- is the pair the BIOS cube screens draw.
    s16 span1 = (s16)(d->base_x - c->base_x);
    pair_move(make_type('b','u','t','c'), make_type('b','a','c','5'),
              make_type('d','o','t','5'), make_type('t','x','t','5'), span1);
    pair_move(make_type('b','u','t','d'), make_type('b','a','c','6'),
              make_type('d','o','t','6'), make_type('t','x','t','6'), (s16)-span1);
}

// The two prompts the game list was missing. Both are the IPL's own elements, moved into
// place above, so they match the B the stock bar draws beside them.
void prompt_bar_draw_glyphs(u8 ui_alpha) {
    if (bar == NULL) return;

    u8 alpha = pulse_alpha(ui_alpha);
    GXColor pill_plain = {0xFF, 0xFF, 0xFF, alpha};
    GXColor pill_green = {0xFF, 0xFF, 0xFF, alpha};
    slot_color_green(&pill_green);
    GXColor letter = {0xFF, 0xFF, 0xFF, ui_alpha}; // the glyph reads white, like its label

    blob_element *stick = blob_find_element(bar, make_type('b','a','c','4'));
    blob_element *abtn  = blob_find_element(bar, make_type('b','u','t','a'));
    if (stick == NULL || abtn == NULL) return;

    // Drawn at an offset rather than by moving the elements: these are shared with the BIOS
    // screens, and moving them is what put a stray pill in the corner of Options and
    // Calendar. draw_named_tex adds the offset to the element's own base.
    s16 dx_stick = (s16)(STICK_X + pal_glyph_dx() - stick->base_x);
    s16 dx_a     = a_dx(); // buta is already where an NTSC row wants it

    // The stick's pill sits a notch below the letter glyphs in the stock bar, which reads
    // fine when it is the only prompt but not in a row with A and B.
    s16 dy_stick = (s16)(abtn->base_y - stick->base_y);

    setup_tex_draw(1, 0, 0);

    // The stick does not switch tilts, it cross-fades between two of them -- which is why
    // the stock one on the main menu breathes and a straight swap blinks. One step every
    // 128 ticks of the counter, the rate BS2 uses.
    // What BS2 actually walks: eight states at 128 ticks each, alternating between the
    // centre and one tilt. Going out, the centre fades away as the tilt arrives; coming
    // back, the tilt fades away as the centre returns. Holding the centre at full alpha and
    // swapping tilts under it is what made it blink.
    u32 c = *prompt_pulse;
    u32 state = (c >> 7) & 7;                 // which half-step of which direction
    u32 f = ((c & 0x7f) * 255) / 127;         // 0..255 through it
    u32 centre = (state & 1) ? f : 255 - f;   // odd states are the return
    u32 tilt   = 255 - centre;

    static const char tilts[] = "6789";
    GXColor centre_c = {0xFF, 0xFF, 0xFF, (u8)((centre * ui_alpha) / 255)};
    GXColor tilt_c   = {0xFF, 0xFF, 0xFF, (u8)((tilt   * ui_alpha) / 255)};

    draw_named_tex(make_type('b','a','c','4'), bar, &pill_plain, dx_stick, dy_stick);
    draw_named_tex(make_type('s','t','k','5'), bar, &centre_c, dx_stick, dy_stick);
    draw_named_tex(make_type('s','t','k',tilts[state >> 1]), bar, &tilt_c, dx_stick, dy_stick);

    // A: only the pill carries the colour and the pulse.
    draw_named_tex(make_type('b','a','c','3'), bar, &pill_green, dx_a, 0);
    draw_named_tex(make_type('b','u','t','a'), bar, &letter, dx_a, 0);
}

// Their labels. Drawn in Cubiboot's own text coordinates instead of through a label record:
// every record in the bar belongs to a BIOS screen, so borrowing one moves that screen's
// text too (which is how Selection ended up on top of Cancel). draw_text maps its x,y as
// (x+64)*20 and (y+64)*10 into the same space the records use, so the two are converted from
// the element positions above and land at the same offset from their glyph as txt2 does.
// draw_text anchors a string differently from the bar's own label records, so the same y
// comes out a few pixels high and the same x a different distance from the glyph. Both are
// corrected here, against the B -- the one prompt whose label the IPL itself places.
#define LABEL_TEXT_Y_FIX 13

// Hand trim on the two labels Cubiboot draws, in element units (about 0x0f per pixel across,
// 0x0e down): Selection sits five pixels further from the stick, Confirm five closer to the A.
#define SEL_X_NUDGE      0x50
#define CONFIRM_X_NUDGE (-0x41)
#define TO_TEXT_X(e) (u16)((e) / 20 - 64)
#define TO_TEXT_Y(e) (u16)((e) / 10 - 64)

void prompt_bar_draw_labels(u8 ui_alpha) {
    if (bar == NULL) return;

    blob_element *b     = blob_find_element(bar, make_type('b','u','t','b'));
    blob_element *abtn  = blob_find_element(bar, make_type('b','u','t','a'));
    blob_element *stick = blob_find_element(bar, make_type('b','a','c','4'));
    if (b == NULL || abtn == NULL || stick == NULL) return;

    // The B is the one prompt the IPL places itself, so its label is the measurement to
    // copy, taken from the glyph's edge. A letter glyph's width is used for the stick too:
    // its pill declares more than twice that, but the ink inside is about a letter wide, so
    // measuring the declared edge pushed Selection visibly further out than the other two.
    s32 gap = 0;
    u16 y = 0;
    for (int i = 0; i < bar->label_count; i++) {
        prompt_label *l = label_at(bar, i);
        if (l->tag != make_type('t','x','t','2')) continue;
        gap = (s32)l->x - ((s32)b->base_x + (s32)b->width / 2);
        y = (u16)(TO_TEXT_Y(l->y) + LABEL_TEXT_Y_FIX);
    }
    if (y == 0) return;

    GXColor white = {0xFF, 0xFF, 0xFF, ui_alpha};
    draw_text("Selection", 0x14,
              TO_TEXT_X(STICK_X + pal_glyph_dx() + abtn->width / 2 + gap + SEL_X_NUDGE + pal_label_dx()), y, &white);
    draw_text("Confirm", 0x14,
              TO_TEXT_X(abtn->base_x + a_dx() + abtn->width / 2 + gap + CONFIRM_X_NUDGE + pal_label_dx()), y, &white);
}

// The header Z pill breathes off the same counter now, so it is in step with the B instead
// of running at its own slower rate.
u8 prompt_pulse_for_pill(u8 ui_alpha) {
    return pulse_alpha(ui_alpha);
}
