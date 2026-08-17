#include "dolphin_dvd.h"

#define assert(...)
// #include <assert.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gcutil.h>
#include <ogc/system.h>
#include "picolibc.h" // for strcasecmp

#include "reloc.h"
#include "flippy_sync.h"
#include "dvd_threaded.h"
#include "bnr_offsets.h"
#include "bnr.h"
#include "emu/tweaks.h" // bnr_cache_put: warm the banner cache from the validation read

#include "dolphin_os.h"
#include "time.h" // udelay: settle time after the bypass reset
#include "gc_dvd.h" // dvd_read_id / dvd_get_error: blocking wait for the drive

#define DVD_FS_DUMP 1

#define GET_OFFSET(o) ((u32)((o[0] << 16) | (o[1] << 8) | o[2]))

#define ENTRY_IS_DIR(i) (entry_table[i].filetype == T_DIR)
#define FILE_POSITION(i) (entry_table[i].addr)
#define FILE_LENGTH(i) (entry_table[i].len)

typedef struct {
    u32 offset;
    u32 length;
} bnr_info_t;

static bnr_info_t get_banner_offset_slow(DiskHeader *header, uint32_t fd) {
    u32 size = OSRoundUp32B(header->FSTSize);
    u32 offset = header->FSTOffset;
    u8 *fst = (void*)0x81700000;

    // read FST
    dvd_threaded_read(fst, size, offset, fd);

    FSTEntry *entry_table = (FSTEntry*)fst;
    u32 total_entries = entry_table[0].len;
    char *string_table = (char*)&(entry_table[total_entries]);
    if ((u32)string_table < 0x81700000 || (u32)string_table > 0x81800000) {
        OSReport("ERROR: String table is out of bounds: %08x\n", (u32)string_table);
        return (bnr_info_t) {
            .offset = 0,
            .length = 0,
        };
    }

#ifdef PRINT_READDIR_FILES
    OSReport("FST contains %u\n", total_entries);
    OSYieldThread();
#endif

    FSTEntry* p = entry_table;
    for (u32 i = 1; i < total_entries; ++i) { //Start @ 1 to skip FST header
        u32 string_offset = GET_OFFSET(p[i].offset);
        char *string = (char*)((u32)string_table + string_offset);
        // OSReport("String table = %08x, String offset = %08x\n", (u32)string_table, string_offset);
        // OSReport("FST (0x%08x) entry: %s\n", FILE_POSITION(i), string);
        if (entry_table[i].filetype == T_FILE && strcasecmp(string, "opening.bnr") == 0) {
#ifdef PRINT_READDIR_FILES
            OSReport("FST (0x%08x) entry: %s\n", FILE_POSITION(i), string);
#endif

            OSYieldThread(); // allow rescheduling
            return (bnr_info_t) {
                .offset = FILE_POSITION(i),
                .length = FILE_LENGTH(i),
            };
        }
    }

    OSYieldThread(); // allow rescheduling
    return (bnr_info_t) {
        .offset = 0,
        .length = 0,
    };
}

