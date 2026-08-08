#include "../flippy_sync.h"

#include <string.h>
#include "ffs/ff.h"
#include "tweaks.h"
#include "drive_probe.h"
#include "fldrv.h"

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
//
// These are FatFs volume names, with one exception: "fldrv" is a FlippyDrive, which
// serves files itself over the drive interface and has no volume to mount. It is
// named the way Swiss names it, because the name is handed to Swiss verbatim in the
// Autoload= argument (see emu/loader.c) -- an invented spelling would need a
// translation step there.
//
// New devices go on the END. The index is the loader-to-patches contract, so
// inserting one in the middle silently renumbers every device above it.
static const char* device_prio[] = { "gcldr", "sdc", "sdb", "sda", "fldrv" };
#define EMU_DEV_GCLDR 0
#define EMU_DEV_FLDRV 4

// Whether this device serves files itself instead of being a FatFs volume. Such a
// device must never reach f_mount, f_open or a "<vol>:" path.
static inline bool emu_dev_is_native(int device) {
	return device == EMU_DEV_FLDRV;
}

// Whether the device currently in use is one. Every dvd_custom_* entry point below
// forks on this: same API, either the drive answers it or FatFs does.
static inline bool emu_is_native(void) {
	return emu_sd_device >= 0 && emu_dev_is_native(emu_sd_device);
}

static bool passthrough = false;

const char* emu_get_device() {
	return emu_sd_device < 0 ? NULL : device_prio[emu_sd_device];
}

#ifndef IPL_CODE
// The file whose presence decides which device the loader reads its settings from.
#define EMU_CONFIG_PATH "/config.ini"

// One order for the whole loader: it is where the bootstrap looks for config.ini, and it is
// what device_order falls back to when config.ini does not set it. Card readers first, the
// ODE last, so a console without one only reaches the drive-interface inquiry once
// everything else has been ruled out.
// fldrv sits beside gcldr rather than after the readers for a reason of its own: the
// two are the same physical connector, so only one of them can ever answer, and their
// relative order never decides anything on a real console.
#define EMU_DEFAULT_DEVICE_ORDER "sdc, sdb, sda, gcldr, fldrv"

static void emu_mount_path(int device, char* path) {
	memcpy(path, device_prio[device], strlen(device_prio[device]) + 1);
	strcat(path, ":");
}

// Hardware spellings for the FatFs volume names. Someone writing a config.ini thinks in
// terms of the thing plugged into their console, not in terms of a FatFs volume -- and
// "sdc" meaning serial port 2 is not guessable. Both are accepted: the volume names have
// to keep working because they are what the loader's own logging prints.
static const struct { const char* alias; const char* volume; } emu_device_aliases[] = {
	{ "sd2sp2",      "sdc"   },
	{ "slot_b",      "sdb"   },
	{ "slot_a",      "sda"   },
	{ "gcloader",    "gcldr" },
	{ "flippy",      "fldrv" },
	{ "flippydrive", "fldrv" },
	// "ode" is deliberately absent: it names a category, not one device, and is
	// resolved against what is actually on the bus. See emu_find_device().
};

static int emu_match_volume(const char* name, int len) {
	int count = sizeof(device_prio) / sizeof(device_prio[0]);
	for (int i = 0; i < count; i++) {
		if ((int)strlen(device_prio[i]) == len && strncasecmp(device_prio[i], name, len) == 0)
			return i;
	}

	return -1;
}

static bool emu_name_is(const char* name, int len, const char* candidate) {
	return (int)strlen(candidate) == len && strncasecmp(candidate, name, len) == 0;
}

static int emu_find_device(const char* name, int len) {
	int device = emu_match_volume(name, len);
	if (device >= 0)
		return device;

	// "ode" means "whichever ODE is installed", not one particular protocol. There is
	// a single drive connector, so a GC Loader and a FlippyDrive can never both be
	// present -- which makes this answerable rather than ambiguous: ask the drive.
	//
	// Resolving it here rather than expanding it into two entries keeps the list a
	// plain sequence of devices, and costs nothing: drive_probe() caches its inquiry,
	// and by the time a device_order line is parsed the bootstrap has already probed.
	// A console with neither falls through to gcldr, which then fails to mount exactly
	// as it does today.
	if (emu_name_is(name, len, "ode")) {
		return drive_probe() == DRIVE_ID_FLIPPY ? EMU_DEV_FLDRV : EMU_DEV_GCLDR;
	}

	int aliases = sizeof(emu_device_aliases) / sizeof(emu_device_aliases[0]);
	for (int i = 0; i < aliases; i++) {
		const char* alias = emu_device_aliases[i].alias;
		if (emu_name_is(name, len, alias))
			return emu_match_volume(emu_device_aliases[i].volume, strlen(emu_device_aliases[i].volume));
	}

	iprintf("device_order: ignoring unknown device '%.*s'\n", len, name);
	return -1;
}

