#include "gcode.h"

#include <string.h>

#ifdef IPL_CODE
#include "../os.h"
#include "../time.h"
#else
#include <ogc/cache.h>
#include <ogc/lwp_watchdog.h>
#endif

// DI regs from YAGCD (same layout flippy_sync.c uses)
#define DI_SR      0 // 0xCC006000 - DI Status Register
#define DI_SR_BRKINTMASK (1 << 5) // Break Complete Interrupt Mask
#define DI_SR_TCINTMASK  (1 << 3) // Transfer Complete Interrupt Mask
#define DI_SR_DEINT      (1 << 2) // Device Error Interrupt Status
#define DI_SR_DEINTMASK  (1 << 1) // Device Error Interrupt Mask

#define DI_CVR     1 // 0xCC006004 - DI Cover Register
#define DI_CMDBUF0 2 // 0xCC006008 - DI Command Buffer 0
#define DI_CMDBUF1 3 // 0xCC00600c - DI Command Buffer 1
#define DI_CMDBUF2 4 // 0xCC006010 - DI Command Buffer 2
#define DI_MAR     5 // 0xCC006014 - DMA Memory Address Register
#define DI_LENGTH  6 // 0xCC006018 - DI DMA Transfer Length Register
#define DI_CR      7 // 0xCC00601c - DI Control Register
#define DI_CR_DMA    (1 << 1) // 0: immediate mode, 1: DMA mode
#define DI_CR_TSTART (1 << 0) // transfer start / transfer pending

// DI Commands
#define DVD_OEM_INQUIRY 0x12000000
// GC Loader raw storage read: CMDBUF1 = LBA in 512 byte sectors, CMDBUF2 = byte
// count, DMA'd to MAR. Same command libogc2 issues from DVD_LowGcodeRead().
#define DVD_GCODE_READ  0xB2000000

// rel_date the GC Loader reports in its inquiry response. This is the same magic
// cubeboot/source/sd.c already uses to tell an ODE apart from a real drive.
#define GCODE_INQUIRY_REL_DATE 0x20196c64

#define GCODE_SECTOR_SIZE 512
#define GCODE_MAX_SECTORS 128 // 64KB per DI transfer

// The DI has no interrupt wired up here, so every transfer is a spin on TSTART.
// A drive that never answers would hang the loader outright without a bound.
#define DI_XFER_TIMEOUT 500000

// How long we give the ODE to come up before deciding there isn't one. Only
// paid when no GC Loader answers -- a drive that replies with some other
// rel_date is a real optical drive and ends the probe immediately.
#define GCODE_PROBE_TRIES  40
#define GCODE_PROBE_DELAY  (10 * 1000) // us

static vu32* const _di_regs = (vu32*)0xCC006000;

static int has_gcode = -1; // -1 = not probed yet

static bool di_wait(void) {
    for (u32 i = 0; i < DI_XFER_TIMEOUT; i++) {
        if (!(_di_regs[DI_CR] & DI_CR_TSTART))
            return true;
    }

    return false;
}

// dst must be 32 byte aligned and len a multiple of 32 (both hold for whole
// sectors out of an aligned buffer).
static bool gcode_read_aligned(u32 sector, void *dst, u32 len) {
    _di_regs[DI_SR] = (DI_SR_BRKINTMASK | DI_SR_TCINTMASK | DI_SR_DEINT | DI_SR_DEINTMASK);
    _di_regs[DI_CVR] = 0; // clear cover int

    _di_regs[DI_CMDBUF0] = DVD_GCODE_READ;
    _di_regs[DI_CMDBUF1] = sector;
    _di_regs[DI_CMDBUF2] = len;

    _di_regs[DI_MAR] = (u32)dst & 0x1FFFFFFF; // Cached -> Effective
    _di_regs[DI_LENGTH] = len;

    // Drop any dirty lines over the destination before the DMA lands on it.
    DCInvalidateRange(dst, len);

    _di_regs[DI_CR] = (DI_CR_DMA | DI_CR_TSTART); // start transfer

    if (!di_wait())
        return false;

    DCInvalidateRange(dst, len);

    // check if ERR was asserted
    return !(_di_regs[DI_SR] & DI_SR_DEINT);
}

bool gcode_sd_init(void) {
    if (has_gcode >= 0)
        return has_gcode == 1;

    static u8 info[32] __attribute__((aligned(32)));

    for (int i = 0; i < GCODE_PROBE_TRIES; i++) {
        memset(info, 0, sizeof(info));

        _di_regs[DI_SR] = (DI_SR_BRKINTMASK | DI_SR_TCINTMASK | DI_SR_DEINT | DI_SR_DEINTMASK);
        _di_regs[DI_CVR] = 0; // clear cover int

        _di_regs[DI_CMDBUF0] = DVD_OEM_INQUIRY;
        _di_regs[DI_CMDBUF1] = 0;
        _di_regs[DI_CMDBUF2] = 0;

        _di_regs[DI_MAR] = (u32)info & 0x1FFFFFFF;
        _di_regs[DI_LENGTH] = sizeof(info);

        DCInvalidateRange(info, sizeof(info));

        _di_regs[DI_CR] = (DI_CR_DMA | DI_CR_TSTART); // start transfer

        if (!di_wait()) {
            // The transfer never completed, so nothing is driving the interface at all --
            // no ODE, no optical drive. Every retry would buy another full timeout, so
            // stop here instead. This is the path a driveless console takes, and the
            // probe now runs on every boot, so it has to be cheap.
            break;
        }

        DCInvalidateRange(info, sizeof(info));

        if (!(_di_regs[DI_SR] & DI_SR_DEINT)) {
            u32 rel_date = *(u32*)&info[4];
            if (rel_date == GCODE_INQUIRY_REL_DATE) {
                has_gcode = 1;
                return true;
            }

            // Something answered and it isn't a GC Loader -- a real optical drive, or an
            // ODE without the raw storage command. Retrying only costs boot time.
            if (rel_date != 0)
                break;
        }

        // Answered with an error or with nothing useful: the drive may still be spinning
        // up, so this is the one case worth retrying.
        udelay(GCODE_PROBE_DELAY);
    }

    has_gcode = 0;
    return false;
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