// Get the BNR offset on the disc
dolphin_game_into_t get_game_info(char *game_path) {
    __attribute__((aligned(32))) static u32 small_buf[8]; // for BNR reads

    const uint8_t flags = IPC_FILE_FLAG_DISABLECACHE | IPC_FILE_FLAG_DISABLESPEEDEMU;
    int ret = dvd_custom_open(game_path, FILE_ENTRY_TYPE_FILE, flags);
    if (ret != 0) {
        OSReport("ERROR: Failed to open %s\n", game_path);
        return (dolphin_game_into_t) { .valid = false };
    }

    // OSReport("DEBUG: file opened %s\n", game_path);

    file_status_t *status = dvd_custom_status();
    if (status->result != 0) {
        OSReport("ERROR: Failed to get status for %s\n", game_path);

        dvd_custom_close(status->fd);
        return (dolphin_game_into_t) { .valid = false };
    }

    // OSReport("DEBUG: status loaded %d\n", status->fd);

    __attribute__((aligned(32))) static DiskHeader header;
    dvd_threaded_read(&header, sizeof(DiskHeader), 0, status->fd); //Read in the disc header

    // OSReport("DEBUG: disk header loaded\n");

    u32 fast_bnr_offset = get_banner_offset_fast(&header);
    // OSReport("DEBUG: Fast BNR offset: %08x\n", fast_bnr_offset);
    if (fast_bnr_offset != 0) {
        // Validate with just the 32-byte magic here; gm_load_banner reads the full banner
        // when it actually needs it. (The fast-path that read the whole 8KB banner into a
        // static buffer grew the loader by ~8KB and broke launching swiss-gc.dol from a
        // folder -- the boot would hard-reset to the console IPL.)
        dvd_threaded_read(small_buf, 32, fast_bnr_offset, status->fd); //Read in the banner data

        u32 magic = small_buf[0];
        if (magic == BANNER_MAGIC_1 || magic == BANNER_MAGIC_2) {
            dolphin_game_into_t info;
            info.valid = true;
            info.bnr_type = magic == BANNER_MAGIC_2; // BANNER_MULTI_LANG
            info.bnr_offset = fast_bnr_offset;
            memcpy(info.game_id, &header, 6);
            info.disc_num = header.DiscID;
            info.disc_ver = header.Version;
            info.dol_offset = header.DOLOffset;
            info.fst_offset = header.FSTOffset;
            info.fst_size = header.FSTSize;
            info.max_fst_size = header.MaxFSTSize;

            dvd_custom_close(status->fd);
            return info;
        }
    }

    // OSReport("DEBUG: loading FST from disk\n");

    if (header.FSTSize > 0x100000) {
        OSReport("ERROR: FST size is too large: %08x\n", header.FSTSize);
        dvd_custom_close(status->fd);
        return (dolphin_game_into_t) { .valid = false };
    }

    // If we didn't find the banner in the fast location, try the FST
    bnr_info_t bnr_info = get_banner_offset_slow(&header, status->fd);
    if (bnr_info.offset != 0) {
        dvd_threaded_read(small_buf, 32, bnr_info.offset, status->fd); //Read in the banner data

        u32 magic = small_buf[0];
        if (magic == BANNER_MAGIC_1 || magic == BANNER_MAGIC_2) {
            dolphin_game_into_t info;
            info.valid = true;
            info.bnr_type = magic == BANNER_MAGIC_2; // BANNER_MULTI_LANG
            info.bnr_offset = bnr_info.offset;
            memcpy(info.game_id, &header, 6);
            info.disc_num = header.DiscID;
            info.disc_ver = header.Version;
            info.dol_offset = header.DOLOffset;
            info.fst_offset = header.FSTOffset;
            info.fst_size = header.FSTSize;
            info.max_fst_size = header.MaxFSTSize;

            dvd_custom_close(status->fd);
            return info;
        }
    }

    // OSReport("DEBUG: FST was loaded\n");

    // invalid file
    dvd_custom_close(status->fd);
    return (dolphin_game_into_t) { .valid = false };
}


// --- physical-disc banner read, stepped one frame at a time --------------------------
//
// The stock Game Play flow only reaches a banner after its apploader stage has loaded the
// game into RAM, and that load lands on the memory Cubiboot occupies -- its lowmem data at
// 0x80100000 and the IPL heap moved down to make room for it -- so the console dies partway
// through "Reading disc...". opening.bnr is just a file in the disc's filesystem, so it can
// be read without any of that, and the region check goes with it: that check lives in the
// machine being skipped.
//
// The read is driven a step per frame rather than in one blocking call, for the same reason
// the stock machine is asynchronous: a drive that has been idle takes seconds to spin up,
// and blocking through that freezes the menu instead of animating "Reading disc..." over it.
// Waiting on the transfer bit costs nothing here -- the frame loop keeps running.

static volatile u32 *const di = (volatile u32*)0xCC006000;
#define DI_I_SR   0
#define DI_I_CVR  1
#define DI_I_CMD0 2
#define DI_I_CMD1 3
#define DI_I_CMD2 4
#define DI_I_MAR  5
#define DI_I_LEN  6
#define DI_I_CR   7

