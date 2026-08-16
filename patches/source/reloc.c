#include "reloc.h"
#include "attr.h"

// used as extern
__attribute_data__ BOOL is_running_dolphin;

#ifdef DEBUG
// This is actually BS2Report on IPL rev 1.2
__attribute_reloc__ void (*OSReport)(const char* text, ...);
#endif

__attribute_reloc__ bios_pad *pad_status;
__attribute_reloc__ u32 *prev_menu_id;
__attribute_reloc__ u32 *cur_menu_id;
__attribute_reloc__ u32 *main_menu_id;
__attribute_reloc__ GXRModeObj *rmode;
__attribute_reloc__ void *stock_bs2tick_resume;
__attribute_reloc__ void *stock_gameselect_draw_resume;
__attribute_reloc__ void *stock_gameselect_draw_skip;

// from main
__attribute_reloc__ state *cube_state;

// from menu
__attribute_reloc__ u32 (*OSDisableInterrupts)();
__attribute_reloc__ BOOL (*OSRestoreInterrupts)(BOOL);
__attribute_reloc__ void (*VIWaitForRetrace)();
__attribute_reloc__ void (*GXSetCurrentMtx)(u32);
