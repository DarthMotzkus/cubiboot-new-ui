#include "fldrv.h"

#include <string.h>

#include "drive_probe.h"

#ifdef IPL_CODE
#include "../os.h"
#else
#include <stdio.h>
#include <ogc/cache.h>
#endif

// Base for the drive's file API. The command byte goes in the low bits and, for the
// calls that address an open file, the handle goes in bits 16-23.
#define DVD_FLIPPY_FILEAPI_BASE 0xB5000000

// Bypass toggle. One command both ways: entry is the bare command, exit carries two
// magic words in the argument buffers -- while bypassed the drive is only snooping the
// bus, so the way back has to be something ordinary drive traffic can never spell.
#define DVD_FLIPPY_BYPASS 0xDC000000

// Handle numbers, matching what the drive hands out. 1..31 are ordinary files; the
// flash is a fixed handle of its own.
#define FLDRV_MAX_HANDLES  31
#define FLDRV_FLASH_HANDLE 64

// One DI transfer. Every call in this file is this sequence with a different command
// byte, which is worth having in one place: written out per call it is eight lines of
// register poking each, and the differences between them stop being visible.
//
// buf may be NULL for commands that carry no payload. to_drive sends rather than
// receives -- open is the only caller that does, handing over a file_entry_t.
static int fldrv_xfer(u32 cmd, u32 arg1, u32 arg2, void* buf, u32 len, bool to_drive) {
	di_regs[DI_SR] = (DI_SR_BRKINTMASK | DI_SR_TCINTMASK | DI_SR_DEINT | DI_SR_DEINTMASK);
	di_regs[DI_CVR] = 0; // clear cover int

	di_regs[DI_CMDBUF0] = cmd;
	di_regs[DI_CMDBUF1] = arg1;
	di_regs[DI_CMDBUF2] = arg2;

	u32 cr = DI_CR_TSTART;

	if (buf != NULL) {
		// Push our side out before the drive reads it, or drop stale lines before it
		// writes over them.
		if (to_drive)
			DCFlushRange(buf, len);
		else
			DCInvalidateRange(buf, len);

		di_regs[DI_MAR] = (u32)buf & 0x1FFFFFFF; // cached -> effective
		di_regs[DI_LENGTH] = len;

		cr |= DI_CR_DMA;
		if (to_drive)
			cr |= DI_CR_RW;
	} else {
		di_regs[DI_MAR] = 0;
		di_regs[DI_LENGTH] = 0;
	}

	di_regs[DI_CR] = cr; // start transfer

	// Bounded, unlike the unbounded TSTART spins this protocol is usually written
	// with: this runs as the IPL, so a drive that stops answering mid-transfer would
	// hang the console with no menu to go back to.
	if (!di_wait())
		return -1;

	if (buf != NULL && !to_drive)
		DCInvalidateRange(buf, len);

	// check if ERR was asserted
	return (di_regs[DI_SR] & DI_SR_DEINT) ? 1 : 0;
}

bool fldrv_present(void) {
	return drive_probe() == DRIVE_ID_FLIPPY;
}

void fldrv_close(uint32_t fd) {
	fldrv_xfer(DVD_FLIPPY_FILEAPI_BASE | IPC_FILE_CLOSE | ((fd & 0xFF) << 16),
	           0, 0, NULL, 0, false);
}

bool fldrv_init(void) {
	static bool released = false;

	if (!fldrv_present())
		return false;

	if (released)
		return true;

	// Closing a handle that was never open is harmless -- the drive reports an error
	// we have no use for -- and that is cheaper than asking which ones are in use.
	for (uint32_t fd = 1; fd <= FLDRV_MAX_HANDLES; fd++)
		fldrv_close(fd);
	fldrv_close(FLDRV_FLASH_HANDLE);

	released = true;
	return true;
}

// The drive reads the request out of memory, so it has to be aligned and cannot live
// on the stack of a caller that might go away mid-transfer.
static GCN_ALIGNED(file_entry_t) request;

static int fldrv_open_common(u32 command, const char* path, uint8_t type, uint8_t flags) {
	memset(&request, 0, sizeof(request));

	strncpy(request.name, path, sizeof(request.name) - 1);
	request.name[sizeof(request.name) - 1] = '\0';
	request.type = type;
	request.flags = flags;

	return fldrv_xfer(DVD_FLIPPY_FILEAPI_BASE | command, 0, 0,
	                  &request, sizeof(request), true);
}

int fldrv_open(const char* path, uint8_t type, uint8_t flags) {
	return fldrv_open_common(IPC_FILE_OPEN, path, type, flags);
}

int fldrv_open_flash(const char* path, uint8_t type, uint8_t flags) {
	// A separate command rather than a path prefix. The FatFs emulation fakes the
	// drive's flash by looking under /cubiboot on the card, which has nothing to do
	// with the real thing: this reaches storage inside the drive that no card holds.
	return fldrv_open_common(IPC_FILE_OPEN_FLASH, path, type, flags);
}

int fldrv_status(file_status_t* dst) {
	return fldrv_xfer(DVD_FLIPPY_FILEAPI_BASE | IPC_READ_STATUS, 0, 0,
	                  dst, sizeof(file_status_t), false);
}

int fldrv_readdir(file_entry_t* dst, uint32_t fd) {
	return fldrv_xfer(DVD_FLIPPY_FILEAPI_BASE | IPC_FILE_READDIR | ((fd & 0xFF) << 16),
	                  0, 0, dst, sizeof(file_entry_t), false);
}

void fldrv_set_default_fd(uint32_t current_fd, uint32_t second_fd) {
	fldrv_xfer(DVD_FLIPPY_FILEAPI_BASE | IPC_SET_DEFAULT_FD
	               | ((current_fd & 0xFF) << 16) | ((second_fd & 0xFF) << 8),
	           0, 0, NULL, 0, false);
}

// The drive goes transparent and the optical drive behind it answers the bus, which is
// what lets the disc screen and a passthrough boot read a real disc on a console that
// still has its drive. The file API is unreachable until the exit magic is sent; open
// handles survive in the drive across the round trip. Swiss brackets its own disc
// passthrough with the same pair (flippy_bypass()).
void fldrv_bypass_enter(void) {
	fldrv_xfer(DVD_FLIPPY_BYPASS, 0, 0, NULL, 0, false);
}

void fldrv_bypass_exit(void) {
	fldrv_xfer(DVD_FLIPPY_BYPASS, FD_BYPASS_EXIT_MAGIC0, FD_BYPASS_EXIT_MAGIC1,
	           NULL, 0, false);
}
