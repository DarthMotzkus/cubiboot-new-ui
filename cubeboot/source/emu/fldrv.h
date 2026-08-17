#ifndef FLDRV_H
#define FLDRV_H

#include "../flippy_sync.h" // ipc.h types: file_entry_t, file_status_t

#ifdef __cplusplus
extern "C" {
#endif

// Native backend for a FlippyDrive: the drive serves paths itself over the drive
// interface, so there is no volume, no sectors and no FatFs in the path. Every call
// here is one DI transfer.
//
// This is the device the FlippyDrive bootloader autoloads cubiboot from -- our DOL
// lives in the drive's internal flash under the name the bootloader expects, while
// config.ini and Swiss sit on the drive's SD card, which is what these paths reach.
//
// flippy_emu.c decides between this and the FatFs emulation per call; nothing else
// should call into here directly.

// Whether a FlippyDrive running firmware is on the bus. Free after the first call.
bool fldrv_present(void);

// Releases handles inherited from the bootloader, then reports readiness.
//
// This is not optional housekeeping. The bootloader loads our DOL out of the drive's
// flash and leaves that file handle OPEN -- which is why updating cubiboot on a
// FlippyDrive means holding X to reach the bootloader menu, so it hands over without
// claiming the handle and the flash copy can be overwritten. Arriving normally, we
// inherit that open handle, so the first open of our own would collide with it.
// Swiss guards the same way: flippy_init() ends in flippy_closefrom(1).
bool fldrv_init(void);

int  fldrv_open(const char* path, uint8_t type, uint8_t flags);
int  fldrv_open_flash(const char* path, uint8_t type, uint8_t flags);
void fldrv_close(uint32_t fd);

// Fills in the result, size and -- after an open -- the fd the drive assigned. Unlike
// the FatFs emulation, which hands out a fixed fd of 1, these are real handles.
int  fldrv_status(file_status_t* dst);

int  fldrv_readdir(file_entry_t* dst, uint32_t fd);

// Points the drive at a file (and a second disc, if any) so an image boots through
// the drive itself instead of being chainloaded.
void fldrv_set_default_fd(uint32_t current_fd, uint32_t second_fd);

// The drive goes transparent so the optical drive behind it answers the bus -- a
// FlippyDrive sits in-line on the drive ribbon, so a real drive can still be there.
// The file API is unreachable while bypassed; exit brings it back, handles intact.
void fldrv_bypass_enter(void);
void fldrv_bypass_exit(void);

#ifdef __cplusplus
}
#endif

#endif // FLDRV_H
