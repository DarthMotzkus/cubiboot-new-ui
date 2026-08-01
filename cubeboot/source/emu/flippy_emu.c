#include "../flippy_sync.h"

#include <string.h>
#include "ffs/ff.h"
#include "tweaks.h"

#ifdef IPL_CODE
#include "../dvd_threaded.h"
#include "../reloc.h"
#include "../attr.h"
#include "../gc_dvd.h"
#include "config.h"
#else
#include <stdio.h>
#include <di/di.h>
#include "../config.h"
#endif


static FATFS fs;

#ifdef IPL_CODE
__attribute_data__ int emu_sd_device;
#else
int emu_sd_device = -1;
#endif
// Index order is shared with the IPL patches: cubeboot picks the device and
// hands the index over as `emu_sd_device`, the patched BIOS only ever mounts
// device_prio[emu_sd_device]. Keep both trees in sync -- this file is copied
// into patches/source/emu at build time.
static const char* device_prio[] = { "gcldr", "sdc", "sdb", "sda" };
#define EMU_DEV_GCLDR 0

static bool passthrough = false;

const char* emu_get_device() {
	return emu_sd_device < 0 ? NULL : device_prio[emu_sd_device];
}

#ifndef IPL_CODE
// Cleared once config.ini has told us load_from_ode_sd is off, so the bootstrap
// fallback below cannot quietly pick the ODE back up on a later mount.
static bool ode_sd_allowed = true;

static void emu_mount_path(int device, char* path) {
	memcpy(path, device_prio[device], strlen(device_prio[device]) + 1);
	strcat(path, ":");
}

static bool emu_try_mount(int device) {
	static char mount_path[256];
	emu_mount_path(device, mount_path);

	iprintf("mount try %s ...\n", mount_path);
	if (f_mount(&fs, mount_path, 1) != FR_OK) {
		iprintf("mount %s fail\n", mount_path);
		return false;
	}

	iprintf("mount %s OK\n", mount_path);
	return true;
}

static void emu_unmount_current() {
	if (emu_sd_device < 0)
		return;

	dvd_custom_close(1); // drop whatever file/dir is still open

	static char mount_path[256];
	emu_mount_path(emu_sd_device, mount_path);
	f_mount(NULL, mount_path, 0);

	emu_sd_device = -1;
}

// Called once config.ini has been parsed, mirroring [cubeboot] load_from_ode_sd.
//
// on  -> the SD card inside the ODE becomes the volume everything after this
//        point is read from: the IPL dump, swiss-gc.dol, banners and the games
//        the menu lists.
// off -> the patched BIOS never gets handed the ODE volume, which is how
//        cubiboot has always behaved.
//
// The bootstrap mount in flippy_emu_mount() tries the ODE last on purpose:
// config.ini has to be readable *before* we know whether this setting is even
// on, and on an ODE-only console that card is the only place it can live.
void emu_apply_ode_preference(bool enabled) {
	if (!enabled) {
		ode_sd_allowed = false;
		if (emu_sd_device == EMU_DEV_GCLDR) {
			iprintf("load_from_ode_sd is off, releasing the ODE SD\n");
			emu_unmount_current();
		}
		return;
	}

	if (emu_sd_device == EMU_DEV_GCLDR)
		return;

	int previous = emu_sd_device;
	emu_unmount_current();

	if (emu_try_mount(EMU_DEV_GCLDR)) {
		emu_sd_device = EMU_DEV_GCLDR;
		return;
	}

	// No ODE answered. Fall back to whatever we were already on rather than
	// booting into an empty menu.
	iprintf("load_from_ode_sd is on but no ODE SD was found\n");
	if (previous >= 0 && emu_try_mount(previous))
		emu_sd_device = previous;
}
#endif

bool flippy_emu_mount() {
	#ifdef IPL_CODE

	static bool mounted = false;
	if (mounted)
		return true;

	if (emu_sd_device < 0)
		return false;

	static char mount_path[256];
	memcpy(mount_path, device_prio[emu_sd_device], strlen(device_prio[emu_sd_device]) + 1);
	strcat(mount_path, ":");
	if (f_mount(&fs, mount_path, 1) != FR_OK)
		return false;

	mounted = true;
	emu_update_boot();
	return true;

	#else

	if (emu_sd_device < 0) {
		// EXI card readers first. The ODE's own SD is the last resort so that a
		// console with no card reader at all can still bootstrap its config.ini
		// off it; emu_apply_ode_preference() then decides whether it is kept.
		int num_devices = sizeof(device_prio) / sizeof(device_prio[0]);
		for (int i = 0; i < num_devices; i++) {
			if (i == EMU_DEV_GCLDR)
				continue;

			if (emu_try_mount(i)) {
				emu_sd_device = i;
				return true;
			}
		}

		if (ode_sd_allowed && emu_try_mount(EMU_DEV_GCLDR)) {
			emu_sd_device = EMU_DEV_GCLDR;
			return true;
		}

		return false;
	}
	return true;

	#endif
}

