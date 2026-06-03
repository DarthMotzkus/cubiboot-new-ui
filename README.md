# cubiboot-new-ui

A fork of [makeo/cubiboot](https://github.com/makeo/cubiboot) — itself a fork of
[cubeboot](https://github.com/OffBroadway/cubeboot) by
[TeamOffBroadway](https://github.com/OffBroadway) — with support for SD2SP2, SD Gecko
or similar SD adapters.

<img width="320" height="240" alt="ezgif-80bc8dbf18a70e39" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

**What this fork adds on top of makeo/cubiboot:**
- A **grid / banner menu UI** ported from cubeboot (selectable via `menu_grid_type`).
- A fix for **random banner corruption** (ARAM→MRAM cache-coherency bug in the banner cache).
- An automated **build & release pipeline** that also rebuilds `apploader.img` so
  In-Game Reset returns to *this* loader instead of a stale one.

> [!IMPORTANT]
> Format your SD card using **exFAT** (not FAT32). Loading files is very slow on FAT32.

## What's in a release

Each tagged release (`v*`) publishes:

| File | What it is |
|------|------------|
| `ipl.dol` | The cubiboot loader (a GameCube IPL replacement). Boot it via PicoBoot/PicoLoader + gekkoboot. |
| `apploader.img` | The Swiss **In-Game-Reset** redirect. It embeds *this build's* loader, so pressing the reset combo in a game returns to this cubiboot menu. Goes in `SD:/swiss/patches/`. |
| `config.ini` | Minimal example config (`menu_grid_type = small_banners`). Goes in the SD card root. |
| `cubiboot.iso` | Bootable GameCube disc image for **GC Loader** (and other ODEs). Put it on the GC Loader's SD/storage and boot it. |
| `EXTRACT_TO_ROOT.zip` | Everything that lives on the SD card (`ipl.dol`, `config.ini`, `swiss/patches/apploader.img`) — just extract it to the root of the card. |

> This fork's releases do **not** include a PicoBoot `.uf2` image.

## Installation — [PicoBoot](https://github.com/webhdx/PicoBoot) / [PicoLoader](https://github.com/makeo/PicoLoader) with gekkoboot payload
1. Download [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol) and copy it to the **root** of your SD card.
2. Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest) `.dol`, rename it to `swiss-gc.dol`, and place it on the SD card.

## Installation — [GC Loader](https://gcloaderhq.com/) (and other ODEs)
The `cubiboot.iso` is a bootable GameCube disc image that simply *is* the cubiboot
loader — no PicoBoot/modchip needed.
1. Download [`cubiboot.iso`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot.iso) and copy it onto your GC Loader's storage (SD/USB), in the folder you boot images from.
2. Boot `cubiboot.iso` from the GC Loader menu — it lands on the cubiboot menu.
3. cubiboot still lists/loads games from your **SD card adapter** (SD2SP2 / SD Gecko),
   so set that card up as usual: copy your games and a `config.ini` to it (see
   [Configuration](#configuration)). Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest)
   `.dol`, rename it to `swiss-gc.dol`, and place it on the SD card.

> The `.iso` is for booting the loader on a GC Loader; it does **not** run in Dolphin
> without a real GameCube IPL/BIOS configured (cubiboot is an IPL replacement).

## Using In-Game Reset
1. Download [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip).
2. Extract its contents to the **root** of the SD card (this drops `apploader.img` into `swiss/patches/`).
3. Press **Z + A + START** while in a game to return to the cubiboot menu.

## Configuration
Put a `config.ini` in the SD card root. `menu_grid_type` selects the selection-menu
grid layout:

| Value | Layout |
|-------|--------|
| `square_icons` | square icons, 8 columns (**default**) |
| `banners` | large banners, 3 columns |
| `small_banners` | small banners, 4 columns |

```ini
[cubeboot]

; Selection-menu grid layout:
;   square_icons  = square icons, 8 columns   (default)
;   banners       = large banners, 3 columns
;   small_banners = small banners, 4 columns
menu_grid_type = small_banners
```

## Building

**CI (recommended):** every push builds `ipl.dol` + `apploader.img` + `cubiboot.iso` +
`config.ini` and uploads them as artifacts; pushing a `v*` tag publishes a GitHub Release
with those files plus `EXTRACT_TO_ROOT.zip`. See [.github/workflows/ci.yml](.github/workflows/ci.yml).

**Local:** the build runs in a reproducible Docker image (devkitPPC + libogc2/libfat
pinned, GameCube-only) defined in [.ci/Dockerfile](.ci/Dockerfile):
```sh
docker build -t cubiboot-dev - < .ci/Dockerfile
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'cd entry && make clean && make'   # -> cubeboot/cubeboot.dol (ipl.dol)
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_apploader.sh' # -> apploader.img
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_iso.sh'       # -> cubiboot.iso
```
`apploader.img` is produced by packing `cubeboot.elf` with the
[swiss-gc packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer)
(reboot variant) and wrapping it in a GameCube-apploader header — see
[.ci/build_apploader.sh](.ci/build_apploader.sh).
`cubiboot.iso` is a GameCube El-Torito ISO9660 image built with `genisoimage` from
[cubeboot-tools'](https://github.com/makeo/cubeboot-tools) `gbi.hdr` (disc header +
apploader) and the loader `.dol` as the boot image — see
[.ci/build_iso.sh](.ci/build_iso.sh).

## Known limitations
- File loading is slow on FAT32 (use exFAT).
- No PicoBoot `.uf2` image in releases (the `.iso` is provided for GC Loader / ODEs).
- Inherited from upstream: `cube_logo` and `button_*` options don't work (use gekkoboot for held-button programs).

## Credits & acknowledgements
This project stands on the work of others — the items below are **not** original to this fork:
- [cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) — the original GameCube IPL loader. (GPL-2.0)
- [cubiboot](https://github.com/makeo/cubiboot) by [makeo](https://github.com/makeo) — the SD2SP2 / SD Gecko fork this is based on. (GPL-2.0)
- [Swiss](https://github.com/emukidid/swiss-gc) by [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) and contributors — the game/app loader cubiboot chainloads. (GPL-2.0)
- [apploader / cubeboot-tools](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
- [packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) (from Swiss) — used to build `apploader.img`. (GPL-2.0)
- For the full breakdown, see upstream [CREDIT.md](https://github.com/makeo/cubiboot/blob/main/CREDIT.md).
