#ifndef GCODE_H
#define GCODE_H

#include <stdint.h>
#include <stdbool.h>
#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Block driver for the SD/storage attached to a GC Loader style ODE, reached
// through the drive interface instead of EXI. Sector size is 512 bytes.
//
// gcode_sd_init() just asks drive_probe() whether the drive on the bus is a GC
// Loader, so calling it repeatedly is free -- the inquiry, the retries while the
// drive wakes up, and the caching all live there and are shared with the other
// backend that talks to the same connector. See emu/drive_probe.h.
bool gcode_sd_init(void);
bool gcode_sd_read(uint32_t sector, uint8_t *data, uint32_t count);
bool gcode_sd_write(uint32_t sector, const uint8_t *data, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif // GCODE_H
