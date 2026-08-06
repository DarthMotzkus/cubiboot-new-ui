#ifndef SECTOR_CACHE_H
#define SECTOR_CACHE_H

#include "ff.h"
#include "diskio.h"

// A cache of raw device sectors, sitting between FatFs and the storage drivers.
//
// None of the drivers cache anything, and the SD paths issue one command per 512-byte
// sector. FatFs finds a file by walking its directory from the start, once per open --
// so opening N files in one folder re-walks the same sectors N times. That is the whole
// cost of a large game list: not the banners, which are read once each, but the same few
// dozen directory sectors fetched over and over.
//
// disk_read()/disk_write() in diskio.c route through here. Nothing else in FatFs, and
// nothing above it, is aware this exists.

DRESULT sector_cache_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
DRESULT sector_cache_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);

// Drops everything. Called automatically when a request arrives for a different drive
// than the one currently cached, which is what happens while the loader probes devices.
void sector_cache_invalidate(void);

// Takes the cache out of the path for good: reads and writes go straight to the device
// from here on. One-way on purpose -- both callers are on the way out of the menu and into
// a game, so there is nothing to come back for, and an enable() nobody calls would imply a
// lifecycle this does not have.
//
// MUST be called before anything that writes over the RAM the pages live in. The pages
// and the bookkeeping that describes them are deliberately in the same region so that a
// wipe takes both (see the note on the magic in sector_cache.c), but that is a backstop
// for a whole-region wipe, not a guarantee -- a partial overwrite can destroy pages and
// leave the bookkeeping intact, after which every read returns whatever landed there.
//
// The caller that matters is bs2start(), which clears 0x80100000-0x81600000 before every
// boot. That range contains these pages.
void sector_cache_disable(void);

// Uncached device access, implemented by diskio.c. Only this cache should call them.
DRESULT disk_read_raw(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_write_raw(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);

#endif // SECTOR_CACHE_H
