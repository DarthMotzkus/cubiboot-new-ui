#include "gcode.h"

#include <string.h>

#ifdef IPL_CODE
#include "../os.h"
#include "../time.h"
#else
#include <ogc/cache.h>
#include <ogc/lwp_watchdog.h>
#endif

// DI registers, the bounded wait and the OEM inquiry all live in drive_probe.h --
// this driver, the FlippyDrive file API and the identification itself all drive the
// same interface, so there is one copy of that plumbing rather than one per backend.
#include "drive_probe.h"

// GC Loader raw storage read: CMDBUF1 = LBA in 512 byte sectors, CMDBUF2 = byte
// count, DMA'd to MAR. Same command libogc2 issues from DVD_LowGcodeRead().
#define DVD_GCODE_READ  0xB2000000

#define GCODE_SECTOR_SIZE 512
#define GCODE_MAX_SECTORS 128 // 64KB per DI transfer

// dst must be 32 byte aligned and len a multiple of 32 (both hold for whole
// sectors out of an aligned buffer).
static bool gcode_read_aligned(u32 sector, void *dst, u32 len) {
    di_regs[DI_SR] = (DI_SR_BRKINTMASK | DI_SR_TCINTMASK | DI_SR_DEINT | DI_SR_DEINTMASK);
    di_regs[DI_CVR] = 0; // clear cover int

    di_regs[DI_CMDBUF0] = DVD_GCODE_READ;
    di_regs[DI_CMDBUF1] = sector;
    di_regs[DI_CMDBUF2] = len;

    di_regs[DI_MAR] = (u32)dst & 0x1FFFFFFF; // Cached -> Effective
    di_regs[DI_LENGTH] = len;

    // Drop any dirty lines over the destination before the DMA lands on it.
    DCInvalidateRange(dst, len);

    di_regs[DI_CR] = (DI_CR_DMA | DI_CR_TSTART); // start transfer

    if (!di_wait())
        return false;

    DCInvalidateRange(dst, len);

    // check if ERR was asserted
    return !(di_regs[DI_SR] & DI_SR_DEINT);
}

bool gcode_sd_init(void) {
    // The inquiry, the retry-while-waking logic and the caching are all in
    // drive_probe(): a GC Loader and a FlippyDrive occupy the same connector, so one
    // inquiry settles which of them is there and both drivers read that one answer.
    return drive_probe() == DRIVE_ID_GCODE;
}

bool gcode_sd_read(uint32_t sector, uint8_t *data, uint32_t count) {
    if (!gcode_sd_init())
        return false;

    if ((u32)data & 31) {
        // FatFs hands us its own window buffer for single sector reads and that
        // one isn't DMA aligned, so stage those through an aligned sector.
        static u8 bounce[GCODE_SECTOR_SIZE] __attribute__((aligned(32)));

        for (uint32_t i = 0; i < count; i++) {
            if (!gcode_read_aligned(sector + i, bounce, GCODE_SECTOR_SIZE))
                return false;

            memcpy(data + (i * GCODE_SECTOR_SIZE), bounce, GCODE_SECTOR_SIZE);
        }

        return true;
    }

    while (count > 0) {
        uint32_t chunk = count > GCODE_MAX_SECTORS ? GCODE_MAX_SECTORS : count;

        if (!gcode_read_aligned(sector, data, chunk * GCODE_SECTOR_SIZE))
            return false;

        sector += chunk;
        data += chunk * GCODE_SECTOR_SIZE;
        count -= chunk;
    }

    return true;
}

bool gcode_sd_write(uint32_t sector, const uint8_t *data, uint32_t count) {
    // Read only on purpose: the loader and the menu never write to the game
    // volume, and a half tested write path on someone's game library is not a
    // trade worth making.
    (void)sector;
    (void)data;
    (void)count;
    return false;
}
