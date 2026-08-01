/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "../tsd.h"
#include "../gcode.h"

static bool disk_is_sd(BYTE pdrv) {
	return pdrv == 0 || pdrv == 1 || pdrv == 2;
}

// SD card sitting behind a GC Loader style ODE, reached over the drive
// interface rather than EXI. See cubeboot/source/emu/gcode.c.
static bool disk_is_gcode(BYTE pdrv) {
	return pdrv == DEV_GCLDR;
}

static const exi_port exi_port_map[FF_VOLUMES] = { { 0, 0 }, { 1, 0 }, { 2, 0 } };

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) {
	if (disk_is_sd(pdrv) || disk_is_gcode(pdrv))
		return 0;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
	if (disk_is_sd(pdrv))
		return tsd_sd_init(exi_port_map[pdrv]) ? 0 : STA_NOINIT;

	if (disk_is_gcode(pdrv))
		return gcode_sd_init() ? 0 : STA_NOINIT;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
	if (disk_is_sd(pdrv))
		return tsd_sd_read(exi_port_map[pdrv], sector, buff, count) ? RES_OK : RES_ERROR;

	if (disk_is_gcode(pdrv))
		return gcode_sd_read(sector, buff, count) ? RES_OK : RES_ERROR;

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
	if (disk_is_sd(pdrv))
		return tsd_sd_write(exi_port_map[pdrv], sector, buff, count) ? RES_OK : RES_WRPRT;

	if (disk_is_gcode(pdrv))
		return gcode_sd_write(sector, buff, count) ? RES_OK : RES_WRPRT;

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
	if (disk_is_sd(pdrv) || disk_is_gcode(pdrv)) {
		switch (cmd) {
			case CTRL_SYNC:
				return RES_OK;
				
			case GET_SECTOR_COUNT:
				*(DWORD*)buff = 0xFFFFFFFF; // who cares
				return RES_OK;
				
			case GET_SECTOR_SIZE:
				*(WORD*)buff = 512;
				return RES_OK;
				
			case GET_BLOCK_SIZE:
				*(DWORD*)buff = 1;
				return RES_OK;
				
			default:
				return RES_PARERR;
		}
	}

	return RES_PARERR;
}


/*-----------------------------------------------------------------------*/
/* Shutdown Drive                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_shutdown(BYTE pdrv) {
	if (disk_is_sd(pdrv) || disk_is_gcode(pdrv))
		return RES_OK;

	return RES_PARERR;
}