u32 diag_banner_stage = 0; // 1 spin-up, 2 header, 3 locate, 4 banner, 5 done, 6+ failed

static int  dbr_step;
static int  dbr_errors;
static int  dbr_frames;
static u32  dbr_offset;
static BNR *dbr_out;

__attribute__((aligned(32))) static DiskHeader dbr_header;
__attribute__((aligned(32))) static u32 dbr_scratch[8];

static void di_start_read(void *dst, u32 len, u32 offset) {
    di[DI_I_SR]   = 0x2E;
    di[DI_I_CVR]  = 0;
    di[DI_I_CMD0] = 0xA8000040;
    di[DI_I_CMD1] = offset >> 2;
    di[DI_I_CMD2] = len;
    di[DI_I_MAR]  = (u32)dst & 0x1FFFFFFF;
    di[DI_I_LEN]  = len;
    di[DI_I_CR]   = 3; // DMA + start
}

static inline bool di_busy(void)  { return (di[DI_I_CR] & 1) != 0; }
static inline bool di_failed(void) { return (di[DI_I_SR] & 0x4) != 0; }

static int dbr_fail(u32 stage) {
    diag_banner_stage = stage;
    dvd_custom_bypass_exit();
    dbr_step = -1;
    return -1;
}

void disc_banner_start(BNR *out) {
    dbr_out = out;
    dbr_step = 0;
    dbr_errors = 0;
    dbr_frames = 0;
    dbr_offset = 0;
    diag_banner_stage = 1;

    // Takes the drive off the file protocol and resets it, so the spin-up wait below has to
    // come after this, not before.
    dvd_custom_bypass_enter();
}

// 0 = still working, 1 = banner read, -1 = no readable disc.
int disc_banner_poll(void) {
    if (++dbr_frames > 900) return dbr_fail(6); // ~15s, well past any spin-up

    switch (dbr_step) {
    case 0: // ask the drive for the disc ID; it answers once the disc is up to speed
        di_start_read(dbr_scratch, 32, 0);
        dbr_step = 1;
        return 0;

    case 1:
        if (di_busy()) return 0;
        if (di_failed()) {
            if (++dbr_errors > 40) return dbr_fail(6);
            dbr_step = 0; // not ready yet -- ask again next frame
            return 0;
        }
        diag_banner_stage = 2;
        di_start_read(&dbr_header, sizeof(DiskHeader), 0);
        dbr_step = 2;
        return 0;

    case 2:
        if (di_busy()) return 0;
        if (di_failed()) return dbr_fail(7);
        DCInvalidateRange(&dbr_header, sizeof(DiskHeader));
        if (dbr_header.DVDMagicWord != 0xC2339F3D) return dbr_fail(8);

        diag_banner_stage = 3;
        dbr_offset = get_banner_offset_fast(&dbr_header);
        if (dbr_offset == 0) {
            // Rare layout: walk the FST instead. Blocking, but the drive is spinning by now
            // so it costs a few milliseconds rather than a spin-up.
            if (dbr_header.FSTSize <= 0x100000)
                dbr_offset = get_banner_offset_slow(&dbr_header, 0).offset;
            if (dbr_offset == 0) return dbr_fail(9);
        }
        di_start_read(dbr_scratch, 32, dbr_offset);
        dbr_step = 3;
        return 0;

    case 3:
        if (di_busy()) return 0;
        if (di_failed()) return dbr_fail(10);
        DCInvalidateRange(dbr_scratch, 32);
        if (dbr_scratch[0] != BANNER_MAGIC_1 && dbr_scratch[0] != BANNER_MAGIC_2)
            return dbr_fail(11);

        diag_banner_stage = 4;
        di_start_read(dbr_out, sizeof(BNR), dbr_offset);
        dbr_step = 4;
        return 0;

    case 4:
        if (di_busy()) return 0;
        if (di_failed()) return dbr_fail(12);
        DCInvalidateRange(dbr_out, sizeof(BNR));

        diag_banner_stage = 5;
        dvd_custom_bypass_exit();
        dbr_step = 5;
        return 1;
    }

    return -1;
}
