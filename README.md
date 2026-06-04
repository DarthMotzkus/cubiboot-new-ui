# cubiboot-new-ui

A fork of [makeo/cubiboot](https://github.com/makeo/cubiboot) — itself a fork of
[cubeboot](https://github.com/OffBroadway/cubeboot) by
[TeamOffBroadway](https://github.com/OffBroadway) — with support for SD2SP2, SD Gecko
or similar SD adapters.

<img width="320" height="240" alt="ezgif-80bc8dbf18a70e39" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

**What this fork adds on top of makeo/cubiboot:**
- A **grid / banner menu UI** ported from cubeboot (selectable via `menu_grid_type`,
  default **`small_banners`** — works even without a `config.ini`).
- A fix for **cold-boot banner corruption**: the banner/icon buffer pools live in a
  low-memory section that PicoBoot doesn't clear on cold boot, so stale "in-use" flags
  aliased buffers (corruption) or starved them (blank), worse the colder the console.
  The pools are now zeroed at startup and banners are kept resident in MRAM. Folders with
  more banners than the pool use a sliding window (re-read from disc on scroll, no ARAM).
- **Cubiboot branding**: a "Games" menu header, and the cubeboot banner with
  "Cubiboot" / "Games Loader" on both the loader banner and the `.iso` BIOS intro
  (replacing the gc-linux "Game Play" banner).
- An automated **build & release pipeline** that also rebuilds `apploader.img` (so
  In-Game Reset returns to *this* loader instead of a stale one) and a flashable
  **`cubiboot_picoloader.uf2`**.

> [!IMPORTANT]
> Format your SD card using **exFAT** (not FAT32). Loading files is very slow on FAT32.

## What's in a release

Each tagged release (`v*`) publishes:

| File | What it is |
|------|------------|
| `cubiboot_picoloader.uf2` | **PicoLoader firmware with cubiboot embedded** — flash it straight to the RP2040 Pico; no separate loader file needed on the SD card. |
| `ipl.dol` | The cubiboot loader (a GameCube IPL replacement). Boot it via PicoBoot/PicoLoader + gekkoboot. |
| `apploader.img` | The Swiss **In-Game-Reset** redirect. It embeds *this build's* loader, so pressing the reset combo in a game returns to this cubiboot menu. Goes in `SD:/swiss/patches/`. |
| `config.ini` | Minimal example config (`menu_grid_type = small_banners`). Goes in the SD card root. |
| `cubiboot.iso` | Bootable GameCube disc image for **GC Loader** (and other ODEs), branded with the Cubiboot banner. |
| `EXTRACT_TO_ROOT.zip` | Everything that lives on the SD card (`ipl.dol`, `config.ini`, `swiss/patches/apploader.img`) — just extract it to the root of the card. |

## Installation — [PicoLoader](https://github.com/makeo/PicoLoader) (flash the Pico, easiest)
1. Download [`cubiboot_picoloader.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoloader.uf2).
2. Hold down the button on the RP2040 Pico while plugging it into your PC.
3. Copy the `.uf2` to the USB drive that appears; the Pico reboots running cubiboot.
4. Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest) `.dol`, rename it to `swiss-gc.dol`, and place it on your SD2SP2 / SD Gecko card (along with your games and a `config.ini`).

## Installation — [PicoBoot](https://github.com/webhdx/PicoBoot) / [PicoLoader](https://github.com/makeo/PicoLoader) with gekkoboot payload
1. Download [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol) and copy it to the **root** of your SD card.
2. Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest) `.dol`, rename it to `swiss-gc.dol`, and place it on the SD card.
> This .dol does **not** run in Dolphin even with IPL.bin set!

## Installation — [GC Loader](https://gcloaderhq.com/) (and other ODEs)
The `cubiboot.iso` is a bootable GameCube disc image that simply *is* the cubiboot
loader (branded with the Cubiboot banner) — no PicoBoot/modchip needed.
1. Download [`cubiboot.iso`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot.iso) and copy it onto your GC Loader's storage (SD/USB), in the folder you boot images from.
2. Boot `cubiboot.iso` from the GC Loader menu — it lands on the cubiboot menu.
3. cubiboot still lists/loads games from your **SD card adapter** (SD2SP2 / SD Gecko),
   so set that card up as usual: copy your games and a `config.ini` to it (see
   [Configuration](#configuration)). Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest)
   `.dol`, rename it to `swiss-gc.dol`, and place it on the SD card.

> The `.iso` is for booting the loader on a GC Loader; it does **not** run in Dolphin, even with IPL.bin set!

## Using In-Game Reset
1. Download [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip).
2. Extract its contents to the **root** of the SD card (this drops `apploader.img` into `swiss/patches/`).
3. Press **Z + A + START** while in a game to return to the cubiboot menu.

## Configuration
Put a `config.ini` in the SD card root. `menu_grid_type` selects the selection-menu
grid layout (the layout also defaults to `small_banners` in code, so a `config.ini` is
optional):

| Value | Layout |
|-------|--------|
| `small_banners` | small banners, 4 columns (**default**) |
| `banners` | large banners, 3 columns |
| `square_icons` | square icons, 8 columns |

`default_folder` sets the directory the menu opens in at startup. Leave it unset (or
commented) to open the SD card root. A leading `/` is added automatically if you omit
it, and if the folder can't be opened cubiboot falls back to the root.

> **Note:** `default_folder` only changes where the menu browses for **games and
> homebrew** (`.dol`/`.dol.gz`/`.iso`/etc.) — those can live in a subfolder. The
> system files must still sit at the **SD card root**: `ipl.dol`, `config.ini`, and
> `swiss/patches/apploader.img`.

```ini
[cubeboot]

; Selection-menu grid layout:
;   small_banners = small banners, 4 columns  (default)
;   banners       = large banners, 3 columns
;   square_icons  = square icons, 8 columns
menu_grid_type = small_banners

; Folder the menu opens in at startup. Leave commented for the SD card root.
; default_folder = /games
```

## Building

**CI (recommended):** every push builds `ipl.dol` + `apploader.img` + `cubiboot.iso` +
`config.ini` + `cubiboot_picoloader.uf2` and uploads them as artifacts; pushing a `v*` tag
publishes a GitHub Release with those files plus `EXTRACT_TO_ROOT.zip`. See
[.github/workflows/ci.yml](.github/workflows/ci.yml).

**Local:** the build runs in a reproducible Docker image (devkitPPC + libogc2/libfat
pinned, GameCube-only) defined in [.ci/Dockerfile](.ci/Dockerfile):
```sh
docker build -t cubiboot-dev - < .ci/Dockerfile
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'cd entry && make clean && make'   # -> cubeboot/cubeboot.dol (ipl.dol)
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_apploader.sh' # -> apploader.img
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_iso.sh'       # -> cubiboot.iso (branded)
```
`apploader.img` is produced by packing `cubeboot.elf` with the
[swiss-gc packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer)
(reboot variant) and wrapping it in a GameCube-apploader header — see
[.ci/build_apploader.sh](.ci/build_apploader.sh).
`cubiboot.iso` is a GameCube El-Torito ISO9660 image built with `genisoimage` from
[cubeboot-tools'](https://github.com/makeo/cubeboot-tools) `gbi.hdr` (re-branded to the
Cubiboot banner) and the loader `.dol` as the boot image — see [.ci/build_iso.sh](.ci/build_iso.sh).
`cubiboot_picoloader.uf2` is the [PicoLoader](https://github.com/makeo/PicoLoader) firmware
with `cubiboot.iso` embedded as the payload (replicating makeo's PicoLoader converter) —
see [.ci/make_picoloader_uf2.py](.ci/make_picoloader_uf2.py).

See [docs/FORK_CHANGES.md](docs/FORK_CHANGES.md) for the full list of changes over makeo/cubiboot.

## Known limitations
- File loading is slow on FAT32 (use exFAT).
- Inherited from upstream: `cube_logo` and `button_*` options don't work (use gekkoboot for held-button programs).

## Credits & acknowledgements
This project stands on the work of others — the items below are **not** original to this fork:
- [cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) — the original GameCube IPL loader. (GPL-2.0)
- [cubiboot](https://github.com/makeo/cubiboot) by [makeo](https://github.com/makeo) — the SD2SP2 / SD Gecko fork this is based on. (GPL-2.0)
- The **grid / banner menu UI** (`custom-loader-menu`) by [Ben Hetherington](https://github.com/BenHetherington), ported from cubeboot. (GPL-2.0)
- [Swiss](https://github.com/emukidid/swiss-gc) by [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) and contributors — the game/app loader cubiboot chainloads. (GPL-2.0)
- [PicoLoader](https://github.com/makeo/PicoLoader) by [makeo](https://github.com/makeo) — the RP2040 ODE the `.uf2` targets. (GPL-2.0)
- [apploader / cubeboot-tools](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
- [packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) (from Swiss) — used to build `apploader.img`. (GPL-2.0)
- The **`default_folder`** config option by [wins1ey](https://github.com/wins1ey), via the [Hazado/cubiboot](https://github.com/Hazado/cubiboot) fork ([merge](https://github.com/Hazado/cubiboot/commit/c91066b4889346fec288393f6a9fe41304652e49)) — ported into this fork. (GPL-2.0)
- For the full breakdown, see upstream [CREDIT.md](https://github.com/makeo/cubiboot/blob/main/CREDIT.md).
