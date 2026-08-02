<div align="center">

# cubiboot-new-ui

**A GameCube IPL replacement that boots your games from a grid of banners.**

<img width="320" height="240" alt="cubiboot menu" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

A fork of [makeo/cubiboot](https://github.com/makeo/cubiboot) — itself a fork of
[cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) —
with support for SD2SP2, SD Gecko, GC Loader/CUBE ODE and similar SD adapters.

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
  - [Homebrew apps](#homebrew-apps)
  - [Where games are read from](#where-games-are-read-from)
  - [Launching Swiss from the menu](#launching-swiss-from-the-menu)
  - [Colors](#colors)
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
| **Homebrew apps with banners** | A folder holding `default.dol` next to `opening.bnr` is listed as a launchable app with its own banner, instead of a folder you have to open. See [Homebrew apps](#homebrew-apps). |
| **Remember last played** | [`remember_last_game = 1`](#remember-last-played) opens the menu in the folder of your last game with it already highlighted — press **A** and go. |
| **Games from the ODE SD** | [`device_order`](#where-games-are-read-from) can point cubiboot at the SD card inside a GC Loader style ODE, so the menu lists what is already on it with no second card reader. |
| **Cold-boot banner fix** | Banner pools live in low memory that PicoBoot doesn't clear on cold boot, so stale "in-use" flags used to alias buffers (corruption) or starve them (blank) — worse the colder the console. The pools are now zeroed at startup and banners stay resident in MRAM. |
| **Folder name in the header** | The menu header names the folder you are browsing; at the card root it reads "CUBIBOOT New UI". |
| **Cubiboot branding** | The Cubiboot banner on the loader and on the `.iso` BIOS intro, replacing the gc-linux "Game Play" one. |
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
   - **The ODE's own SD card** (no second reader): put `swiss-gc.dol` and a `config.ini`
     containing `device_order = gcldr` in the **root of that same card**. See
     [Where games are read from](#where-games-are-read-from).
   - **An SD card adapter** (SD2SP2 / SD Gecko): nothing to set — card readers come first by
     default. Set the adapter's card up as usual.

### In-Game Reset

Optional, works with every method above.

1. Download [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip).
2. Extract it to the **root** of the SD card — this drops `apploader.img` into
   `swiss/patches/`.
3. Press **Z + A + START** in a game to return to the cubiboot menu.

## Configuration

Put a `config.ini` in the root of the card cubiboot reads from. It's optional: without one
you get the `small_banners` layout and the card root as the starting folder.

Releases ship a commented template with every option — it is
[`.ci/config.ini`](.ci/config.ini) in this repo, and `config.ini` in the release zip. The
block below is the short version.

```ini
[cubeboot]

; Selection-menu grid layout:
;   small_banners = small banners, 4 columns  (default)
;   banners       = large banners, 3 columns
;   square_icons  = square icons, 8 columns
menu_grid_type = small_banners

; One color for the whole UI -- boot logo, menu cubes, the info box at the bottom of the
; game list and the big PRESS START (hex, example orange):
; theme_color = ff9801

; Change only the boot Cube logo color (hex, overrides theme_color). Any color key also
; accepts `random`, which re-rolls on every boot:
; cube_color = ff9801
; cube_color = random

; Folder the menu opens in at startup. Leave commented for the card root.
; default_folder = /games

; Pre-select the last game you booted when the menu opens (1 = on, 0 = off).
remember_last_game = 0

; Which storage to read games from, most wanted first: sd2sp2, slot_b, slot_a, ode.
; The FatFs volume names (sdc, sdb, sda, gcldr) also work.
; Leave commented for the default below.
; device_order = sd2sp2, slot_b, slot_a, ode
```

### All options

| Key | Values | Default | What it does |
|-----|--------|---------|--------------|
| [`menu_grid_type`](#menu-layout) | `small_banners` · `banners` · `square_icons` | `small_banners` | Menu grid layout |
| [`default_folder`](#starting-folder) | path | card root | Folder the menu opens in |
| [`remember_last_game`](#remember-last-played) | `1` · `0` | `0` | Pre-select the last game you booted |
| [`device_order`](#where-games-are-read-from) | device names | `sd2sp2, slot_b, slot_a, ode` | Which storage to read games from |
| [`theme_color`](#colors) | hex RGB · `random` | stock | One color for the whole UI |
| [`cube_color`](#colors) | hex RGB · `random` | `theme_color` | Boot logo color |
| [`menu_cube_color`](#colors) | hex RGB · `random` · palette name | `theme_color` | Grid cubes / banner tiles |
| [`menu_box_color`](#colors) | hex RGB · `random` | `theme_color` | Info panel under the game list |
| [`menu_start_color`](#colors) | hex RGB · `random` | `theme_color` | The big block "PRESS START" |
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

### Where games are read from

`device_order` lists the storage cubiboot should use, most wanted first. The first entry
that mounts becomes the volume everything is read from: the IPL dump, `swiss-gc.dol`,
banners and the games the menu lists.

| Name | Where it is |
|------|-------------|
| `sd2sp2` (or `sdc`) | Serial Port 2 — an **SD2SP2** |
| `slot_b` (or `sdb`) | Memory card **slot B** — an SD Gecko |
| `slot_a` (or `sda`) | Memory card **slot A** — an SD Gecko |
| `ode`, `gcloader` (or `gcldr`) | The SD card **inside the ODE** — a [GC Loader](https://gcloaderhq.com/) or anything answering the same drive commands |

The default, used when the key is absent:

```ini
device_order = sd2sp2, slot_b, slot_a, ode
```

Leaving a device out is how you keep cubiboot off it — there is no separate on/off switch.
So a console with both an SD2SP2 and a GC Loader, whose games live on the ODE, says:

```ini
device_order = ode
```

Two things to know:

- **One volume at a time.** cubiboot does not merge cards. Whichever entry wins holds
  everything: `swiss-gc.dol`, the games, and `ipl.bin` if you use one.
- **`config.ini` can live on any of them.** cubiboot looks for it on every device it can
  mount, in the default order above, and reads the first one that actually has the file —
  it has to, since `device_order` lives *inside* that file. If two cards both carry a
  `config.ini`, the default order breaks the tie.

The ODE's card is mounted read-only. A console without an ODE pays one drive inquiry per
boot for the `gcldr` entry, which gives up as soon as a real optical drive answers.

### Homebrew apps

A folder that holds **`default.dol`** and **`opening.bnr`** side by side is treated as an
application, not as a folder. It shows up in the grid with the banner from its `opening.bnr`,
and pressing **A** runs the `.dol` directly instead of opening the folder.

```
/apps/
  my-app/
    default.dol     <- what gets launched
    opening.bnr     <- name, description and banner art
  another-app/
    default.dol
    opening.bnr
```

Both filenames are fixed. The banner is the same format retail discs use, so a title,
description and 96x32 image all come from that one file.

A folder missing either file behaves exactly as before — you enter it and browse. The check
costs one file probe per folder while the list is being built, and folders without an
`opening.bnr` stop right there, so a library of game folders is unaffected.

**Making the banner.** [`tools/banner-converter/run.py`](tools/banner-converter) turns any
image into an `opening.bnr`. Download it from this repo, put your artwork next to it and run:

```sh
pip install Pillow
python run.py
```

Pick option **2**, answer the title/author/description prompts, and it writes
`output/<name>/opening.bnr`. Drop your `default.dol` beside that file and the folder is ready
for the card. See [its README](tools/banner-converter) for the sizing rules — the slot is
96×32, so a wordmark that looks thin wants a *taller* source, not a wider one.

### Launching Swiss from the menu

Drop Swiss's `.dol` *or* `.iso` in any folder and give it a name starting with `swiss`
(e.g. `swiss-gc.dol`, `Swiss v0.6r2073.iso`). Cubiboot boots a `swiss…`-named image
**directly through its own apploader** instead of handing it to Swiss — without that prefix
a Swiss disc image just resets to the stock IPL.

Swiss as a homebrew app works too: `apps/swiss/default.dol` is recognised by the **folder**
name, since the file inside is always `default.dol`. It runs directly rather than being
handed to Swiss — and it stays launchable even with no `swiss-gc.dol` at the root, so a card
that only has Swiss as an app can still start it.

This is separate from the `swiss-gc.dol` engine that must sit at the card **root** for games.

### Colors

Every color key takes a hex RGB code
([color picker](https://www.w3schools.com/colors/colors_hexadecimal.asp)) or `random`, which
picks a different color on every boot — it seeds from the console clock, so a console with a
dead RTC battery will keep landing on the same one. `theme_color` is the one-liner; the rest
are per-item overrides on top of it.

```ini
theme_color = ff9801   ; spice orange everywhere
```

| Key | What it paints |
|-----|----------------|
| `theme_color` | Everything below, unless that item is set explicitly |
| `cube_color` | The boot logo cube only |
| `menu_cube_color` | The cubes / banner tiles in the game grid |
| `menu_box_color` | The info panel under the game list (filename, description, thumbnail) |
| `menu_start_color` | The big block "PRESS START" on the pre-boot screen |

With no color key at all you get the stock look, unchanged.

**The grid cubes keep their shading.** The IPL ships four shades per cube — bright, dimmed,
and a selected variant of each — and `menu_cube_color` moves that whole set onto your color
instead of flattening it, so the selected cube still stands out. You can also just name one
of the six palettes the IPL already has, which uses Nintendo's own shades verbatim:

```ini
menu_cube_color = green   ; blue | green | yellow | orange | red | purple (default)
```

Naming a palette *and* setting `theme_color` picks that palette and then tints it.

**The info panel is a gradient, from one color.** It is darkest at the bottom and brightens
upward into the color you set, so `menu_box_color` is that bright end and the dark end is
your color at ~20% lightness. Same hue and saturation throughout, so you pick one color and
the shading takes care of itself. The dark end goes much further down than the stock panel's,
because the stock one also swings its hue from purple to magenta; copying that swing onto an
arbitrary color turns the gradient into a clash, so lightness carries the effect alone and
has to work harder for it.

**The big "PRESS START"** is drawn by the stock BIOS, which offers no color parameter, so
cubiboot recolors the block palette it reads. Only the RGB is touched — the per-block
intensity that drives the fly-in and fade is left alone, so the animation is unchanged. The
small `Press START to begin!` line above it is a separate draw and always stays white.

> [!NOTE]
> This one is per-IPL-revision, and all seven revisions cubiboot can boot on are covered:
> NTSC 1.0-001, 1.1, 1.2-001, 1.2-101, PAL 1.0-001, PAL 1.2-101 and MPAL. The remaining IPLs
> the menu can recognise are NPDP / dev-kit BIOSes, which the loader refuses to boot anyway.

> [!NOTE]
> `000000` works as an actual black, and `random` picks a fresh color on every boot.

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
