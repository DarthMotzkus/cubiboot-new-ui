#ifndef DRIVE_PROBE_H
#define DRIVE_PROBE_H

#include <stdint.h>
#include <stdbool.h>
#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Who is answering on the drive interface, and the DI register access every
// backend that talks to it shares.
//
// There is one drive connector, so at most one of these devices can be present on
// a given console -- a GC Loader and a FlippyDrive cannot both be installed. That
// is what lets a single inquiry identify the device for every caller, instead of
// each backend running its own probe with its own retry loop. On a console with no
// drive at all, the cost stays exactly one bounded transfer.

// ---------------------------------------------------------------------------
// DI registers, indexed as u32 words from 0xCC006000 (YAGCD).
// ---------------------------------------------------------------------------
#define DI_SR      0 // DI Status Register
#define DI_SR_BRKINTMASK (1 << 5) // Break Complete Interrupt Mask
#define DI_SR_TCINTMASK  (1 << 3) // Transfer Complete Interrupt Mask
#define DI_SR_DEINT      (1 << 2) // Device Error Interrupt Status
#define DI_SR_DEINTMASK  (1 << 1) // Device Error Interrupt Mask

#define DI_CVR     1 // DI Cover Register
#define DI_CMDBUF0 2 // DI Command Buffer 0
#define DI_CMDBUF1 3 // DI Command Buffer 1
#define DI_CMDBUF2 4 // DI Command Buffer 2
#define DI_MAR     5 // DMA Memory Address Register
#define DI_LENGTH  6 // DI DMA Transfer Length Register
#define DI_CR      7 // DI Control Register
#define DI_CR_RW     (1 << 2) // access mode, 0: read, 1: write
#define DI_CR_DMA    (1 << 1) // 0: immediate mode, 1: DMA mode
#define DI_CR_TSTART (1 << 0) // transfer start / transfer pending

#define DVD_OEM_INQUIRY 0x12000000

extern vu32* const di_regs;

// The DI has no interrupt wired up here, so every transfer is a spin on TSTART.
// A drive that never answers would hang the loader outright without a bound.
#define DI_XFER_TIMEOUT 500000

// Spins until the current transfer clears TSTART. Returns false if it never did.
bool di_wait(void);

// ---------------------------------------------------------------------------
// Drive identification
// ---------------------------------------------------------------------------
typedef enum {
	DRIVE_ID_NONE = 0, // nothing drove the bus: no ODE, no optical drive
	DRIVE_ID_UNKNOWN,  // something answered, but not a device we can use
	DRIVE_ID_GCODE,    // GC Loader style ODE -- raw storage commands (gcode.c)
	DRIVE_ID_FLIPPY,   // FlippyDrive running firmware -- file API is live
} drive_id_t;

// Identifies the drive from a single OEM inquiry and caches the answer, so every
// call after the first is free. Retries only while the drive is still waking up.
drive_id_t drive_probe(void);

#ifdef __cplusplus
}
#endif

#endif // DRIVE_PROBE_H