static FIL file;
static FFDIR dir;

int dvd_custom_open(const char* path, uint8_t type, uint8_t flags) {
	if (!flippy_emu_mount())
		return 1;

	dvd_custom_close(1);

	char dev_path[256];
	memcpy(dev_path, emu_get_device(), strlen(emu_get_device()) + 1);
	strcat(dev_path, ":");
	strcat(dev_path, path);

	if (type == FILE_ENTRY_TYPE_DIR) {
		return f_opendir(&dir, dev_path) == FR_OK ? 0 : 1;
	}

	if (type == FILE_ENTRY_TYPE_FILE) {
		int ffs_flags = FA_READ;
		if (flags & IPC_FILE_FLAG_WRITE)
			ffs_flags |= FA_WRITE | FA_OPEN_ALWAYS;

		return f_open(&file, dev_path, ffs_flags) == FR_OK ? 0 : 1;
	}

	return 1;
}

int dvd_custom_open_flash(const char *path, uint8_t type, uint8_t flags) {
	if (type != FILE_ENTRY_TYPE_FILE)
		return 1;

	char flash_path[256];
	strcpy(flash_path, "/cubiboot");
	strcat(flash_path, path);
	if (dvd_custom_open(flash_path, type, flags) == 0)
		return 0;

	return dvd_custom_open(path, type, flags);
}

#ifdef IPL_CODE
static GCN_ALIGNED(file_status_t) _status;
file_status_t* dvd_custom_status() {
	file_status_t* status = &_status;
#else
int dvd_custom_status(file_status_t* status) {
#endif
	memset(status, 0, sizeof(file_status_t));
	status->fd = 1;
	
	if (file.obj.fs == NULL && dir.obj.fs == NULL) {
		status->result = 1;
		status->fsize = 0;
		#ifdef IPL_CODE
		return status;
		#else
		return 0;
		#endif
	}
	
	status->result = 0;
	status->fsize = __builtin_bswap64(f_size(&file));
	#ifdef IPL_CODE
	return status;
	#else
	return 0;
	#endif
}

int dvd_read(void* dst, unsigned int len, uint64_t offset, unsigned int fd) {
	if (passthrough) {
		extern int normal_dvd_read(void* dst, unsigned int len, uint64_t offset, unsigned int fd);
		return normal_dvd_read(dst, len, offset, fd);
	}

	FRESULT res;
	UINT bytes_read;
	
	res = f_lseek(&file, offset);
	if (res != FR_OK) {
		return 1;
	}
	
	res = f_read(&file, dst, len, &bytes_read);
	if (res != FR_OK) {
		return 1;
	}
	
	return 0;
}

int dvd_threaded_read(void* dst, unsigned int len, uint64_t offset, unsigned int fd) {
	return dvd_read(dst, len, offset, fd);
}

int dvd_custom_readdir(file_entry_t* dst, unsigned int fd) {
	FILINFO fno;
	FRESULT res;

	fno.fname[0] = 0;
	res = f_readdir(&dir, &fno);
	if (res != FR_OK)
		return 1;
	
	if (fno.fname[0] == 0) {
		dst->name[0] = 0;
		return 0;
	}

	strcpy(dst->name, fno.fname);
	dst->type = (fno.fattrib & AM_DIR) ? FILE_ENTRY_TYPE_DIR : FILE_ENTRY_TYPE_FILE;
	dst->size = fno.fsize;
	dst->attrib = fno.fattrib;
	
	return 0;
}

int dvd_custom_mkdir(char* path) {
	if (!flippy_emu_mount())
		return 1;

	return f_mkdir(path) == FR_OK ? 0 : 1;
}

void dvd_custom_close(uint32_t fd) {
	f_close(&file);
	f_closedir(&dir);
}

void dvd_custom_bypass_enter() {
	passthrough = true;
	#ifdef IPL_CODE
	dvd_reset();
	#else
	DI_Reset();
	#endif
}

void dvd_custom_bypass() {
	dvd_custom_bypass_enter();
}

void dvd_custom_bypass_exit() {
	#ifdef IPL_CODE
	dvd_stop_motor();
	#else
	DI_StopMotor();
	#endif
	passthrough = false;
}


// not implemented
int dvd_custom_write(char *buf, uint32_t offset, uint32_t length, uint32_t fd) {
	return 1;
}

void dvd_set_default_fd(uint32_t current_fd, uint32_t second_fd) {

}

int dvd_custom_unlink(char *path) {
	return 1;
}

int dvd_custom_unlink_flash(char *path) {
	return 1;
}

int dvd_custom_presence(bool playing, const char *status, const char *sub_status) {
	return 1;
}

int dvd_custom_fs_info(fs_info_t* status) {
	return 1;
}
