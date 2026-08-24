# Fork changes over makeo/cubiboot

This fork = pristine **makeo/cubiboot `main`** + the `OffBroadway/cubeboot@custom-loader-menu`
banner-grid layout + a cold-boot banner-corruption fix + Cubiboot branding + a CI that
releases all artifacts + the menu/quality-of-life work in sections F–P. This document records
every change so it can be re-applied onto a fresh makeo clone.

For how the pieces fit together — the two-binary split, how the BIOS is patched, how settings
reach the menu — see [ARCHITECTURE.md](ARCHITECTURE.md).

| | Change | Main files |
|---|---|---|
| A | Banner-grid layout (cherry-picked from cubeboot) | `patches/source/grid.c`, `menu.c` |
| B | Cold-boot banner-corruption fix | `patches/source/games.c` |
| C | Scaling past the 128-buffer pool | `patches/source/games.c` |
| D | Cubiboot branding | `.ci/brand_*.py`, `patches/source/menu.c` |
| E | CI / release pipeline | `.github/workflows/ci.yml`, `.ci/` |
| F | Filename titles + multi-disc banners | `patches/source/games.c` |
| G | `default_folder` | `patches/source/main.c` |
| H | `remember_last_game` | `patches/source/games.c`, `main.c` |
| I | Booting a Swiss disc image natively | `cubeboot/source/emu/loader.c` |
| J | Storage selection + the ODE's SD card | `cubeboot/source/emu/gcode.c` + FatFs glue |
| K | Homebrew apps as banner entries | `patches/source/games.c` |
| L | Folder name in the header, rebranding, banner tool | `patches/source/menu.c`, `.ci/brand_*.py`, `tools/` |
| M | Boot delays (`preboot_delay_ms`, `postboot_delay_ms`) that actually fire | `patches/source/main.c` |
| N | Faster game lists: per-file sector buffer + a sector cache | `cubeboot/source/emu/ffs/` |
| O | Native FlippyDrive support | `cubeboot/source/emu/drive_probe.c`, `fldrv.c`, `flippy_emu.c` |
| P | `cube_logo`: custom boot logo, revived without a PNG decoder | `patches/source/games.c`, `tools/cube-logo-converter/` |
| Q | The disc screen reads a disc that arrives after it opens | `patches/source/menu.c`, `dolphin_dvd.c` |
| R | The stock bottom prompt bar, corrected and reused | `patches/source/prompt.c` |

## A. custom-loader-menu banner layout (cherry-picked)

The banner grid is the upstream `OffBroadway/cubeboot@custom-loader-menu` feature, applied
to makeo by cherry-picking exactly these commits (oldest → newest):

```
92d75b1  Add variable for number of columns in grid
0d6bb5e  Add `menu_grid_type` .ini setting
1b6d148  Add layout and cube scaling for a grid of banners
cb9889a  Update asset loading to handle many column counts
5a631df  Add an alternative 4x4 grid of small banners
831023e  Move `menu_grid_type_t` into its own file
2adc77d  Extrude the save cube's vertices
e918477  Reduce the rotation used for banner-width icons
```

Two trivial merge conflicts resolved: kept makeo's `__attribute_data_empty__` on
`browser_lines` while adding CLM's `columns_per_line` (`patches/source/grid.c`); combined
CLM's `menu_grid_type` `set_patch_value` line with makeo's commented block
(`cubeboot/source/main.c`).

## B. Cold-boot banner-corruption fix  (`patches/source/games.c`)

**Root cause:** the banner/icon buffer pools live in the custom `.data_lowmem` section
(`attr.h`: `__attribute_aligned_data_lowmem__` = `section(".data_lowmem")`), which
PicoBoot/gekko does **not** zero on cold boot. So `gm_banner_pool[].used` came up as random
garbage → `gm_get_banner_buf` either aliased two banners onto one buffer (corruption) or
found none free (blank), worse the colder the RAM. Only `.iso` (disc) banners were affected;
`.dol`/program entries set `use_banner=false` and never allocate a banner buffer.

Fix:
1. **Zero the pools at startup** (the actual root fix): `gm_init_heap()` does
   `memset(gm_banner_pool, 0, …)` + `memset(gm_icon_pool, 0, …)` before any asset loads.
2. **Keep banners resident in MRAM** (the async per-scroll ARAM unload/reload amplified the
   random-`used` bug into corruption): `gm_banner_setup`/`gm_banner_setup_unload` just set
   `state = GM_LOAD_STATE_LOADED` (no MRAM↔ARAM DMA); banners display straight from their
   resident buffers.

## C. Scalable past the 128-buffer pool  (`patches/source/games.c`)

`ASSET_BUFFER_COUNT = 128`. ≤128 banners/folder stay fully resident (the proven path,
unchanged). When a folder has **more** than 128, a sliding window engages:
- `gm_evict_on_scroll` flips true once the pool fills during `gm_check_files`.
- While true: `gm_line_free` releases off-screen lines and `gm_line_load` re-reads
  on-screen lines from disc via `gm_load_banner(entry, 0, false, /*use_cache=*/false)`.