// Walks a comma or space separated device list. Writes the next device index into *device
// (-1 when the name is not one we know) and returns where to resume, or NULL at the end.
static const char* emu_next_device(const char* p, int* device) {
	while (*p == ' ' || *p == '\t' || *p == ',')
		p++;

	if (*p == '\0')
		return NULL;

	const char* start = p;
	while (*p != '\0' && *p != ' ' && *p != '\t' && *p != ',')
		p++;

	*device = emu_find_device(start, (int)(p - start));
	return p;
}

static bool emu_try_mount(int device) {
	if (emu_dev_is_native(device)) {
		// A FlippyDrive is "mounted" by being present: it serves paths itself, so there
		// is no volume and f_mount would be meaningless. All that is needed is to take
		// back the handles the bootloader left open.
		if (!fldrv_init()) {
			iprintf("mount %s fail\n", device_prio[device]);
			return false;
		}

		iprintf("mount %s OK (native)\n", device_prio[device]);
		return true;
	}

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

static void emu_drop_mount(int device) {
	if (emu_dev_is_native(device)) {
		// Nothing was mounted, so there is nothing to unmount. The probe result stays
		// cached either way -- the drive does not stop being there.
		return;
	}

	static char mount_path[256];
	emu_mount_path(device, mount_path);
	f_mount(NULL, mount_path, 0);
}

// Mounts `device` and reports whether it actually carries a config.ini, leaving it
// unmounted when it does not.
//
// Looking only at the first device that *mounts* is what made this confusing: with a card
// reader and an ODE both present, whichever mounted first was the only one ever consulted,
// so a config.ini sitting on the other card was invisible -- a console with an empty SD2SP2
// and the config on the ODE booted with default settings and no games. Moving the file to
// the other card only moved the failure. Asking every device removes the guesswork: the
// file is found wherever the user put it.
static bool emu_try_mount_with_config(int device) {
	if (!emu_try_mount(device))
		return false;

	if (emu_dev_is_native(device)) {
		// The drive takes the path as-is; there is no volume to prefix. This reaches the
		// SD card in the FlippyDrive, which is where config.ini lives -- the drive's
		// internal flash holds only the loader the bootloader autoloads.
		if (fldrv_open(EMU_CONFIG_PATH, FILE_ENTRY_TYPE_FILE, 0) != 0) {
			iprintf("no %s on %s\n", EMU_CONFIG_PATH, device_prio[device]);
			return false;
		}

		// Hand the handle straight back. Real handles are a finite resource here, and
		// leaking one from a probe is how you end up short later for no visible reason.
		static GCN_ALIGNED(file_status_t) probe_status;
		if (fldrv_status(&probe_status) == 0)
			fldrv_close(probe_status.fd);

		iprintf("found %s on %s\n", EMU_CONFIG_PATH, device_prio[device]);
		return true;
	}

	char probe_path[256 + sizeof(EMU_CONFIG_PATH)];
	emu_mount_path(device, probe_path);
	strcat(probe_path, EMU_CONFIG_PATH);

	FIL probe;
	if (f_open(&probe, probe_path, FA_READ) != FR_OK) {
		iprintf("no %s on %s\n", EMU_CONFIG_PATH, device_prio[device]);
		emu_drop_mount(device);
		return false;
	}

	f_close(&probe);
	iprintf("found %s on %s\n", EMU_CONFIG_PATH, device_prio[device]);
	return true;
}

static void emu_unmount_current() {
	if (emu_sd_device < 0)
		return;

	dvd_custom_close(1); // drop whatever file/dir is still open

	if (emu_dev_is_native(emu_sd_device)) {
		emu_sd_device = -1;
		return;
	}

	static char mount_path[256];
	emu_mount_path(emu_sd_device, mount_path);
	f_mount(NULL, mount_path, 0);

	emu_sd_device = -1;
}

// Called once config.ini has been parsed, with the raw [cubeboot] device_order value.
//
// The list names FatFs volumes -- sdc (SD2SP2), sdb (memory card slot B), sda (slot A) and
// gcldr (the card inside a GC Loader style ODE) -- and the first one that mounts becomes the
// volume everything after this point is read from: the IPL dump, swiss-gc.dol, banners and
// the games the menu lists. Leaving a device out of the list is how you keep cubiboot off
// it; there is no separate on/off switch.
//
// A NULL or empty value means config.ini did not ask for anything, and the bootstrap already
// settled on a device using the same default order, so there is nothing to redo.
void emu_apply_device_order(const char* order) {
	if (order == NULL || *order == '\0')
		return;

	int previous = emu_sd_device;
	bool moved = false;
	int device;

	for (const char* p = order; (p = emu_next_device(p, &device)) != NULL; ) {
		if (device < 0)
			continue;

		// Already sitting on the first device the user asked for.
		if (!moved && device == previous)
			return;

		if (!moved) {
			emu_unmount_current();
			moved = true;
		}

		if (emu_try_mount(device)) {
			emu_sd_device = device;
			return;
		}
	}

	// Nothing in the list mounted. Fall back to whatever the bootstrap had rather than
	// booting into an empty menu over one bad line in config.ini.
	iprintf("device_order: nothing usable, keeping %s\n",
	        previous >= 0 ? device_prio[previous] : "(none)");
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

	if (emu_is_native()) {
		// The loader already took the handles back before handing over; this side only
		// has to confirm the drive is still the one answering.
		if (!fldrv_init())
			return false;

		mounted = true;
		emu_update_boot();
		return true;
	}

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
		int device;

		// Two passes over the default order. The first takes the device that actually
		// holds a config.ini, so the file is honoured wherever the user put it -- the
		// order only breaks a tie between two cards that both have one. device_order
		// cannot help here: it lives inside the file being looked for.
		for (const char* p = EMU_DEFAULT_DEVICE_ORDER; (p = emu_next_device(p, &device)) != NULL; ) {
			if (device >= 0 && emu_try_mount_with_config(device)) {
				emu_sd_device = device;
				return true;
			}
		}

		// Nothing carries a config.ini. Settle for the first device that mounts, so a card
		// holding games but no config still works.
		for (const char* p = EMU_DEFAULT_DEVICE_ORDER; (p = emu_next_device(p, &device)) != NULL; ) {
			if (device >= 0 && emu_try_mount(device)) {
				emu_sd_device = device;
				return true;
			}
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

	if (emu_is_native()) {
		// Straight through, path unchanged: the drive resolves it. Both files and
		// directories are the same call, distinguished by type, so there is no separate
		// opendir here the way FatFs needs one.
		//
		// Note this skips the close below. That close exists because the emulation keeps
		// exactly one FIL and one FFDIR, so opening a second thing without closing the
		// first would strand it -- fd 1 is a fiction it hands to every caller. A real
		// drive gives out real handles, several can be open at once, and callers close
		// the fd they were given. Closing "1" here would shut whichever file happened to
		// hold that handle.
		return fldrv_open(path, type, flags);
	}

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

	if (emu_is_native()) {
		if (!flippy_emu_mount())
			return 1;

		// The drive has real flash of its own, so this is a different command rather
		// than the /cubiboot-on-the-card impersonation used below. It is also where the
		// loader we are running came from. No close first, for the same reason as in
		// dvd_custom_open().
		if (fldrv_open_flash(path, type, flags) == 0)
			return 0;

		// Then the SD card, because the two callers of this want different things. The
		// drive's flash is right for what the drive shipped -- stub.bin, its own Swiss --
		// but apploader.img has to be OUR build's, and the card root is where the docs
		// tell people to put it. Flash-only here would silently cost FlippyDrive owners
		// In-Game Reset, since a drive whose flash has no /swiss/patches would simply
		// report no file and the option would switch itself off.
		return fldrv_open(path, type, flags);
	}

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
	if (emu_is_native()) {
		// Straight from the drive: a real handle and the real size, where everything
		// below is the emulation manufacturing both. Note it is not byte-swapped here --
		// the swap below exists to imitate the format the drive already sends.
		if (fldrv_status(status) != 0) {
			memset(status, 0, sizeof(file_status_t));
			status->result = 1;
		}

		#ifdef IPL_CODE
		return status;
		#else
		return 0;
		#endif
	}

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
	// A FlippyDrive serves file reads through the ordinary drive read command with the
	// handle in it -- the same transfer passthrough already uses for a real disc, which
	// is why there is nothing device-specific to add here. Callers that cannot promise
	// an aligned buffer, length and offset go through dvd_read_data() instead, exactly
	// as they must for passthrough.
	if (passthrough || emu_is_native()) {
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
	if (emu_is_native()) {
		// The drive fills in a file_entry_t directly -- the same struct this function
		// spends its FatFs half translating a FILINFO into. An empty name still means
		// end of directory, so callers see no difference.
		return fldrv_readdir(dst, fd);
	}

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
	if (emu_is_native()) {
		fldrv_close(fd);
		return;
	}

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
	// Only a real drive can act on this: it tells the drive which open file to present
	// as the disc, so an image boots through the drive itself. There is nothing for the
	// FatFs emulation to do, which is why it stayed empty.
	if (emu_is_native())
		fldrv_set_default_fd(current_fd, second_fd);
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
