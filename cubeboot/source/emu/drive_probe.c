#include "drive_probe.h"

#include <string.h>

#ifdef IPL_CODE
#include "../os.h"
#include "../time.h"
#else
#include <ogc/cache.h>
#include <ogc/lwp_watchdog.h>
#endif

// rel_date values reported in the inquiry response.
//
// The GC Loader magic is the one cubeboot/source/sd.c has always used to tell an
// ODE apart from a real drive. The FlippyDrive values are the ones Swiss matches in
// deviceHandler_Flippy_test(), which is the implementation cubiboot has to agree
// with anyway -- Swiss is handed the device name and does its own detection after
// the handoff, so disagreeing about what is on the bus would be worse than not
// supporting the drive at all.
// Only the two we act on are defined. A FlippyDrive in its bootloader reports
// 0x20220420 and a stock optical drive one of 0x20010608 / 0x20010831 / 0x20020402 /
// 0x20020823; all of those take the same "answered, unusable" path, so naming them
// would suggest branches that do not exist. See the comment on that path below.
#define REL_DATE_GCODE     0x20196c64
#define REL_DATE_FLIPPY_FW 0x20220426 // firmware: file API is live

// How long the drive gets to come up before we decide there isn't one. Only paid
// when nothing we recognise answers -- any definite answer ends the probe on the
// first attempt.
#define DRIVE_PROBE_TRIES 40
#define DRIVE_PROBE_DELAY (10 * 1000) // us

vu32* const di_regs = (vu32*)0xCC006000;

static int cached_id = -1; // -1 = not probed yet

bool di_wait(void) {
	for (u32 i = 0; i < DI_XFER_TIMEOUT; i++) {
		if (!(di_regs[DI_CR] & DI_CR_TSTART))
			return true;
	}

	return false;
}

drive_id_t drive_probe(void) {
	if (cached_id >= 0)
		return (drive_id_t)cached_id;

	static u8 info[32] __attribute__((aligned(32)));
	drive_id_t id = DRIVE_ID_NONE;

	for (int i = 0; i < DRIVE_PROBE_TRIES; i++) {
		memset(info, 0, sizeof(info));

		di_regs[DI_SR] = (DI_SR_BRKINTMASK | DI_SR_TCINTMASK | DI_SR_DEINT | DI_SR_DEINTMASK);
		di_regs[DI_CVR] = 0; // clear cover int

		di_regs[DI_CMDBUF0] = DVD_OEM_INQUIRY;
		di_regs[DI_CMDBUF1] = 0;
		di_regs[DI_CMDBUF2] = 0;

		di_regs[DI_MAR] = (u32)info & 0x1FFFFFFF;
		di_regs[DI_LENGTH] = sizeof(info);

		DCInvalidateRange(info, sizeof(info));

		di_regs[DI_CR] = (DI_CR_DMA | DI_CR_TSTART); // start transfer

		if (!di_wait()) {
			// The transfer never completed, so nothing is driving the interface at
			// all -- no ODE, no optical drive. Every retry would buy another full
			// timeout, so stop here instead. This is the path a driveless console
			// takes, and the probe runs on every boot, so it has to be cheap.
			id = DRIVE_ID_NONE;
			break;
		}

		DCInvalidateRange(info, sizeof(info));

		if (!(di_regs[DI_SR] & DI_SR_DEINT)) {
			// rel_date is at offset 4 of the inquiry response (dvd_info_t:
			// rev_level, dev_code, then rel_date).
			u32 rel_date = *(u32*)&info[4];

			if (rel_date == REL_DATE_GCODE) {
				id = DRIVE_ID_GCODE;
				break;
			}

			if (rel_date == REL_DATE_FLIPPY_FW) {
				id = DRIVE_ID_FLIPPY;
				break;
			}

			// Something answered and it is nothing we can use. Retrying only costs
			// boot time, so stop.
			//
			// The FlippyDrive bootloader date (0x20220420) and the stock optical
			// drive dates land here deliberately, with no branch of their own,
			// because a FlippyDrive on the path users actually arrive by reports
			// firmware. Two things establish that rather than assumption:
			//
			//  - Swiss's flippy_init() requires rel_date == 0x20220426 from a plain
			//    inquiry before its file API is usable, and its flippy_bypass(false)
			//    sends nothing at all when the drive already reports 0x2022042x -- a
			//    drive in firmware or bootloader mode is by definition not bypassed.
			//  - cubeboot, which this fork descends from and which ran on Flippyboot,
			//    called the file API and read the drive's internal flash straight
			//    away at startup with no bypass exit anywhere ahead of it. Bypass is
			//    only ever entered, deliberately, to hand a real disc to a game --
			//    which is what bs2start() still does for DVD passthrough.
			//
			// So reaching the file API from either state would mean commanding a
			// drive we have only just met, to support a way of booting that does not
			// occur: the drive loads our DOL, which leaves it running firmware.
			if (rel_date != 0) {
				id = DRIVE_ID_UNKNOWN;
				break;
			}
		}

		// Answered with an error or with nothing useful: the drive may still be
		// spinning up, so this is the one case worth retrying.
		udelay(DRIVE_PROBE_DELAY);
	}

	cached_id = (int)id;
	return id;
}