- The `use_cache=false` re-reads **bypass makeo's `bnr_cache`** so scroll re-reads never
  touch ARAM (ARAM is the corruption path). `bnr_cache` is kept intact for the ≤128 path.
- `gm_load_banner` guards against re-loading an already-LOADED banner (no buffer leak), and
  `gm_line_changed` frees before it loads so the window always has a pool buffer.

## D. Branding

- **Menu header** = `"Games"` (`patches/source/menu.c`, `custom_gameselect_menu`).
- **Menu / loader banner** = "Cubiboot" / "Games Loader" with the cubeboot banner image —
  baked into `patches/data/default_opening.bin` via `.ci/brand_opening.py`.
- **`.iso` BIOS-intro banner** = the cubeboot banner + "Cubiboot" / "Games Loader" (replaces
  the stock gc-linux "Game Play" banner) — `.ci/brand_gbi.py` patches `gbi.hdr` at iso-build
  time (invoked from `.ci/build_iso.sh`), pulling the banner pixels from
  `default_opening.bin`.
- **`small_banners` is the in-code default** (`cubeboot/source/settings.c`
  `load_settings()`), so the layout works even without a `config.ini`; the `.ini` can still
  override `menu_grid_type`.

## E. CI / release  (`.github/workflows/ci.yml`, `.ci/`)

