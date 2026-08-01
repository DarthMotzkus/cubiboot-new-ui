<div align="center">

# cubiboot-new-ui

**A GameCube IPL replacement that boots your games from a grid of banners.**

<img width="320" height="240" alt="cubiboot menu" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

A fork of [makeo/cubiboot](https://github.com/makeo/cubiboot) — itself a fork of
[cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) —
with support for SD2SP2, SD Gecko and similar SD adapters.

</div>

---

## Contents

- [Highlights](#highlights)
- [Before you start](#before-you-start)
- [Downloads](#downloads)
- [Installation](#installation)
  - [Method 1: PicoBoot or PicoLoader with gekkoboot](#method-1-picoboot-or-picoloader-with-gekkoboot)
  - [Method 2: PicoLoader with cubiboot flashed in](#method-2-picoloader-with-cubiboot-flashed-in)
  - [Method 3: GC Loader and other ODEs](#method-3-gc-loader-and-other-odes)
  - [In-Game Reset](#in-game-reset)
- [Configuration](#configuration)
  - [All options](#all-options)
  - [Menu layout](#menu-layout)
  - [Starting folder](#starting-folder)
  - [Remember last played](#remember-last-played)
  - [Games on the ODE SD card](#games-on-the-ode-sd-card)
  - [Launching Swiss from the menu](#launching-swiss-from-the-menu)
  - [Cube color](#cube-color)
- [Large folders and the banner pool](#large-folders-and-the-banner-pool)
- [Known limitations](#known-limitations)
- [Building](#building)
- [Credits](#credits)

---

## Highlights

What this fork adds on top of [makeo/cubiboot](https://github.com/makeo/cubiboot):

| | |
|---|---|
| **Grid / banner menu UI** | Ported from cubeboot. Three layouts, selectable with [`menu_grid_type`](#menu-layout); defaults to `small_banners` even without a `config.ini`. |
| **Real filenames** | The list shows the `.iso` **filename** instead of the internal game name, and loads the correct banner for each disc of a multi-disc game (e.g. Resident Evil 0 Disc 1 / Disc 2). |
| **Remember last played** | [`remember_last_game = 1`](#remember-last-played) opens the menu in the folder of your last game with it already highlighted — press **A** and go. |
| **Games from the ODE SD** | [`load_from_ode_sd = on`](#games-on-the-ode-sd-card) reads games straight off the SD card inside a GC Loader style ODE. No second card reader needed. |
| **Cold-boot banner fix** | Banner pools live in low memory that PicoBoot doesn't clear on cold boot, so stale "in-use" flags used to alias buffers (corruption) or starve them (blank) — worse the colder the console. The pools are now zeroed at startup and banners stay resident in MRAM. |
| **Cubiboot branding** | A "Games" menu header, plus the Cubiboot banner on the loader and on the `.iso` BIOS intro (replacing the gc-linux "Game Play" banner). |
| **Automated releases** | CI rebuilds `apploader.img` (so In-Game Reset returns to *this* loader, not a stale one) and a flashable `cubiboot_picoloader.uf2`. |

Full changelog against upstream: [docs/FORK_CHANGES.md](docs/FORK_CHANGES.md).
How it all fits together: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Before you start

> [!IMPORTANT]
> - Format the SD card as **exFAT**, not FAT32. Loading is very slow on FAT32.
> - Keep `.iso` and `.dol` names under **28 characters**, or they get cropped in the list.
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

Your games can live anywhere, including subfolders — see [`default_folder`](#starting-folder).

## Downloads

Every tagged release (`v*`) publishes:

| File | What it is |
|------|------------|
| **`EXTRACT_TO_ROOT.zip`** | Everything that belongs on the SD card (`ipl.dol`, `config.ini`, `swiss/patches/apploader.img`). Extract it to the root of the card — the easiest starting point. |
| `ipl.dol` | The cubiboot loader (a GameCube IPL replacement). Booted via PicoBoot/PicoLoader + gekkoboot. |
| `cubiboot_picoloader.uf2` | PicoLoader firmware with cubiboot **embedded** — flash it to the RP2040 Pico; no loader file needed on the card. |
| `cubiboot.iso` | Bootable GameCube disc image for **GC Loader** and other ODEs, branded with the Cubiboot banner. |
| `apploader.img` | The Swiss **In-Game-Reset** redirect. Embeds *this build's* loader, so the reset combo returns to this menu. Goes in `SD:/swiss/patches/`. |
| `config.ini` | Minimal example config (`menu_grid_type = small_banners`). Goes in the card root. |

[**→ Latest release**](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest)

## Installation

Pick the one that matches your console:

| Your setup | Use |
|---|---|
| PicoBoot or PicoLoader modchip | [Method 1](#method-1-picoboot-or-picoloader-with-gekkoboot) — recommended, updates by swapping a file on the SD card |
| PicoLoader, and you want no loader file on the card | [Method 2](#method-2-picoloader-with-cubiboot-flashed-in) |
| GC Loader or another ODE, no modchip | [Method 3](#method-3-gc-loader-and-other-odes) |

### Method 1: PicoBoot or PicoLoader with gekkoboot

**Recommended.** Updating cubiboot later is just replacing a file on the SD card — no
disassembly.

1. Flash your Pico with the `.uf2` from [PicoBoot](https://github.com/webhdx/PicoBoot) or
   [PicoLoader](https://github.com/makeo/PicoLoader).
2. Download [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol)
   and copy it to the **root** of your SD card.
3. Put [Swiss](https://github.com/emukidid/swiss-gc/releases/latest) on the card as
   `swiss-gc.dol`, plus a [`config.ini`](#configuration) and your games.

### Method 2: PicoLoader with cubiboot flashed in

Cubiboot lives in the Pico's firmware, so nothing but games and `swiss-gc.dol` needs to be
on the card.

1. Flash your Pico with the `.uf2` from [PicoLoader](https://github.com/makeo/PicoLoader).
2. Download [`cubiboot_picoloader.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoloader.uf2).
3. Hold the button on the RP2040 Pico while plugging it into your PC.
4. Copy the `.uf2` to the USB drive that appears; the Pico reboots running cubiboot.
5. Put Swiss on your SD2SP2 / SD Gecko card as `swiss-gc.dol`, along with a
   [`config.ini`](#configuration) and your games.

> [!WARNING]
> With this method every cubiboot update means opening the console and re-flashing the Pico.
> Method 1 is easier to live with.

### Method 3: GC Loader and other ODEs

`cubiboot.iso` is a bootable GameCube disc image that simply *is* the cubiboot loader — no
modchip needed.

1. Download [`cubiboot.iso`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot.iso)
   and copy it onto your [GC Loader](https://gcloaderhq.com/)'s storage, in the folder you
   boot images from.
2. Boot `cubiboot.iso` from the GC Loader menu — it lands on the cubiboot menu.
3. Choose where the games come from:
   - **The ODE's own SD card** (no second reader): put a `config.ini` with
     `load_from_ode_sd = on` and `swiss-gc.dol` in the **root of that same card**. See
     [Games on the ODE SD card](#games-on-the-ode-sd-card).
   - **An SD card adapter** (SD2SP2 / SD Gecko): leave the option off — that's the default —
     and set the adapter's card up as usual.

### In-Game Reset

Optional, works with every method above.

1. Download [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip).
2. Extract it to the **root** of the SD card — this drops `apploader.img` into
   `swiss/patches/`.
3. Press **Z + A + START** in a game to return to the cubiboot menu.

## Configuration

Put a `config.ini` in the root of the card cubiboot reads from. It's optional: without one
you get the `small_banners` layout and the card root as the starting folder.

```ini
[cubeboot]

; Selection-menu grid layout:
;   small_banners = small banners, 4 columns  (default)
;   banners       = large banners, 3 columns
;   square_icons  = square icons, 8 columns
menu_grid_type = small_banners

; Change the boot Cube logo color (hex, example orange):
; cube_color = ff9801

; Folder the menu opens in at startup. Leave commented for the card root.
; default_folder = /games

; Pre-select the last game you booted when the menu opens (1 = on, 0 = off).
remember_last_game = 0

; Read games off the SD card inside a GC Loader / ODE (on = yes, off = no).
load_from_ode_sd = off
```

### All options

| Key | Values | Default | What it does |
|-----|--------|---------|--------------|
| [`menu_grid_type`](#menu-layout) | `small_banners` · `banners` · `square_icons` | `small_banners` | Menu grid layout |
| [`default_folder`](#starting-folder) | path | card root | Folder the menu opens in |
| [`remember_last_game`](#remember-last-played) | `1` · `0` | `0` | Pre-select the last game you booted |
| [`load_from_ode_sd`](#games-on-the-ode-sd-card) | `on` · `off` | `off` | Read games from the ODE's own SD card |
| [`cube_color`](#cube-color) | hex RGB | stock | Boot logo color |
| `force_progressive` | `1` · `0` | `0` | Force progressive scan |

Other keys inherited from upstream are parsed in
[`cubeboot/source/settings.c`](cubeboot/source/settings.c); see also
[docs/settings.md](docs/settings.md). Note that `cube_logo` and `button_*` **do not work**
(see [Known limitations](#known-limitations)).

### Menu layout

| Value | Layout |
|-------|--------|
| `small_banners` | small banners, 4 columns (**default**) |
| `banners` | large banners, 3 columns |
| `square_icons` | square icons, 8 columns |

`square_icons` is the one to use for very large folders — see
[Large folders and the banner pool](#large-folders-and-the-banner-pool).

### Starting folder

`default_folder` sets the directory the menu opens in. Leave it unset (or commented) to open
the card root. A leading `/` is added automatically if you omit it, and if the folder can't
be opened cubiboot falls back to the root.

> [!NOTE]
> `default_folder` only changes where the menu browses for **games and homebrew**
> (`.dol` / `.dol.gz` / `.iso` / …). The system files still have to sit at the card root:
> `ipl.dol`, `config.ini`, `swiss-gc.dol` and `swiss/patches/apploader.img`.

### Remember last played

`remember_last_game = 1` makes the menu open **in the folder of the last game you booted**,
with that game already highlighted — so on the next boot you just press **A**. Off by default.

> [!IMPORTANT]
> This reads Swiss's own recent list, so Swiss has to be keeping one. In Swiss, open
> **Settings** and set **Recent List** to **On** (it writes `RecentListLevel=On` into
> `/swiss/settings/global.ini`). With it **Off** there is no `recent.ini` to read and
> cubiboot falls back to [`default_folder`](#starting-folder).

<details>
<summary><b>How it works, and how it interacts with <code>default_folder</code></b></summary>

<br/>

- Cubiboot boots games by chainloading **Swiss** with autoload, so Swiss records every launch
  in its own recent-games list (`/swiss/settings/recent.ini`). Cubiboot just **reads** that
  list back — there is no extra file to write.
- On the next cold boot the menu opens directly in the folder holding the most recent game
  — including a letter/genre subfolder, not just `default_folder` — and highlights it.
  Navigate away normally (**B** goes up a level).
- **No stalls:** for that first folder cubiboot does *not* wait for every banner before
  showing the list. It scans the folder (fast — headers only), puts the cursor on your last
  game, and a **background thread** fills banners in priority order: the on-screen window
  around your game first, so it appears almost immediately regardless of folder size, then
  the rest while the menu is already usable. Pressing **A** works even while banners are
  still loading.

**`remember_last_game` overrides `default_folder`.** When it's on, the menu **always** opens
in the last played folder. `default_folder` (or the card root, if unset) is only a fallback:
on the very first boot before any game has been played, or if the last game's folder is gone.

**Large folders:** if the last played folder holds more games than fit in the banner pool
(>128), it falls back to the sliding window — banners are read from the card as they scroll
into view. Either way your highlighted game shows first. See
[Large folders and the banner pool](#large-folders-and-the-banner-pool).

</details>

### Games on the ODE SD card

`load_from_ode_sd = on` makes cubiboot read from the SD card that sits **inside** the ODE —
a [GC Loader](https://gcloaderhq.com/) or anything else answering the same drive commands —
so the menu lists and boots the games already on it, with no second card reader.

Accepts `on` / `off` (and `1`/`0`, `true`/`false`, `yes`/`no`). Defaults to `off`, which is
the classic behaviour: EXI card readers only.

Two things to know:

- **One volume at a time.** With this on, *everything* cubiboot reads comes off the ODE's
  card — the IPL dump, `swiss-gc.dol`, banners and games. Keep them together on that card.
- **Where `config.ini` is read from.** Cubiboot has to read `config.ini` before it can know
  this setting, so it mounts the first card it finds, trying EXI card readers first and the
  ODE's card last. On a console with no card reader that *is* the ODE's card — exactly where
  you want `config.ini`. If you have both, put `config.ini` on the card reader and turn this
  on to move everything else to the ODE.

The ODE's card is mounted read-only. More detail in [docs/settings.md](docs/settings.md).

### Launching Swiss from the menu

Drop Swiss's `.dol` *or* `.iso` in any folder and give it a name starting with `swiss`
(e.g. `swiss-gc.dol`, `Swiss v0.6r2073.iso`). Cubiboot boots a `swiss…`-named image
**directly through its own apploader** instead of handing it to Swiss — without that prefix
a Swiss disc image just resets to the stock IPL.

This is separate from the `swiss-gc.dol` engine that must sit at the card **root**.

### Cube color

Set the GameCube boot logo color with a hex RGB code
([color picker](https://www.w3schools.com/colors/colors_hexadecimal.asp)):

```ini
cube_color = ff9801   ; spice orange
```

## Large folders and the banner pool

Banners live in a fixed low-memory pool capped at **128 banner images** (ARAM streaming was
dropped — it corrupted banners on load). That cap defines two modes:

| Files in the folder | Behaviour |
|---|---|
| **≤ 128** | All banners stay resident. Scrolling is instant. Best case for the banner layouts. |
| **> 128** | The pool fills, then switches to an **on-demand sliding window** — off-screen banners are freed and re-read from the card as you scroll. Names still appear instantly; only images load on demand. |

> [!WARNING]
> In on-demand mode the banner layout gets sluggish and **may crash** while scrolling. For a
> very large list in one folder, switch to `menu_grid_type = square_icons`. Filenames,
> last-played and default-folder all still work in that layout.

**Tips**

- Keep folders under 128 files for instant scrolling.
- Split big libraries into subfolders (genre, favourites, next-to-play).
- For a large library you want to keep in one folder, use the cube layout.

The 128 limit is a fail-safe. It can be raised in code, but that risks out-of-memory errors.

## Known limitations

- File loading is slow on FAT32 — use **exFAT**.
- Neither `ipl.dol` nor `cubiboot.iso` runs in **Dolphin Emulator**, even with an IPL.bin set.
- Inherited from upstream: `cube_logo` and `button_*` don't work (use gekkoboot for
  held-button programs).
- The banner layouts may crash in folders over 128 files — see
  [Large folders and the banner pool](#large-folders-and-the-banner-pool).

## Building

### CI (recommended)

Every push builds `ipl.dol` + `apploader.img` + `cubiboot.iso` + `config.ini` +
`cubiboot_picoloader.uf2` and uploads them as artifacts. Pushing a `v*` tag publishes a
GitHub Release with those files plus `EXTRACT_TO_ROOT.zip`. See
[.github/workflows/ci.yml](.github/workflows/ci.yml).

### Local

The build runs in a reproducible Docker image (devkitPPC + libogc2/libfat pinned,
GameCube-only) defined in [.ci/Dockerfile](.ci/Dockerfile):

```sh
docker build -t cubiboot-dev - < .ci/Dockerfile
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'cd entry && make clean && make'    # -> cubeboot/cubeboot.dol (ipl.dol)
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_apploader.sh' # -> apploader.img
docker run --rm -v "$PWD":/work cubiboot-dev bash -lc 'bash /work/.ci/build_iso.sh'       # -> cubiboot.iso (branded)
```

<details>
<summary><b>How each artifact is produced</b></summary>

<br/>

- **`apploader.img`** — `cubeboot.elf` packed with the
  [swiss-gc packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) (reboot
  variant) and wrapped in a GameCube-apploader header. See
  [.ci/build_apploader.sh](.ci/build_apploader.sh).
- **`cubiboot.iso`** — a GameCube El-Torito ISO9660 image built with `genisoimage` from
  [cubeboot-tools'](https://github.com/makeo/cubeboot-tools) `gbi.hdr` (re-branded to the
  Cubiboot banner), with the loader `.dol` as the boot image. See
  [.ci/build_iso.sh](.ci/build_iso.sh).
- **`cubiboot_picoloader.uf2`** — the [PicoLoader](https://github.com/makeo/PicoLoader)
  firmware with `cubiboot.iso` embedded as the payload, replicating makeo's PicoLoader
  converter. See [.ci/make_picoloader_uf2.py](.ci/make_picoloader_uf2.py).

</details>

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
- For the full breakdown, see upstream [CREDIT.md](https://github.com/makeo/cubiboot/blob/main/CREDIT.md) and this fork's [CREDIT.md](CREDIT.md).
