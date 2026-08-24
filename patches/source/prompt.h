#pragma once

#include <gctypes.h>
#include <stdbool.h>

// One entry of a blob's element table, as the IPL's draw dispatchers walk it (16 bytes;
// field offsets read straight out of the BS2 disassembly of fn 0x81309f3c).
typedef struct {
    u32 tag;
    u16 base_x;
    u16 base_y;
    u16 width;
    u16 height;
    u16 tex_index;
    u8  unk14;
    u8  flags;
} blob_element;

blob_element *blob_find_element(void *blob, u32 tag);

// The stock bottom prompt bar ("(B) ... Cancel"), reused and corrected. See prompt.c.
void prompt_bar_init(void);
void prompt_bar_layout(void);
void prompt_bar_draw_glyphs(u8 alpha);
void prompt_bar_draw_labels(u8 alpha);
u8 prompt_pulse_for_pill(u8 ui_alpha);
