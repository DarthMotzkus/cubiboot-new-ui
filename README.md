<div align="center">

# cubiboot-new-ui

**A GameCube IPL replacement that boots your games from a grid of banners, and much more!**

<img width="320" height="240" alt="cubiboot menu" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

**Hello there! Welcome. FIRST OF ALL: This is not a simple forwarder as the original cubeboot/cubiboot. It's support too many devices, settings and customizations... You HAVE to read this README bellow AND the docs about the [settings](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/settings.md) you want to use. Use AI to a faster Q&A, it will read the /docs folder for a much faster support for your questions.**   

A fork of [makeo/cubiboot](https://github.com/makeo/cubiboot) — itself a fork of
[cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) —
with support for SD2SP2, SD Gecko, GC Loader/CUBE ODE, FlippyDrive and similar SD adapters.

</div>

---

## Contents

- [Highlights](#highlights)
- [Before you start](#before-you-start)
- [Downloads](#downloads)
- [Installation](#installation)
- [Configuration](#configuration)
- [Known limitations](#known-limitations)
- [Building](#building)
- [Documentation](#documentation)
- [Credits](#credits)

---

## Highlights

What this fork adds on top of [makeo/cubiboot](https://github.com/makeo/cubiboot):

| | |
|---|---|
| **Grid / banner menu UI** | Ported from cubeboot. Three layouts, selectable with [`menu_grid_type`](docs/settings.md#menu_grid_type); defaults to `small_banners` even without a `config.ini`. |
| **Real filenames** | The list shows the `.iso` **filename** instead of the internal game name, and loads the correct banner for each disc of a multi-disc game (e.g. Resident Evil 0 Disc 1 / Disc 2). |
| **Homebrew apps with banners** | A folder holding `default.dol` next to `opening.bnr` is listed as a launchable app with its own banner, instead of a folder you have to open. See [Homebrew apps](docs/settings.md#homebrew-apps). |
| **Remember last played** | [`remember_last_game = on`](docs/settings.md#remember_last_game) opens the menu in the folder of the last game or app you booted, already highlighted — press **A** and go. |
| **Text scrolling** | A title longer than the info box [scrolls on its own](docs/settings.md#text_scroll); the description scrolls with **L**/**R**. |
| **Games from the ODE / FlippyDrive SD** | [`device_order`](docs/settings.md#device_order) can point cubiboot at the SD card inside a GC Loader/CUBE-ODE or a FlippyDrive, so the menu lists what is already on it with no second card reader. |
| **16:9 widescreen menu** | [`force_widescreen = on`](docs/settings.md#force_widescreen) renders the whole menu anamorphic, so it comes out proportioned on a TV set to Full/16:9. Ported from [cubeboot PR #57](https://github.com/OffBroadway/cubeboot/pull/57). |
| **Folder name in the header** | The menu header names the folder you are browsing; at the card root it reads your device names (e.g. SD2SP2, ODE SD, FLIPPY SD, SLOT A/B SD). |
| **Custom boot logo** | [`cube_logo`](docs/settings.md#cube_logo) swaps the "GAMECUBE" text in the boot animation for your own art. Draw it anywhere, then convert the PNG with the [cube logo converter](tools/cube-logo-converter/) — a browser page with live preview, nothing to install. |
| **Cubiboot branding** | The Cubiboot banner on the loader and on the `.iso` BIOS intro, replacing the gc-linux "Game Play" one. |

## Before you start

> [!IMPORTANT]
> - Format the SD card (from any size) as **exFAT**, not FAT32.
> - Neither `ipl.dol` nor `cubiboot.iso` runs in **Dolphin Emulator**, even with an IPL.bin configured.

You will also need [Swiss](https://github.com/emukidid/swiss-gc/releases/latest): cubiboot
chainloads it to actually boot games. Rename its `.dol` to **`swiss-gc.dol`** and put it in
the **root** of the card cubiboot reads from.

These files always live at the **root** of that card:

```
/ipl.dol                        (only for installation Method 1)
/config.ini
/swiss-gc.dol
/swiss/patches/apploader.img    (only if you want In-Game Reset)
```

Your games can live anywhere, including subfolders — see
[`default_folder`](docs/settings.md#default_folder). On a **FlippyDrive** the last two lines
work differently — see [Method 4](docs/INSTALL.md#method-4-flippydrive).

## Downloads

Every tagged release (`v*`) publishes:

| File | What it is |
|------|------------|
| **`EXTRACT_TO_ROOT.zip`** | Everything that belongs on the SD card (`ipl.dol`, `config.ini`, `swiss/patches/apploader.img`). Extract it to the root of the card — the easiest starting point. |
| `ipl.dol` | The cubiboot loader (a GameCube IPL replacement). Booted via PicoBoot/PicoLoader + gekkoboot. |
| `flippydrive.dol` | The loader for a **FlippyDrive** — same binary as `ipl.dol`. **Rename it to `cubeboot.dol`** and flash it into the drive; see [Method 4](docs/INSTALL.md#method-4-flippydrive). |
| `cubiboot_picoloader_payload.uf2` | PicoLoader firmware with cubiboot **embedded** — flash it to the RP2040 Pico; no loader file needed on the card. |
| `cubiboot_picoboot_payload.uf2` | The cubiboot payload for **PicoBoot** — flash it on top of the official [PicoBoot](https://github.com/webhdx/PicoBoot/releases) firmware (≥ v0.4; Pico 2 needs v0.5.0) and it replaces the stock gekkoboot in place; no `ipl.dol` or gekkoboot needed on the card. One file for both boards. |
| `cubiboot.iso` | Bootable GameCube disc image for **GC Loader** and **CUBE-ODE**, branded with the Cubiboot banner. |
| `apploader.img` | The Swiss **In-Game-Reset** redirect. Embeds *this build's* loader, so the reset combo returns to this menu — which is why it has to be replaced on every [update](docs/INSTALL.md#updating). Goes in `SD:/swiss/patches/`, and Swiss's **In-Game Reset** setting must be set to **`Apploader`** ([details](docs/INSTALL.md#in-game-reset)). Not used on a **FlippyDrive**, which gets IGR through Swiss's **Reboot** option instead. |
| `config.ini` | Minimal example config (`menu_grid_type = small_banners`). Goes in the card root. |

[**→ Latest release**](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest)

## Installation

The most common setup — a **PicoBoot or PicoLoader** modchip with gekkoboot — and updating
later is just swapping files on the card:

1. **Wipe the Pico first** (recommended): hold **BOOTSEL** while plugging the Pico into
   your PC and copy
   [`universal_flash_nuke.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/raw/main/tools/flash-nuke/universal_flash_nuke.uf2)
   to the USB drive that appears. It erases the flash completely and drops the Pico
   straight back into BOOTSEL, ready for the next step. Skipping this is the classic cause
   of a **double boot**: leftovers of whatever was flashed before survive next to the new
   install, and the console boots through two loaders — or keeps loading the old one.
   Works on Pico and Pico 2; see [tools/flash-nuke](tools/flash-nuke/README.md).
2. Flash your Pico with the `.uf2` for the chip you actually have — they are different
   boards, not two names for one thing: [PicoBoot](https://github.com/webhdx/PicoBoot) for a
   PicoBoot install, or
   [PicoLoader+Gekkoboot](https://github.com/makeo/PicoLoader/releases/download/v1.3/picoloader_gekkoboot.uf2)
   for a PicoLoader one.
3. Download [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol)
   and copy it to the **root** of your SD card.
4. Put [Swiss](https://github.com/emukidid/swiss-gc/releases/latest) on the card as
   `swiss-gc.dol`, plus a [`config.ini`](#configuration) and your games.

Every other path is in the **[install guide](docs/INSTALL.md)**:

| Your setup | Guide |
|---|---|
| PicoBoot / PicoLoader with gekkoboot (the steps above, in full) | [Method 1](docs/INSTALL.md#method-1-picoboot-or-picoloader-with-gekkoboot) |
| cubiboot flashed **into** the modchip — no loader file on the card | [Method 2](docs/INSTALL.md#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) |
| GC Loader or CUBE-ODE, no modchip | [Method 3](docs/INSTALL.md#method-3-gc-loader-and-cube-ode) |
| FlippyDrive — the drive boots cubiboot itself | [Method 4](docs/INSTALL.md#method-4-flippydrive) |
| **In-Game Reset** (Z + A + START back to the menu) | [Setup](docs/INSTALL.md#in-game-reset) |
| **Updating** — which files to replace, and why both | [Updating](docs/INSTALL.md#updating) |

## Configuration

Put a `config.ini` in the root of the card cubiboot reads from. It's optional: without one
you get the `small_banners` layout and the card root as the starting folder.

Releases ship a commented template with every option — it is
[`.ci/config.ini`](.ci/config.ini) in this repo, and `config.ini` in the release zip.

| Key | Values | Default | What it does |
|-----|--------|---------|--------------|
| [`menu_grid_type`](docs/settings.md#menu_grid_type) | `small_banners` · `banners` · `square_icons` | `small_banners` | Menu grid layout |
| [`default_folder`](docs/settings.md#default_folder) | path | card root | Folder the menu opens in |
| [`remember_last_game`](docs/settings.md#remember_last_game) | `on` · `off` | `off` | Pre-select the last game or app you booted |
| [`device_order`](docs/settings.md#device_order) | device names | `sd2sp2, slot_b, slot_a, ode, flippy` | Which storage to read games from |
| [`theme_color`](docs/settings.md#colors) | [hex RGB](https://www.w3schools.com/colors/colors_hexadecimal.asp) · `random` | stock | One color for the whole UI |
| [`cube_color`](docs/settings.md#colors) | [hex RGB](https://www.w3schools.com/colors/colors_hexadecimal.asp) · `random` | `theme_color` | Boot logo color |
| [`menu_cube_color`](docs/settings.md#colors) | [hex RGB](https://www.w3schools.com/colors/colors_hexadecimal.asp) · `random` · palette name | `theme_color` | Grid cubes / banner tiles |
| [`menu_box_color`](docs/settings.md#colors) | [hex RGB](https://www.w3schools.com/colors/colors_hexadecimal.asp) · `random` | `theme_color` | Info panel under the game list |
| [`menu_start_color`](docs/settings.md#colors) | [hex RGB](https://www.w3schools.com/colors/colors_hexadecimal.asp) · `random` | `theme_color` | The big block "PRESS START" |
| [`preboot_delay_ms`](docs/settings.md#preboot_delay_ms-and-postboot_delay_ms) | milliseconds | `0` | Wait before the boot animation, for a TV to lock on |
| [`postboot_delay_ms`](docs/settings.md#preboot_delay_ms-and-postboot_delay_ms) | milliseconds | `0` | Hold the last frame after picking a game, before it boots |
| [`force_widescreen`](docs/settings.md#force_widescreen) | `on` · `off` | `off` | Render the menu anamorphic for a 16:9 TV |
| [`force_progressive`](docs/settings.md#force_progressive) | `on` · `off` | `off` | Menu in 480p — IPL 1.1/1.2 only, safely ignored on IPL 1.0 |
| [`text_scroll`](docs/settings.md#text_scroll) | `on` · `off` · seconds | `on` (2 s) | Long titles scroll after this delay |
| [`big_titles_scroll_speed`](docs/settings.md#big_titles_scroll_speed) | `1`–`255` | `10` | Marquee pace in frames per character (bigger = slower) |
| [`swiss_on_dvd_boot`](docs/settings.md#swiss_on_dvd_boot) | `on` · `off` | `on` | Boot physical discs through Swiss, which is what carries IGR and the region bypass |

On/off switches take `on` or `off`; `1`/`0`, `yes`/`no` and `true`/`false` also work
(`text_scroll` is the one exception — there a number means seconds). A value that is neither
leaves the default alone instead of flipping the switch.

Every key, with the full behavior notes — colors, homebrew apps, launching Swiss from the
menu, the disc screen, large folders — is in **[docs/settings.md](docs/settings.md)**.

## Known limitations

- File loading is slow on FAT32 — use **exFAT**.
- Neither `ipl.dol` nor `cubiboot.iso` runs in **Dolphin Emulator**, even with an IPL.bin set.
- The banner layouts may crash in folders over 128 files — see
  [Large folders and the banner pool](docs/settings.md#large-folders-and-the-banner-pool).
- **Autobooting a disc from power-on** is not implemented and is not planned. The console's
  own BIOS already does it, so with gekkoboot installed you can hold **D-Pad Left** (or the
  reset button) at power-on to boot the stock IPL instead of cubiboot — see
  [`swiss_on_dvd_boot`](docs/settings.md#swiss_on_dvd_boot).

## Building

**CI (recommended):** every push builds `ipl.dol` + `apploader.img` + `cubiboot.iso` +
`config.ini` + `cubiboot_picoloader_payload.uf2` + `cubiboot_picoboot_payload.uf2` and
uploads them as artifacts. Pushing a `v*` tag publishes a GitHub Release with those files
plus `EXTRACT_TO_ROOT.zip`. See [.github/workflows/ci.yml](.github/workflows/ci.yml).

**Local:** the build runs in a reproducible Docker image (devkitPPC + libogc2/libfat pinned,
GameCube-only) defined in [.ci/Dockerfile](.ci/Dockerfile):

```sh
docker build -t cubiboot-dev - < .ci/Dockerfile
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'cd entry && make clean && make'    # -> cubeboot/cubeboot.dol (ipl.dol)
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_apploader.sh' # -> apploader.img
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_iso.sh'       # -> cubiboot.iso (branded)
```

How each release artifact is produced, and the whole boot chain, are in
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#how-each-release-artifact-is-produced).

## Documentation

| Doc | What it covers |
|---|---|
| [docs/INSTALL.md](docs/INSTALL.md) | Every install method in full, In-Game Reset, updating |
| [docs/settings.md](docs/settings.md) | Every `config.ini` key and how the menu treats the card |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | How the three-stage boot chain fits together (for developers) |
| [docs/FORK_CHANGES.md](docs/FORK_CHANGES.md) | Full changelog against upstream |
| [docs/README_es.md](docs/README_es.md) | README en español |

## Credits

This project stands on the work of others — the items below are **not** original to this fork:

- [cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) — the original GameCube IPL loader. (GPL-2.0)
- [cubiboot](https://github.com/makeo/cubiboot) by [makeo](https://github.com/makeo) — the SD2SP2 / SD Gecko fork this is based on. (GPL-2.0)
- The **grid / banner menu UI** (`custom-loader-menu`) by [Ben Hetherington](https://github.com/BenHetherington), ported from cubeboot. (GPL-2.0)
- [Swiss](https://github.com/emukidid/swiss-gc) by [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) and contributors — the game/app loader cubiboot chainloads. (GPL-2.0)
- [PicoLoader](https://github.com/makeo/PicoLoader) by [makeo](https://github.com/makeo) — the RP2040 ODE the `.uf2` targets. (GPL-2.0)
- [apploader / cubeboot-tools](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
- [packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) (from Swiss) — used to build `apploader.img`. (GPL-2.0)
- The **`default_folder`** config option by [wins1ey](https://github.com/wins1ey), via the [Hazado/cubiboot](https://github.com/Hazado/cubiboot) fork ([merge](https://github.com/Hazado/cubiboot/commit/c91066b4889346fec288393f6a9fe41304652e49)) — ported into this fork. (GPL-2.0)
- The **GC Loader ODE SD** block driver, reverse-engineered from a `cubiboot-gcldr.iso` build and cross-checked against libogc2's `DVD_LowGcodeRead`. (GPL-2.0)
- The **stock disc screen** infrastructure by [Jpe230](https://github.com/Jpe230), from [PR #8](https://github.com/DarthMotzkus/cubiboot-new-ui/pull/8) — the renderer dispatcher and the stock-screen addresses across all seven IPL revisions, which this fork's disc screen is built on. (GPL-2.0)
- For the full breakdown, see upstream [CREDIT.md](https://github.com/makeo/cubiboot/blob/main/CREDIT.md) and this fork's [CREDIT.md](CREDIT.md).
