# Architecture

How cubiboot is put together, and why it is split the way it is. This is the map you want
before changing anything under `cubeboot/`, `patches/` or `entry/`.

For user-facing docs see the [README](../README.md); for the diff against upstream see
[FORK_CHANGES.md](FORK_CHANGES.md).

## Contents

- [Boot chain](#boot-chain)
- [Stage 1: entry](#stage-1-entry)
- [Stage 2: the loader](#stage-2-the-loader)
- [Stage 3: the injected menu](#stage-3-the-injected-menu)
- [How the BIOS is patched](#how-the-bios-is-patched)
- [The loader to patches contract](#the-loader-to-patches-contract)
- [The shared storage stack](#the-shared-storage-stack)
- [Memory map](#memory-map)
- [Build pipeline](#build-pipeline)
- [Booting a game](#booting-a-game)

## Boot chain

```
PicoBoot / PicoLoader / gekkoboot        GC Loader / ODE
            |                                  |
            v                                  v
        ipl.dol  ( = entry.dol )          cubiboot.iso
            |                                  |
            +---------------+------------------+
                            v
                    stage 1: entry
              gunzips the real loader, jumps
                            |
                            v
                  stage 2: cubeboot.dol
        reads config.ini, loads the IPL image, applies
        patches.elf into it, hands over control
                            |
                            v
                stage 3: patched BS2 (BIOS)
        stock boot animation + the cubiboot menu, which
        chainloads Swiss to run a game
```

Both distribution formats converge on the same stage 1. `cubiboot.iso` is an El-Torito
ISO9660 image whose boot payload *is* the loader `.dol`; the ODE reads it straight off the
disc. See [.ci/build_iso.sh](../.ci/build_iso.sh).

## Stage 1: entry

`entry/` builds a minimal freestanding DOL (no libogc, `--specs=nosys.specs`,
[entry/nosys.ld](../entry/nosys.ld)) that contains `cubeboot.dol` gzipped as
`data/cubeboot.gz`. It installs a syscall handler, inflates the payload with `tinf` and jumps
to it.

Why: the loader is ~666 KB and gzip gets it to ~58% of that, which matters for flashing it
into an RP2040 and for load time off an SD card.

`entry.dol` is what the release publishes as **`ipl.dol`**.

The single RWX-segment linker warning at this stage is expected — `nosys.ld` deliberately
lays out one contiguous self-contained blob at a fixed address.

## Stage 2: the loader

`cubeboot/source/`, built with libogc2. `main()` in
[cubeboot/source/main.c](../cubeboot/source/main.c) runs roughly:

1. **Video up** (`VIDEO_Init`, mode picked from `VIDEO_GetCurrentTvMode`). The framebuffer is
   parked above the patch blob, at `_patches_end` rounded up.
2. **Parse the embedded `patches.elf`** — section header table, symbol table, string tables.
   The ELF is linked into the loader as a binary blob (`data/patches.elf` → `patches_elf.h`).
3. **`load_settings()`** ([settings.c](../cubeboot/source/settings.c)) reads `/config.ini`.
   This is the first file access, so it is what triggers the storage device probe.
4. **`emu_apply_device_order()`** settles which volume the rest of the boot uses, now that
   `device_order` is known. See [the storage stack](#the-shared-storage-stack).
5. **`load_ipl()`** ([ipl.c](../cubeboot/source/ipl.c)) reads `/ipl.bin` if present, else
   dumps the console's own ROM. It descrambles from `0x100` to the SJIS font, CRCs the result
   to identify the revision, and copies BS2 to `0x81300000`.
6. **Apply the patches** — see [below](#how-the-bios-is-patched).
7. **Inject the settings** with `set_patch_value` — see
   [the contract](#the-loader-to-patches-contract).
8. **Tear down libogc** (`GX_AbortFrame`, `ASND_End`, `SYS_ResetSystem(SYS_SHUTDOWN)`,
   restoring the three low-memory pointers that reset clobbers) and
   `__lwp_thread_stopmultitasking(bs2entry)` into the patched BIOS.

### IPL revisions

Seven are supported, keyed by CRC of the decoded image
([ipl.c](../cubeboot/source/ipl.c), `bios_table[]`):

| Revision | `reloc_prefix` | `patch_suffix` |
|---|---|---|
| NTSC 1.0 | `ntsc10` | `VER_NTSC_10` |
| NTSC 1.1 | `ntsc11` | `VER_NTSC_11` |
| NTSC 1.2 (DOL-001) | `ntsc12_001` | `VER_NTSC_12_001` |
| NTSC 1.2 (DOL-101) | `ntsc12_101` | `VER_NTSC_12_101` |
| PAL 1.0 | `pal10` | `VER_PAL_10` |
| PAL 1.1 / MPAL | `pal11` | `VER_PAL_11` |
| PAL 1.2 | `pal12` | `VER_PAL_12` |

Every stock-BIOS address the menu calls exists once per revision. That is what the two
columns above are for.

## Stage 3: the injected menu

`patches/source/`, built freestanding with `-DIPL_CODE -ffixed-r12 -ffixed-r13
-ffunction-sections -fdata-sections`. No libogc, no libc beyond a bundled picolibc subset.
It links against the stock BIOS's own functions.

`patches/linker/` holds one script per revision plus `combined.ld`, so a single `patches.elf`
carries the code for all seven and the loader picks at runtime.

Key files:

| File | What it does |
|---|---|
| `main.c` | patch entry points, boot animation hooks, the injected config variables |
| `games.c` | directory scan, banner/icon pools, last-played, background loading |
| `menu.c` | menu structure, the "Games" screen |
| `grid.c` | grid geometry per `menu_grid_type` |
| `gameid.c`, `iso9660.c`, `gc_dvd.c` | reading disc headers and banners |
| `emu/` | **copied from `cubeboot/source/emu/`** — the shared storage stack |

## How the BIOS is patched

Two mechanisms, both driven by the loader walking `patches.elf`:

### 1. Code patches — `.patch.*` sections

A section named

```
.patch.<name>_<patch_suffix>_func
```

is copied verbatim to its `sh_addr`, overwriting stock BIOS code. The loader only applies
sections whose suffix matches the detected revision, so `.patch._force_lang_VER_PAL_12_func`
is skipped on an NTSC console.

The number of bytes to copy is **not** the section size. The loader looks up a companion
symbol `<name>_<patch_suffix>_size` (strip `_func`, append `_size`) and uses that, falling
back to `sh_size` when absent. This lets a patch be assembled with padding while overwriting
only the intended instructions.

`sh_addr` is masked to `0x8XXXXXXX` before use.

### 2. Symbol relocations — the `.reloc` section

`.reloc` is a table of pointers the injected code uses to call into the stock BIOS. For every
symbol whose value falls inside `.reloc`, the loader looks up

```
<reloc_prefix>_<symbol name>
```

and writes that address into the slot. So `OSReport` in `.reloc` becomes `ntsc11_OSReport`'s
address on an NTSC 1.1 console.

**A missing relocation is fatal** — the loader calls `prog_halt("Failed BIOS Patching
relocation")` rather than jumping into a BIOS with null function pointers.

`patches/scripts/validate-patches.go` runs at build time and flags targets it can't resolve;
`make validate` in `cubeboot/` lists them.

### Section types matter

The loader **skips `SHT_NOBITS` sections** — it copies content, it does not zero BSS. This is
the direct cause of the cold-boot banner bug documented in [FORK_CHANGES.md](FORK_CHANGES.md)
§B: `.data_lowmem` is `NOLOAD`/NOBITS, so the banner pools came up holding whatever was in
RAM. They are now explicitly zeroed by `gm_init_heap()`.

## The loader to patches contract

The loader has libogc, a heap and a filesystem. The injected menu has none of that at the
moment the loader is still running. Everything the menu needs to know is therefore written
into its variables *before* the jump, by symbol name:

```c
set_patch_value(symshdr, syment, symstringdata, "remember_last_game", settings.remember_last_game);
```

which finds the symbol in `patches.elf` and does `*(u32*)ptr = value`. On the other side the
variable is declared `__attribute_data__` (i.e. `section(".data")`) so it lands in an
allocated, copied section:

```c
__attribute_data__ u32 remember_last_game = 0;
```

Current contract:

| Symbol | Source |
|---|---|
| `cube_color` | `config.ini` |
| `theme_color`, `menu_cube_color`, `menu_box_color`, `menu_start_color` | `config.ini` (see below) |
| `force_progressive` | `config.ini` |
| `force_widescreen` | `config.ini` |
| `swiss_on_dvd_boot` | `config.ini` |
| `disable_mcp_select` | `config.ini` |
| `remember_last_game` | `config.ini` |
| `show_watermark` | `config.ini` |
| `preboot_delay_ms`, `postboot_delay_ms` | `config.ini` |
| `text_scroll_enabled`, `text_scroll_delay_s`, `title_scroll_step_frames` | `config.ini` |
| `menu_grid_type` | `config.ini` |
| `default_folder`, `cube_logo_path` | `config.ini` (strings, `strcpy`'d into the patch buffer) |
| `start_passthrough_game` | argv / the `PASS` magic at `0x80001800` |
| `is_running_dolphin` | ECID probe (`helpers.c`) |
| **`emu_sd_device`** | which storage volume the loader mounted |

`emu_sd_device` is the important one: it is an **index into `device_prio[]`**, and that array
lives in `flippy_emu.c`, which is compiled into *both* programs. The loader picks a device
and passes the index; the menu mounts `device_prio[emu_sd_device]`. Change the array and both
sides change together — that is the whole point of the shared directory.

Adding a new setting means touching four places: `settings.h`, `settings.c` (parse),
`main.c` (`set_patch_value`), and a `__attribute_data__` declaration in `patches/source/`.

The color settings are the one group that needs more than a raw value. `set_patch_value` only
moves single words, and `0` has to keep meaning "key absent", so a configured color carries a
tag in its top byte — see the `CFG_COLOR_*` macros in `settings_types.h`, which is a symlink
shared by both sides. That is also what lets `menu_cube_color` accept either a hex color or
one of the IPL's own palette names through a single u32. The loader only parses
(`ini_get_color`); every fallback and derivation happens menu-side in `patches/source/theme.c`,
so `docs/settings.md` describes behaviour that lives in exactly one file.

## The shared storage stack

`cubeboot/source/emu/` is copied to `patches/source/emu/` by `entry/Makefile` on every build.
Files there compile for both worlds and branch on `IPL_CODE`:

```c
#ifdef IPL_CODE
#include "../os.h"      // DCInvalidateRange from the stock BIOS
#include "../time.h"    // udelay
#else
#include <ogc/cache.h>  // libogc
#include <ogc/lwp_watchdog.h>
#endif
```

Layers, top to bottom:

```
dvd_custom_open / dvd_read / dvd_custom_readdir ...   flippy_emu.c
        (the file API both the loader and the menu call)
                     |                       |
                   FatFs                     |         emu/ffs/ff.c
                     |                       |
              disk_read / disk_write         |         emu/ffs/diskio.c
                |                  |         |
         tsd_sd_read        gcode_sd_read  fldrv_*
         (EXI: SD2SP2,      (drive iface:  (FlippyDrive: the drive
          SD Gecko)          GC Loader)     serves files itself)
           tsd.c               gcode.c       fldrv.c
```

`diskio.c` maps FatFs drive numbers to drivers: `0..2` are the EXI ports, `7` (`DEV_GCLDR`) is
the ODE. The volume strings come from `ffconf.h` `FF_VOLUME_STRS`. A FlippyDrive is the
exception to the whole stack: it answers the file API directly over its own protocol
(`fldrv.c`), is never a FatFs volume, and is deliberately absent from `FF_VOLUME_STRS` — see
`emu_dev_is_native()` in `flippy_emu.c`.

### Device selection

`device_prio[] = { "gcldr", "sdc", "sdb", "sda", "fldrv" }` in
[flippy_emu.c](../cubeboot/source/emu/flippy_emu.c).

The array leads with the ODE because the IPL patches index straight into it, but the loader
works from `EMU_DEFAULT_DEVICE_ORDER` — `"sdc, sdb, sda, gcldr, fldrv"` — so a console without
a drive-interface device only reaches the drive inquiry after everything else has been ruled
out. That one string is both the bootstrap's search order and the default for `device_order`.
config.ini also accepts hardware spellings (`sd2sp2`, `slot_a`, `slot_b`, `ode`, `gcloader`,
`flippy`, `flippydrive`) which resolve onto the same volumes — `ode` is a plain alias for
`gcldr`, the GC Loader. A FlippyDrive
is deliberately **not** an ODE spelling: it does not replace the drive (it rides the drive
ribbon beside it), it speaks its own protocol, and it answers to its own names.

There is one attachment point on the drive interface, so at most one of these devices is
installed — which one it is gets decided once: `drive_probe()` (`emu/drive_probe.c`) sends a
single OEM inquiry, caches the answer, and both `gcode.c` and `fldrv.c` compare against it.
The disc screen reads the same cached answer to refuse **Z** on an ODE; a FlippyDrive keeps
the screen, since the optical drive is still behind it (`fldrv_bypass_enter/exit` in
`fldrv.c` make the drive transparent for the read).

The bootstrap runs that order **twice**. The first pass takes the device that actually holds a
`/config.ini`; only if none does, a second pass settles for the first device that merely
mounts. That two-pass shape is the fix for a real failure: `load_settings()` runs once, against
whatever volume is mounted at that moment, so consulting only the first device that *mounted*
meant a `config.ini` on the other card was never opened. A console with an empty SD2SP2 and the
config on the ODE booted with default settings and no games, and moving the file to the other
card only moved the failure. Searching for the file removes the guesswork entirely.

Once the settings are parsed, `emu_apply_device_order()` walks the `device_order` list from
config.ini and mounts the first entry that works. There is no on/off switch for any device:
leaving a name out of the list is what keeps cubiboot off it. An empty or absent value means
the bootstrap's choice already followed the same default order, so nothing is redone.

The menu never probes: it mounts exactly `device_prio[emu_sd_device]`.

### The GC Loader protocol

`gcode.c` talks to the ODE over the DI (drive interface) registers at `0xCC006000`:

- **Detect** — `0x12000000` (OEM inquiry), 32-byte DMA response; it is a GC Loader if
  `rel_date` (offset 4) is `0x20196c64`.
- **Read** — `0xB2000000`, `CMDBUF1` = LBA in 512-byte sectors, `CMDBUF2` = byte count,
  DMA'd to `MAR`. Destination must be 32-byte aligned; unaligned callers bounce through a
  staging sector.
- **Write** — not wired up; the volume is read-only.

This is the same detection and the same command libogc2 uses in `DVD_LowGcodeRead`, which is
what Swiss's GC Loader device driver goes through. Practical consequence: **if Swiss lists the
ODE as a GC Loader, cubiboot will read it too.** FlippyDrive uses a different command set
(`0xB5` file API) and is not covered.

## Memory map

| Address | What |
|---|---|
| `0x80000000` | low memory / OS globals; the loader preserves `0xF4`, `0xC0`, `0xD4` across `SYS_ResetSystem` |
| `0x80001800` | stub magic — `PASS` (`0x50415353`) forces passthrough boot |
| `0x80100000` | `.data_lowmem` — banner/icon pools, the path lists, the 2 MB games heap and the boot-logo texture (`0x3b8740`, ~3.7 MB of the 4 MB `link.ld` asserts), `NOLOAD`; the pools are zeroed by `gm_init_heap()` |
| `0x81300000` | BS2 lands here; also the loader's arena ceiling (`__myArena1Hi`) and where stage 1 links |
| `0x81500000` | `ipl_metadata_t` sits just below this |
| `0x81600000` | `.data_empty` |
| `0x81601680` | `patches.elf` `.text` and the injected data |
| above `_patches_end` | the loader's framebuffer |

Sizes worth knowing: the IPL image is `0x200000`, BS2 code starts at `0x820` into it, and
`current_dol_buf` in the loader is 750 KB.

## Build pipeline

```
patches/        --make-->  patches.elf
                             |  (validate-patches.go, objcopy strips .comment)
                             v
cubeboot/       --make-->  cubeboot.dol      (patches.elf embedded as data)
                             |                 == released ipl.dol / flippydrive.dol
                             |  gzip
                             v
entry/          --make-->  entry.dol         (stub + cubeboot.gz, linked at 0x81300000)
                             |
        +--------------------+---------------------+---------------------+
        v                                          v                     v
  build_apploader.sh                         build_iso.sh        make_picoboot_uf2.py
  (swiss-gc packer +                   (genisoimage, El-Torito,  (BS2-scramble entry.dol,
   apploader header)                    gbi.hdr re-branded)       payload-only UF2 @0x80000)
        |                                          |                     |
        v                                          v                     v
  apploader.img                              cubiboot.iso    cubiboot_picoboot_payload.uf2
                                                   |
                                          make_picoloader_uf2.py
                                                   v
                                     cubiboot_picoloader_payload.uf2
```

`entry/Makefile` drives the whole chain, including the `cubeboot/source/emu` →
`patches/source/emu` copy. Because the `data/cubeboot.gz` target declares no dependencies,
**`make clean` is required** for any source change to take effect.

`apploader.img` must be rebuilt with the loader: it embeds this build's `cubeboot.elf`, so a
stale one sends In-Game Reset back to an old menu. The CI and both local wrappers always
rebuild them together.

### How each release artifact is produced

- **`apploader.img`** — `cubeboot.elf` packed with the
  [swiss-gc packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) (reboot
  variant) and wrapped in a GameCube-apploader header. See
  [.ci/build_apploader.sh](../.ci/build_apploader.sh).
- **`cubiboot.iso`** — a GameCube El-Torito ISO9660 image built with `genisoimage` from
  [cubeboot-tools'](https://github.com/makeo/cubeboot-tools) `gbi.hdr` (re-branded to the
  Cubiboot banner), with the loader `.dol` as the boot image. See
  [.ci/build_iso.sh](../.ci/build_iso.sh).
- **`cubiboot_picoloader_payload.uf2`** — the [PicoLoader](https://github.com/makeo/PicoLoader)
  firmware with `cubiboot.iso` embedded as the payload, replicating makeo's PicoLoader
  converter. See [.ci/make_picoloader_uf2.py](../.ci/make_picoloader_uf2.py).
- **`cubiboot_picoboot_payload.uf2`** — the [PicoBoot](https://github.com/webhdx/PicoBoot)
  payload-only update. The payload is `entry/entry.dol` (the stage-1 stub linked at
  `0x81300000` — the only DOL PicoBoot can inject), scrambled with the BS2 bootrom
  scrambler and stamped with PicoBoot's `IPLBOOT `/`PICO` payload framing, replicating
  PicoBoot's `tools/process_ipl.py`. It targets flash `0x80000`, where official PicoBoot
  firmware ≥ v0.4 streams the payload from — which is why the official firmware has to be
  flashed first. See [.ci/make_picoboot_uf2.py](../.ci/make_picoboot_uf2.py).

## Booting a game

The menu does not boot games itself. `chainload_swiss_game()` in
[loader.c](../cubeboot/source/emu/loader.c) loads `/swiss-gc.dol` and hands it an argv.
The load goes through `dvd_custom_open_flash()`: on a FlippyDrive that is the drive's own
internal flash first — which ships with Swiss, so the card needs no `swiss-gc.dol` — and the
card root only as fallback; on every other device it is `/cubiboot/swiss-gc.dol` then
`/swiss-gc.dol` on the card.

```
Autoload=<device>:/path/to/game.iso
AutoBoot=Yes
BS2Boot=No
Prefer Clean Boot=No
IGRType=Apploader          (only if swiss/patches/apploader.img exists)
IGRType=Reboot             (instead, when the device is a FlippyDrive -- the drive
                            autoloads cubiboot from flash on reboot, so no apploader.img)
```

plus `CUBEBOOT=1` in the environment. Swiss then does the real work — and, as a side effect,
records the launch in `/swiss/settings/recent.ini`, which is exactly what
`remember_last_game` reads back on the next boot.

Two exceptions bypass Swiss:

- A file whose basename starts with `swiss` is booted **directly through cubiboot's own
  apploader** (`is_swiss_image()`), because autoloading a Swiss disc through Swiss resets to
  the stock IPL.
- Passthrough boot (`start_passthrough_game`) uses `Autoload=dvd:/*.gcm`.