On push it builds in the reproducible `cubiboot-dev` Docker image (`.ci/Dockerfile`):
`ipl.dol`, `apploader.img` (`.ci/build_apploader.sh`), and `cubiboot.iso`
(`.ci/build_iso.sh`, branded). It then generates `config.ini`
(`[cubeboot]\n\nmenu_grid_type = small_banners`) and `EXTRACT_TO_ROOT.zip`
(= `ipl.dol` + `config.ini` + `swiss/patches/apploader.img`), and builds
`cubiboot_picoloader_payload.uf2` — PicoLoader firmware (`makeo.github.io/PicoLoader/fw/picoloader.uf2`)
with `cubiboot.iso` embedded at flash `0x10031000` for both RP2040/RP2350 family ids, via
`.ci/make_picoloader_uf2.py` (replicates makeo's PicoLoader converter). It also builds
`cubiboot_picoboot_payload.uf2` via `.ci/make_picoboot_uf2.py`: the payload is
`entry/entry.dol` (stage 1, linked at `0x81300000` — the only DOL PicoBoot can inject,
**not** the released `ipl.dol`, which is `cubeboot.dol` at `0x80003100`), flattened,
scrambled with the BS2 bootrom scrambler (keystream offset `0x720`) and framed with
PicoBoot's `IPLBOOT `+size header and trailing `PICO` tag — replicating PicoBoot's
`tools/process_ipl.py`. The `.uf2` targets flash `0x10080000` with interleaved
RP2040/RP2350 family blocks — the region official PicoBoot firmware ≥ v0.4 DMA-streams
the payload from (1.5 MiB limit; ≤ v0.3.x has no separate payload region and silently
ignores the file). So the install is: flash webhdx's `picoboot_full_*.uf2` once, then
this on top — it replaces the stock gekkoboot payload in place, firmware untouched.
Cubiboot no longer ships full firmware+payload images (`_pico`/`_pico2.uf2`, dropped
in v1.9.0). On a `v*` tag it publishes a GitHub Release with all the artifacts.

The body comes from `.ci/release-notes.md`, written per release. It **opens** with a standing
"Updating from an earlier release?" block that every release repeats verbatim, quoted with a
`>>` prefix so it reads as a permanent notice rather than as one of this version's changes:
`apploader.img` holds a second copy of the loader, so replacing only the loader leaves In-Game
Reset on the previous version silently. First, not last — it is the one item in the notes that
costs a user a broken update if they miss it. Keep it when rewriting the notes; the marked
comment in the file says as much, so it survives whoever writes the next one.

## F. Filename titles + multi-disc banners  (`patches/source/games.c`)

Upstream showed the **internal game name** from the disc header, so every disc of a
multi-disc set displayed the same title and the list gave no way to tell them apart.

`gm_set_title_from_path()` now writes `entry->desc.fullGameName` from the **filename** with
its extension stripped — the same thing Swiss shows. `Resident Evil 0 Disc 2.iso` reads as
`Resident Evil 0 Disc 2`.

There is no in-code truncation: a title longer than the box (~28 chars) is clipped at draw
time, which is why the README asks for short filenames.

Banner lookup is keyed on `(game_id, disc_num, disc_ver)` rather than `game_id` alone
(`bnr_cache_get`/`bnr_cache_put`), so disc 2 gets disc 2's banner instead of disc 1's.
`gm_check_files` pairs entries that share a game id but differ in `disc_num`.

## G. `default_folder`  (`patches/source/main.c`)

The `default_folder` option by [wins1ey](https://github.com/wins1ey), via the
[Hazado/cubiboot](https://github.com/Hazado/cubiboot) fork
([merge](https://github.com/Hazado/cubiboot/commit/c91066b4889346fec288393f6a9fe41304652e49)),
ported onto this tree.

`resolve_default_folder()` adds a leading `/` when the value omits one, verifies the folder
opens, and falls back to `/` when it doesn't. The string reaches the patch as a `strcpy`
into the `default_folder` buffer (not a `set_patch_value` word) — see
[ARCHITECTURE.md](ARCHITECTURE.md#the-loader-to-patches-contract).

**Where it runs matters.** It used to run from `pre_thread_init()`, and that silently broke
both this option and `remember_last_game` on the memory card slot readers: that hook fires
so early that on EXI channels 0/1 the BIOS still owns the channel, the driver's
`EXILock`/`EXISelect` fail immediately (NULL callback — no waiting), the very first mount
fails, and the menu fell back to the root — always, while the same config worked on SD2SP2
(channel 2, which the BIOS never touches). The card itself was fine: the enum thread's later
mount succeeded, which is why the root listing still appeared and the folder could be opened
by hand. The fix moves the decision, not the logic: `pre_thread_init()` passes `NULL` to
`gm_start_thread()`, and `gm_thread_worker()` resolves last-played → `default_folder` as its
first act, on the thread whose first SD access is the one that demonstrably works. Same
reads, same order, no retries or delays added anywhere.

## H. `remember_last_game`  (`patches/source/games.c`, `main.c`)

Opens the menu in the folder of the last game booted, with that game highlighted.

**No new state is written.** cubiboot boots games by chainloading Swiss, and Swiss already
records every launch in `/swiss/settings/recent.ini`. `gm_read_last_played()` reads the first
`Recent_*=` line that resolves to a game, strips the device prefix, and returns the path. Only
the first 512 bytes are read — `Recent_0` is line 2, always well inside the first sector.

Consequence worth knowing: **Swiss's Recent List must be On** (`RecentListLevel=On` in
`/swiss/settings/global.ini`), otherwise there is no file to read and the menu falls back to
`default_folder`.

`gm_last_played_folder()` strips the filename to get the containing folder — so a letter or
genre subfolder is honoured, not just `default_folder` — and verifies it still opens. Like
`resolve_default_folder()`, it is called from `gm_thread_worker()`, not `pre_thread_init()`
— calling it that early breaks the slot A/B card readers (see section G).

The cold-boot path is deliberately not banner-blocking. `gm_arm_last_played()` marks the scan
so `gm_check_files` skips the resident banner preload; the folder is scanned by headers only,
the cursor is placed via `gm_match_last_played()`, and `gm_bg_load_last_played()` fills
banners on a background thread in priority order — the on-screen window around the highlighted
game first, then the rest. **A** works while that is still running.

`remember_last_game` overrides `default_folder`; the latter is only the fallback (first boot,
or the folder is gone).

## I. Booting a Swiss disc image natively  (`cubeboot/source/emu/loader.c`)

Handing a Swiss `.iso`/`.gcm` to Swiss via `Autoload=` is Swiss-in-Swiss, and it just resets
to the stock IPL.

`is_swiss_image()` matches any file whose **basename starts with `swiss`**, regardless of
extension, and routes it through cubiboot's own apploader (`load_dol_file` + `run`) instead of
the autoload path. `is_swiss()` covers the chainloader itself, and matches an app by its **folder** name when
the file is `default.dol` -- an app's filename is always that, so the folder is the only
thing identifying it. `emu_can_boot` lets Swiss through even with no `swiss-gc.dol` at the
root: everything else is booted by chainloading Swiss, but Swiss needs no chainloader, and a
card carrying it only as an app would otherwise refuse to launch the one thing that fixes
that.

Normal games don't start with `swiss`, so nothing else is affected. This is separate from the
`swiss-gc.dol` engine that has to sit at the card root.

The disc-image half of this only bites when `force_swiss_default = 1`: with the default of 0
every image already takes the native apploader, so a Swiss `.iso` boots under any name. The
`.dol` and app forms have no such escape -- they are always routed through Swiss -- so there
the name is what decides, always.

## J. Storage selection and the ODE's SD card  (`device_order`)

Lets cubiboot read games straight off the SD card inside a GC Loader style ODE, so no EXI
card reader is needed. The driver was reverse-engineered from a `cubiboot-gcldr.iso` build
and cross-checked against libogc2's `DVD_LowGcodeRead`; the resulting `gcode_read_aligned`
compiles instruction-for-instruction identical to the reference build's.

- **`cubeboot/source/emu/gcode.c` + `.h`** (new) — block driver over the drive interface.
  Detection is the OEM inquiry `0x12000000` checking `rel_date == 0x20196c64`; reads are
  `0xB2000000` with `CMDBUF1` = LBA and `CMDBUF2` = byte count. Writes are stubbed. Lives
  under `emu/` so it compiles into both the loader and the injected menu.
- **`emu/ffs/diskio.c`** — `DEV_GCLDR` (pdrv 7) wired into `disk_initialize`/`read`/`write`/
  `status`/`ioctl`/`shutdown`.
- **`emu/flippy_emu.c`** — `device_prio[]` gains `"gcldr"`, and `EMU_DEFAULT_DEVICE_ORDER`
  (`"sdc, sdb, sda, gcldr"`) becomes the single order the loader works from.
  `emu_apply_device_order()` walks the list from `config.ini` and mounts the first entry that
  works.
- **`cubeboot/source/settings.{c,h}` + `main.c`** — `device_order`, kept as a raw string so
  the device list stays owned by `flippy_emu.c`.
- **`.github/workflows/ci.yml`** — the generated `config.ini` ships the key commented, so it
  is discoverable without changing behaviour.

Two deliberate departures from the recovered implementation: DI transfers are bounded by a
timeout instead of spinning on `TSTART` forever, and the probe stops as soon as a real optical
drive answers — or as soon as nothing answers at all — rather than burning 40 retries.

### Finding `config.ini`

The first cut of this shipped a `load_from_ode_sd` boolean, and it did not survive contact
with hardware. `load_settings()` runs once, against whatever volume happens to be mounted, and
the bootstrap took the first device that *mounted* — so on a console with both an SD2SP2 and a
GC Loader, the card reader always won and a `config.ini` on the ODE was never opened. A tester
with the config on the ODE booted to default settings and no games; it only worked once the
SD2SP2 was physically removed. Moving the file to the other card just moved the failure, and
inverting the probe order only swapped which user it hit.

The fix is to search: `flippy_emu_mount()` runs the default order twice, first taking the
device that actually carries a `/config.ini`, then falling back to the first that merely
mounts. `device_order` cannot help here — it lives inside the file being looked for.

`load_from_ode_sd` was then dropped rather than kept as an alias. It never appeared in a
tagged release, so there was nothing to preserve, and a list expresses everything the boolean
did plus what it could not: excluding a card reader, reordering slots, or naming a device that
does not exist yet.

Scope: this is the **GC Loader protocol**, not "any ODE". If Swiss lists the drive as a GC
Loader, cubiboot reads it too. FlippyDrive uses a different command set and is not covered
by this driver — section O adds it natively.

## K. Homebrew apps as banner entries  (`patches/source/games.c`)

A folder holding `default.dol` next to `opening.bnr` is listed as a launchable application
with its own banner rather than as a folder to enter. Both filenames are fixed, which is what
keeps the detection cheap.

- **`GM_FILE_TYPE_APP`** — a new entry type. It needs no menu work: an app is neither
  `PROGRAM` nor `DIRECTORY` and carries no icon, so it already falls through to the same
  drawing path a game uses, and `boot_entry.path` already points at the `.dol`, so booting is
  the plain homebrew path.
- **Detection** happens in `gm_check_files`, not in the readdir loop, so the cost sits where
  entries are already being opened one by one. `opening.bnr` is probed first because it is the
  rarer of the two: a folder without one stops after a single failed open, which is every
  folder in a normal game library.
- **`standalone_bnr`** in `gm_extra_t`. `gm_load_banner` used `dvd_bnr_offset == 0` to mean
  "no banner", and 0 is exactly where a standalone `opening.bnr` starts, so the sentinel had
  to move to a flag.
- **BNR1 vs BNR2.** The `BNR` struct is BNR2-shaped (six `BNRDesc`, `0x1FA0`); a BNR1 file
  stops after one (`0x1960`). Reading the larger size off the smaller file left the tail
  holding the previous banner's bytes, so the read clamps to the file's real size and clears
  the buffer first. `desc[0]` and the pixel data sit inside both formats, and the magic now
  fills `dvd_bnr_type`, which had been a TODO.
- **Title.** Games override the banner's own name with the filename so multi-disc sets are
  distinguishable. An app is always `default.dol` and has no sibling disc, so it keeps the
  banner's name.

The banner pool is unaffected in practice: it is released whole on every folder change
(`gm_start_thread`), so an apps folder and a games folder never compete for the 128 buffers.
An apps folder past 128 entries would degrade to the same sliding window games use.

## L. Header, branding and the banner tool

- **Menu header** names the folder being browsed (`patches/source/menu.c`,
  `custom_gameselect_menu`) instead of the fixed `"Games"`. The card root has no folder name
  to show, so it carries the product name instead.
- **Banner artwork and text** are the Cubiboot wordmark with `"GC Games and Apps Loader"` /
  `"build <version>"` under it, where the version is whatever `brand_opening.py` stamped: the
  tag name on a release build, the short SHA otherwise. Both the menu cube and the `.iso` BIOS
  intro read from
  `patches/data/default_opening.bin` -- `brand_gbi.py` copies its pixel data into `gbi.hdr`
  at iso-build time -- so replacing that one file covers both. The artwork is fitted to
  width, not stretched; the slot is 96x32, so a source's aspect ratio decides how many of
  the 32 rows it fills.
- **`tools/banner-converter/`** vendors the converter that produces an `opening.bnr` from
  any image, since an app cannot be listed with a banner without one. Canonical copy is
  [banner-converter-gc](https://github.com/DarthMotzkus/banner-converter-gc); take fixes
  upstream first.

Note that the version string in the banner is baked into `default_opening.bin`, so a release
that forgets to re-run `brand_opening.py` will show the previous version on the console.

## M. Boot delays that actually fire  (`patches/source/main.c`)

`preboot_delay_ms` and `postboot_delay_ms` are upstream cubeboot options. Both parse correctly
here and both reach the patch as words, so the plumbing was never the problem — neither one
produced the delay you asked for.

**`preboot_delay_ms` was ~3.6x short.** `pre_menu_init()` converted milliseconds to fields as
`preboot_delay_ms / fps`, the reciprocal of the real conversion (`ms * fps / 1000`). `15000`
came out as 250 fields ≈ 4.2 s on NTSC, 300 fields = 6 s on PAL, instead of 15 s. Now
`((u64)preboot_delay_ms * fields_per_sec) / 1000`. The wait deliberately still runs on
`VIWaitForRetrace()` rather than the time base: keeping VI going through the wait is the whole
point of the option, since a TV cannot lock onto a signal that isn't there.

**`postboot_delay_ms` never fired at all.** It was handled in `bs2tick()`, gated on
`start_passthrough_game` and measured from `completed_time`. Two faults, either one fatal:

- `start_passthrough_game` is set only by the Z-trigger DVD passthrough path (`menu.c`).
  Booting from the grid sets `*bs2start_ready` and nothing else, so the branch was
  unreachable on the path essentially everyone uses.
- `completed_time` is stamped when the *boot logo* animation finishes at cold boot. By the
  time a game is picked out of the menu, minutes may have passed, so
  `elapsed > postboot_delay_ms` was already true and it returned `STATE_START_GAME` at once.
  Upstream this was sound — there, the game boots straight after the logo, so "since the
  animation ended" and "since the game was chosen" are the same instant. The menu is what
  broke the assumption.

The wait now sits at the top of `bs2start()`, the single point every boot path converges on
(grid game, program/app, DVD passthrough), which runs after the exit animation and ahead of
the device and audio teardown. The IPL's frame loop has already exited by then, so the last
frame it drew stays on screen for the whole wait — the effect the option exists to produce.
`bs2tick()` therefore no longer returns `STATE_WAIT_LOAD` on any path.

## N. Faster game lists  (`cubeboot/source/emu/ffs/`)

Two changes under FatFs, both aimed at the same thing: a folder of games took long enough
to populate that you could watch it fill in, and leaving the folder and coming back paid the
whole cost again.

**A private sector buffer per open file** (`ffconf.h`: `FF_FS_TINY` 1 -> 0). Under the tiny
configuration a file has no buffer of its own and its data moves through the single shared
buffer inside the filesystem object -- the same one holding directory and FAT sectors. That
is precisely the wrong arrangement for this workload, which alternates between walking a
directory and reading a file: every read discarded the directory sector just walked to, so
the next lookup fetched it from the card again. It costs `FF_MAX_SS` on the one `FIL` (96 ->
4192 bytes) and the image gets *smaller*, because the tiny configuration carries extra code
to shuffle that shared buffer around: 695,840 -> 694,528.

**A sector cache** (`ffs/sector_cache.{c,h}`), sitting between FatFs and the drivers with
`disk_read`/`disk_write` routed through it. Nothing below FatFs cached anything and the SD
paths issue one command per 512-byte sector, while FatFs resolves a path by walking its
directory from the start on every open -- so listing N games re-walked the same sectors N
times.

It is **direct-mapped** rather than LRU, which is the only real design decision in it. A
page's slot is its page number modulo the slot count. For a directory swept repeatedly from
the start -- the one pattern that matters -- that gives a full hit rate with no bookkeeping at
all once the working set fits, and an O(1) lookup instead of a list walked on every sector
access. LRU has nothing to offer the pattern: when the set fits nothing is evicted either
way, and when it does not, recency evicts exactly the page wanted next. Sizing is therefore a
cliff and not a dial, and the comment in the file carries the arithmetic.

384 KB of pages, sized to cover `games.c`'s own 1920-entry listing cap (~300 KB of directory
at typical ISO name lengths), leaving ~350 KB of the region free. The pages live in
`.data_lowmem`, which is NOLOAD -- reserved RAM that costs nothing in the image. In a loaded
section this would have added 384 KB to `ipl.dol`; as it is, the whole thing costs ~5 KB of
code. Whole-page-and-larger transfers bypass it: those are files being streamed and read
once, so caching them would evict what the cache exists to keep.

**The dangerous part** is that those pages sit in RAM the rest of the firmware treats as
scratch, so two callers must disable the cache before clearing it, and both do:

- `bs2start()` clears `0x80100000-0x81600000` before every boot, which is where the pages
  are. Left enabled, the bookkeeping would keep reporting them valid and every read after
  that point would return zeros -- and the files read next are the game and the apploader, so
  nothing would boot on any device.
- `load_dol()` clears an incoming DOL's BSS at whatever address the DOL declares, then reads
  its text and data sections. That is a partial overwrite at an address only the DOL knows,
  so unlike the wipe above it can destroy pages while leaving the cache's magic intact.

The magic kept beside the pages catches a whole-region wipe as a backstop, and covers first
use where the region is NOLOAD and holds whatever was there before. It cannot catch a partial
overwrite, hence the explicit disables. `disable()` is one-way with no `enable()` to match:
both callers are on the way out of the menu into a game.

**Confirmed on hardware by the maintainer**, and the test that showed it matters: hold A to
skip the boot animation. With the animation playing, the loading finishes behind it and both
builds look identical -- that free time is what had been hiding the cost all along. Skipped,
v1.6.2 still fills banners in while this build has them ready, and folder navigation is
quicker. No banner corruption across cold boots.

Still outstanding: re-entering a folder rebuilds its list from scratch -- every game reopened
and its header re-read. The banners now come largely from memory rather than the card, which
is the part that got faster, but the list itself is rebuilt. Remembering visited folders is
the fix, and a separate piece of work.
## O. Native FlippyDrive support  (`cubeboot/source/emu/`)

cubeboot was written against the FlippyDrive's own file protocol; this fork commented
that path out of both `flippy_sync.c` copies and put FatFs emulation in its place, which
left `device_prio` with no entry a FlippyDrive could answer to. So the loader ran on one
and found nothing.

**Identification is shared, not per device.** There is one drive connector, so a GC Loader
and a FlippyDrive can never both be installed -- which makes "what is on the bus" a single
question with a single answer. `drive_probe()` asks it once with an OEM inquiry, caches the
result, and both backends read it; `gcode_sd_init()` is now just a comparison against that.
Two independent probes would have meant a second retry loop and a second bounded timeout on
a console with no drive at all, the case that runs on every boot and can only ever fail.
The DI registers and the bounded wait moved there too, since three call sites drive them.

Detection is read-only, which is sufficient rather than merely careful. Swiss's
`flippy_init()` requires `rel_date == 0x20220426` from a plain inquiry before its file API
works, and its `flippy_bypass(false)` sends nothing when the drive already reports
`0x2022042x`. Nothing is written to a drive we have only just met, so a console with a real
optical drive or a GC Loader is untouched by the new code path.

**The device is not a volume.** `fldrv` serves paths itself, so it never reaches `f_mount`,
`f_open` or a `"<vol>:"` prefix. It is also deliberately absent from `FF_VOLUME_STRS`, so
FatFs would reject `f_mount("fldrv:")` outright -- a second line of defence under
`emu_dev_is_native()`. Every `dvd_custom_*` entry point in `flippy_emu.c` forks on which
side owns the current device.

The name is Swiss's, not a choice: `emu/loader.c` builds `Autoload=<dev>:<path>` and hands
that string to Swiss, so an invented spelling would need translating there. `flippy` and
`flippydrive` are config spellings for it. `ode` is a static alias for `gcldr` and covers
the GC Loader and the CUBE-ODE, which speak the same commands; a FlippyDrive is deliberately
not under it, because it is not an ODE -- it rides the drive ribbon beside the optical drive
instead of replacing it, which is also why it keeps the disc screen (Z) that a real ODE
refuses, and why cubiboot brackets that screen's disc read with the drive's bypass
entry/exit commands (`fldrv_bypass_enter/exit`).

Three more consequences of the drive not being an ODE, all deliberate:

- **Swiss comes from the drive's flash first.** `dvd_custom_open_flash()` asks the drive's
  internal flash -- which ships with Swiss -- before falling back to the card root, so a
  `swiss-gc.dol` on the FlippyDrive's SD card is a precaution, not a requirement.
- **IGR is `IGRType=Reboot`, not `Apploader`.** The drive autoloads cubiboot from flash on
  every reboot, so a plain reboot already returns to the menu; `apploader.img` is never
  read on this hardware and does not work there. `chainload_swiss_game()` picks the type
  from the active device.
- **The menu header names it `FLIPPY SD`** instead of lumping it under `ODE SD`.

**What the protocol actually needed.** Almost none of it was new. The file read is the
ordinary drive read command with the handle in bits 16-23 -- the same transfer passthrough
already used for a real disc -- so `dvd_read()` only gained the native case beside
`passthrough`. The remaining commands were the dormant ones in `flippy_sync.c`, rewritten as
one transfer helper plus thin wrappers: written out per command, as they had been, the
differences between them stop being visible, which is how a whole file API gets commented
out as a block without anyone noticing the read was the odd one out. The helper bounds its
wait, unlike the unbounded `TSTART` spins this protocol is usually written with -- this runs
as the IPL, where a drive that stops answering hangs a console with no menu to return to.

**Inherited handles.** `fldrv_init()` releases handles 1..31 and the flash handle before
anything else. The bootloader loads our DOL out of the drive's flash and leaves that handle
open -- which is why updating cubiboot on one of these requires holding X to reach the
bootloader menu, so it hands over without claiming the file and the flash copy can be
overwritten. Arriving normally, we inherit it. Swiss guards identically, with
`flippy_closefrom(1)` at the end of `flippy_init()`.

Two places the emulation's habits would have been wrong on real hardware: `dvd_custom_open()`
skips its "close fd 1 first" step, which exists only because the emulation keeps one `FIL`
and hands the fiction of fd 1 to every caller -- a real drive gives out real handles and
several can be open at once. And `dvd_custom_status()` returns the drive's answer without the
byte swap, since that swap exists to imitate the format the drive already sends.

**Confirmed on hardware by the maintainer**: this build runs on a real FlippyDrive and works
normally. A console without one cannot reach any of it -- the probe has to positively
identify a FlippyDrive first, and everything else falls through to the card readers as
before.

## P. `cube_logo`: custom boot logo  (`patches/source/games.c`, `tools/cube-logo-converter/`)

The original cubeboot had a `cube_logo` option that replaced the "GAMECUBE" text under the
cube in the boot animation. It has been dead since upstream's
[873893b](https://github.com/OffBroadway/cubeboot/commit/873893b) ("slide back on patch and
png changes") reverted the PNG work: the loader kept parsing the key, but the patch-side
loader was left inside `#if 0` calling into `upng`, and both PNG libraries (`upng`, `ok_png`)
were deleted from the tree. Nothing on the console said so — the key simply did nothing.

Revived without reintroducing a decoder. The console reads **raw linear RGBA8, exactly
352x40 (56320 bytes)** and does only the GX tiling, through the `Metaphrasis` converter
already linked in for banners. Keeping the decode off the console is what keeps the injected
blob small: `IPL.dol` grows by 448 bytes in total, because the 56 KB texture buffer lands in
`.data_lowmem` (`NOLOAD`) and is fully overwritten by the conversion before use.

`gm_load_cube_logo()` runs as the first act of `gm_thread_worker()`, **not** from a boot hook
— the same constraint as section G: at `pre_thread_init()` the BIOS still owns EXI channels
0/1, so a slot A/B reader's first access fails. It is the enum thread's first read, small, and
lands well before the animation draws the text. Wrong size or missing file falls back to the
stock text, so a bad conversion cannot break the boot.

**Alignment is not the obvious thing.** The stock texture does not fill its canvas: the
letters occupy a 316x33 box at +6,+3 and the trademark sign takes x 324..345, so the word
sits left of the texture centre and the model position compensates. Art centred on the full
352x40 canvas therefore renders visibly shifted right on a TV. The geometry was measured from
the IPL's own texture (descramble the dump, decompress the Yay0 archive at `0xb5260` of
NTSC 1.1, decode the I8 tiles), and both converters default to that box.

`tools/cube-logo-converter/` ships the conversion: `index.html` is a single-file browser page
(vanilla JS, Lanczos-3 resample on premultiplied alpha, live preview, geometry guides, no
build step and nothing uploaded) and `png2cubelogo.py` is the Pillow CLI with the same
geometry constants. Verified equivalent: same placement, and the few byte differences are
edge pixels where the page's premultiplied path is the more correct of the two.

**Confirmed on hardware by the maintainer**: a custom logo loads and draws correctly on a
NTSC-J IPL 1.1 console with an SD2SP2, alignment fix included.

## Q. The disc screen reads a disc that arrives after it opens  (`patches/source/menu.c`, `dolphin_dvd.c`)

The disc screen drives the drive itself (see the `disc_banner_*` state machine), and its read
used to be one-shot: pressing **Z** started it, and whatever it concluded stood until the user
left and came back. Putting a disc in while the screen was up therefore did nothing -- the
screen kept saying "Please insert a NINTENDO GAMECUBE DISC" with the disc already in the tray.

Three things were wrong, and they compounded:

- **Nothing watched the lid.** The cover register was sampled only on the browser side of
  `handle_gameselect_inputs`, which is unreachable in disc mode, and the two variables that
  tracked it were written and never read. `stock_disc_tick()` now samples the cover between
  reads and starts a fresh read on the lid closing.
- **The cover register cannot be trusted through a read.** `disc_banner_start()` resets the
  drive interface, and a drive that has just been reset reports its cover as OPEN until it
  re-asserts the line. Sampling through the read saw an open that never happened and re-read
  on the next closed frame, forever -- and each pass reset the drive before it could finish
  spinning up, so the disc was never recognised. Hence: sampled only between reads, and an
  open only counts after it has held for a few frames.
- **An error from the drive was treated as an answer.** A 40-error cap put a ~1.4s ceiling on
  the wait, which a tray that was just closed never makes; a drive still spinning up fails
  every command it is given. Only the overall timeout ends the read now, and the disc-ID
  attempts are paced rather than issued every other frame -- hammering the drive through its
  own spin-up was itself keeping it from finishing.

Also here: **START** only boots a disc the screen has actually read (the stock screen ignores
it on the other two states, and the passthrough would otherwise hand Swiss a drive with no
readable disc), and **B** always leaves -- `disc_banner_cancel()` waits out the transfer in
flight on the shared bounded spin and hands the drive back, where before B was refused for the
whole of a read.

## R. The stock bottom prompt bar, corrected and reused  (`patches/source/prompt.c`)

The row of button prompts along the bottom of every BIOS screen is IPL data: one `GLH0` group
per language, holding an element table (the pills, the button glyphs, the animated analog
stick, the "..." separators) and a label table. The group header describes all three tables, so
one pointer reaches everything -- and it is found by shape rather than by seven addresses,
since nothing else in the image is a `GLH0` whose element table starts with `bac1` and whose
label table starts with `txt1`.

What the fork does with it:

- drops the `...` between every glyph and its label, and closes the labels up over the gap;
- renames `Cancel` to `Back`. The strings the screens draw do not come from the group's own
  pool -- that is only a default -- so every standalone `Cancel` in the image is rewritten,
  terminator required so `Cancelar` on a PAL image is left alone;
- puts **A before B** on every screen. The bar carries two A/B pairs, one drawn by the BIOS
  cube screens and one by the sub-screens and Cubiboot's own; both are swapped, each within
  its own span, so the rows stay where the IPL centred them;
- gives the game list the analog stick and the A it never had, drawn from these same elements
  at the positions the BIOS sub-screens use for theirs.

Two things are read out of BS2 rather than reinvented, because inventing them is visible: the
pulse is a triangle between 100 and 255 over 256 ticks of the IPL's own bar counter (so the A,
the Z pill and the stock B breathe in step), and the stick walks eight states at 128 ticks
each, **cross-fading the centre against one tilt** -- holding the centre at full alpha and
swapping tilts under it blinks. The A's green is read from the colour the current screen tints
its prompts with, at offset 420 in that struct on every revision but NTSC 1.0, which keeps it
at 364.

Placement is per revision where it has to be: the vertical trim carries between NTSC and PAL,
the horizontal does not, and the A carries a trim of its own. Every one of these is a named
constant, and `prompt_region` is 1 or 2 rather than 0 or 1 -- cubeboot's relocation walk reads
a zero-valued symbol as a broken reloc and halts the boot.

## Re-applying onto a fresh makeo clone

1. `git clone https://github.com/makeo/cubiboot && cd cubiboot`
2. Add the cubeboot remote, fetch, cherry-pick the 8 commits in section A (resolve the 2
   conflicts as noted).
3. Apply the `games.c` fix (B) + scalable (C), the branding (D), and the `.ci/` + workflow (E).
4. Apply the menu work: titles + multi-disc (F), `default_folder` (G), `remember_last_game`
   (H) — all in `patches/source/games.c` and `main.c`.
5. Apply the shared-`emu/` changes: Swiss image boot (I) in `loader.c`, and the ODE SD driver
   (J) — `gcode.{c,h}`, `diskio.c`, `flippy_emu.c`, plus the `settings.c`/`main.c` wiring.
6. Apply the app entries (K): `games.h` type + flag, `games.c` detection and banner load,
   plus the `GM_FILE_TYPE_APP` cases in `patches/source/main.c` and `emu/tweaks.c`.
7. Apply the boot-delay fixes (M) in `patches/source/main.c`: the conversion in
   `pre_menu_init()` and the wait moved out of `bs2tick()` into `bs2start()`.
8. Apply the list-speed work (N): `FF_FS_TINY` 0 in `emu/ffs/ffconf.h`,
   `emu/ffs/sector_cache.{c,h}`, the `disk_read`/`disk_write` routing in `emu/ffs/diskio.c`,
   and the `sector_cache_disable()` calls in `patches/source/main.c` (`bs2start`) and
   `patches/source/boot.c` (`load_dol`).
9. Apply the FlippyDrive work (O): `emu/drive_probe.{c,h}` and `emu/fldrv.{c,h}`, the
   `device_prio`/alias/dispatch changes in `emu/flippy_emu.c`, the include guard on both
   `flippy_sync.h` copies, and the `flippydrive.dol` artifact in the workflow.
10. Apply the custom boot logo (P): `gm_load_cube_logo()` in `patches/source/games.c`, the
   un-`static`'d `cube_text_tex` in `patches/source/main.c`, the `cube_logo_path` `strcpy` in
   `cubeboot/source/main.c`, and `tools/cube-logo-converter/`.
11. Run `brand_opening.py` once on `patches/data/default_opening.bin` (force-add it; it is
   `*.bin`-gitignored).
12. Add `CLAUDE.md` and `docs/ARCHITECTURE.md`; drop the upstream `docs/SD_Boot*.md` and
   `docs/RP2040_Boot*.md` guides, which describe cubeboot's filenames and options, not this
   fork's.
