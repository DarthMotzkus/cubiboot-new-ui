#include "sector_cache.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef IPL_CODE
#include "../../attr.h"
// .data_lowmem is NOLOAD: reserved RAM that costs nothing in the image. That matters here
// -- 384 KB of pages in a loaded section would be 384 KB added to ipl.dol. Nothing zeroes
// it at runtime, which is fine for page contents (a page is filled from the card before it
// is ever read) but is why the bookkeeping needs the magic below.
#define SECTOR_CACHE_MEM __attribute_aligned_data_lowmem__
#else
#define SECTOR_CACHE_MEM __attribute__((aligned(32)))
#endif

// disk_ioctl() reports 512 for every drive we support.
#define SECTOR_SZ 512

// Sectors per page. Bigger pages read ahead, which suits this workload exactly: both a
// directory walk and a FAT chain walk run forwards. They also cost more on a miss, and a
// miss here is one device read either way.
#define PAGE_SECTORS 4
#define PAGE_SZ      (PAGE_SECTORS * SECTOR_SZ)

// 192 pages x 2 KB = 384 KB.
//
// SIZING: this is a cliff, not a dial. The access pattern that matters is a directory
// swept from the start, over and over -- once per file opened. If the directory fits, every
// sweep after the first costs nothing. If it does not fit, each page is evicted before it
// is revisited and the hit rate is zero, whatever the replacement policy. So the only
// question is whether a realistic directory fits:
//
//     directory bytes ~= entries * 32 * (1 + ceil(name_len / 13))
//
// A 200-game folder with 40-character names is ~32 KB. games.c caps a listing at 1920
// entries, which is ~300 KB at the same name length. 384 KB covers that cap with room, and
// leaves ~350 KB of the region still free afterwards.
#define PAGE_COUNT 192

#define PAGE_EMPTY (~(LBA_t)0)

// DIRECT-MAPPED, deliberately, rather than a list with a replacement policy.
//
// A page's slot is fixed: page number modulo PAGE_COUNT. For a sweep whose working set
// fits, every page lands in a slot of its own and the hit rate is 100% with no bookkeeping
// at all -- no list to walk on every access, no ordering to maintain. LRU buys nothing
// here, because a repeated forward sweep never rewards recency: when the set fits nothing
// is evicted, and when it does not, LRU evicts exactly the page about to be wanted.
//
// It also makes a lookup O(1). A 192-entry list searched on every sector access is real
// work in the middle of the path this is supposed to make faster.
static SECTOR_CACHE_MEM BYTE cache_data[PAGE_COUNT * PAGE_SZ];

// Bookkeeping lives in the SAME region as the pages, on purpose: anything that wipes the
// region takes the magic with it, so the cache notices and rebuilds rather than serving
// zeros out of pages that were cleared underneath it. It also covers first use, where this
// region is NOLOAD and holds whatever was there before.
//
// This is a backstop, not a guarantee -- an overwrite that lands on pages but misses these
// few bytes is undetectable. Callers that clear memory must still call
// sector_cache_disable() themselves.
#define CACHE_MAGIC 0x53454331u // 'SEC1'
static SECTOR_CACHE_MEM uint32_t cache_magic;
static SECTOR_CACHE_MEM LBA_t page_base[PAGE_COUNT];
static SECTOR_CACHE_MEM BYTE cache_pdrv;

static bool cache_disabled;

static void cache_reset(BYTE pdrv) {
	for (unsigned i = 0; i < PAGE_COUNT; i++)
		page_base[i] = PAGE_EMPTY;

	cache_pdrv = pdrv;
	cache_magic = CACHE_MAGIC;
}

void sector_cache_invalidate(void) {
	cache_magic = 0; // next access rebuilds
}

void sector_cache_disable(void) {
	// Bypass the pages and mark them untrusted both. cache_disabled deliberately lives
	// outside the cached region, so the wipe that follows cannot un-disable it.
	cache_disabled = true;
	cache_magic = 0;
}

static inline unsigned page_slot(LBA_t page) {
	return (unsigned)(page % PAGE_COUNT);
}

static inline BYTE *page_data(unsigned slot) {
	return cache_data + slot * PAGE_SZ;
}

// Called on every request: rebuilds the first time through, when the pages have been
// destroyed underneath us, and when FatFs switches drives -- which is what happens while
// the loader works down its device list.
static void cache_claim(BYTE pdrv) {
	if (cache_magic != CACHE_MAGIC || pdrv != cache_pdrv)
		cache_reset(pdrv);
}

static DRESULT cache_access(BYTE pdrv, BYTE *buffer, LBA_t first_sector,
                            UINT num_sectors, bool is_write) {
	// A transfer of a whole page or more is a file being streamed -- a banner, a DOL, an
	// ISO. Those are read once and never looked at again, so caching them would evict the
	// directory sectors this exists to keep, to no benefit. Straight to the device.
	if (num_sectors >= PAGE_SECTORS) {
		if (!is_write)
			return disk_read_raw(pdrv, buffer, first_sector, num_sectors);

		// A bulk write still has to leave the cache truthful about any page it covers.
		for (UINT i = 0; i < num_sectors; i++) {
			LBA_t page = (first_sector + i) / PAGE_SECTORS;
			unsigned slot = page_slot(page);
			if (page_base[slot] == page)
				page_base[slot] = PAGE_EMPTY;
		}

		return disk_write_raw(pdrv, buffer, first_sector, num_sectors);
	}

	while (num_sectors) {
		LBA_t page = first_sector / PAGE_SECTORS;
		unsigned offset = (unsigned)(first_sector % PAGE_SECTORS);
		UINT run = PAGE_SECTORS - offset;
		if (run > num_sectors)
			run = num_sectors;

		unsigned slot = page_slot(page);
		BYTE *cached = page_data(slot) + offset * SECTOR_SZ;

		if (is_write) {
			DRESULT res = disk_write_raw(pdrv, buffer, first_sector, run);
			if (res != RES_OK)
				return res;

			// Write-through, so the device is authoritative and the page only has to
			// agree with it. Upstream FatFs glue often writes back instead, which needs a
			// flush hooked to CTRL_SYNC and loses data if any path misses it -- for a menu
			// that essentially never writes, that is risk bought for nothing.
			if (page_base[slot] == page)
				memcpy(cached, buffer, run * SECTOR_SZ);
		} else {
			if (page_base[slot] != page) {
				// Fill the whole page, not just the sectors asked for: the next request is
				// almost always the sectors right after these.
				page_base[slot] = PAGE_EMPTY; // not valid until the read lands
				if (disk_read_raw(pdrv, page_data(slot), page * PAGE_SECTORS,
				                  PAGE_SECTORS) != RES_OK) {
					// Usually a page running past the end of the volume. Serve what was
					// actually asked for, uncached, and leave the slot empty.
					return disk_read_raw(pdrv, buffer, first_sector, num_sectors);
				}

				page_base[slot] = page;
			}

			memcpy(buffer, cached, run * SECTOR_SZ);
		}

		buffer += run * SECTOR_SZ;
		first_sector += run;
		num_sectors -= run;
	}

	return RES_OK;
}

DRESULT sector_cache_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
	if (count == 0)
		return RES_OK;

	if (cache_disabled)
		return disk_read_raw(pdrv, buff, sector, count);

	cache_claim(pdrv);
	return cache_access(pdrv, buff, sector, count, false);
}

DRESULT sector_cache_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
	if (count == 0)
		return RES_OK;

	if (cache_disabled)
		return disk_write_raw(pdrv, buff, sector, count);

	cache_claim(pdrv);
	return cache_access(pdrv, (BYTE *)buff, sector, count, true);
}
