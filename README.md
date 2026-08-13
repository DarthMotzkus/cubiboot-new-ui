<div align="center">

# cubiboot-new-ui

**A GameCube IPL replacement that boots your games from a grid of banners.**

<img width="320" height="240" alt="cubiboot menu" src="https://github.com/user-attachments/assets/eb1d6fc9-f0eb-4a38-8f93-20daa4a0af19" />

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
  - [Method 1: PicoBoot or PicoLoader with gekkoboot](#method-1-picoboot-or-picoloader-with-gekkoboot)
  - [Method 2: cubiboot flashed into the modchip (PicoBoot or PicoLoader)](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader)
  - [Method 3: GC Loader and CUBE-ODE](#method-3-gc-loader-and-other-odes)
  - [Method 4: FlippyDrive](#method-4-flippydrive)
  - [In-Game Reset](#in-game-reset)
- [Updating](#updating)
- [Configuration](#configuration)
  - [All options](#all-options)
  - [Menu layout](#menu-layout)
  - [Starting folder](#starting-folder)
  - [Remember last played](#remember-last-played)
  - [Homebrew apps](#homebrew-apps)
  - [Where games are read from](#where-games-are-read-from)
  - [Launching Swiss from the menu](#launching-swiss-from-the-menu)
  - [Colors](#colors)
  - [Widescreen (16:9)](#widescreen-169)
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
| **Games from the ODE SD** | [`device_order`](#where-games-are-read-from) can point cubiboot at the SD card inside a GC Loader/CUBE-ODE and FlippyDrive, so the menu lists what is already on it with no second card reader. |
| **16:9 widescreen menu** | [`force_widescreen = 1`](#widescreen-169) renders the whole menu anamorphic, so it comes out proportioned on a TV set to Full/16:9. Ported from [cubeboot PR #57](https://github.com/OffBroadway/cubeboot/pull/57). |
| **Cold-boot banner fix** | Banner pools live in low memory that PicoBoot doesn't clear on cold boot, so stale "in-use" flags used to alias buffers (corruption) or starve them (blank) — worse the colder the console. The pools are now zeroed at startup and banners stay resident in MRAM. |
| **Folder name in the header** | The menu header names the folder you are browsing; at the card root it reads "CUBIBOOT New UI". |
| **Cubiboot branding** | The Cubiboot banner on the loader and on the `.iso` BIOS intro, replacing the gc-linux "Game Play" one. |
| **Automated releases** | CI rebuilds `apploader.img` (so In-Game Reset returns to *this* loader, not a stale one) and a flashable `cubiboot_picoloader_payload.uf2`. |

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
| `flippydrive.dol` | The loader for a **FlippyDrive** — same binary as `ipl.dol`. **Rename it to `cubeboot.dol`** and flash it into the drive; see [Method 4](#method-4-flippydrive). |
| `cubiboot_picoloader_payload.uf2` | PicoLoader firmware with cubiboot **embedded** — flash it to the RP2040 Pico; no loader file needed on the card. |
| `cubiboot_picoboot_payload.uf2` | The cubiboot payload for **PicoBoot** — flash it on top of the official [PicoBoot](https://github.com/webhdx/PicoBoot/releases) firmware (≥ v0.4; Pico 2 needs v0.5.0) and it replaces the stock gekkoboot in place; no `ipl.dol` or gekkoboot needed on the card. One file for both boards. |
| `cubiboot.iso` | Bootable GameCube disc image for **GC Loader** and **CUBE-ODE**, branded with the Cubiboot banner. |
| `apploader.img` | The Swiss **In-Game-Reset** redirect. Embeds *this build's* loader, so the reset combo returns to this menu — which is why it has to be replaced on every [update](#updating). Goes in `SD:/swiss/patches/`, and Swiss's **In-Game Reset** setting must be set to **`Apploader`** ([details](#in-game-reset)). |
| `config.ini` | Minimal example config (`menu_grid_type = small_banners`). Goes in the card root. |

[**→ Latest release**](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest)

## Installation

Pick the one that matches your console:

| Your setup | Use |
|---|---|
| PicoBoot or PicoLoader modchip | [Method 1](#method-1-picoboot-or-picoloader-with-gekkoboot) — recommended, updates by swapping files on the SD card |
| PicoBoot or PicoLoader, and you want no loader file on the card | [Method 2](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) |
| GC Loader or CUBE-ODE, no modchip | [Method 3](#method-3-gc-loader-and-other-odes) |
| FlippyDrive | [Method 4](#method-4-flippydrive) — the drive boots cubiboot itself |

### Method 1: PicoBoot or PicoLoader with gekkoboot

**Recommended.** Updating cubiboot later is just replacing files on the SD card — no
disassembly. See [Updating](#updating) for which ones.

1. Flash your Pico with the `.uf2` from [PicoBoot](https://github.com/webhdx/PicoBoot) or
   [PicoLoader+Gekkoboot](https://github.com/makeo/PicoLoader/releases/download/v1.3/picoloader_gekkoboot.uf2).
2. Download [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol)
   and copy it to the **root** of your SD card.
3. Put [Swiss](https://github.com/emukidid/swiss-gc/releases/latest) on the card as
   `swiss-gc.dol`, plus a [`config.ini`](#configuration) and your games.

### Method 2: cubiboot flashed into the modchip (PicoBoot or PicoLoader)

Cubiboot lives in the Pico's firmware, so nothing but games and `swiss-gc.dol` needs to be
on the card.

**PicoBoot (Pico or Pico 2):**

1. Flash the **official PicoBoot firmware** first, if the Pico doesn't run it yet: hold
   the **BOOTSEL** button on the Pico while plugging it into your PC, and copy
   `picoboot_full_pico.uf2` (Pico) or `picoboot_full_pico2.uf2` (Pico 2) from the
   [PicoBoot releases](https://github.com/webhdx/PicoBoot/releases) to the USB drive that
   appears. Already running PicoBoot ≥ v0.4 (Pico 2 shipped with v0.5.0)? Skip this step.
2. Enter BOOTSEL mode again (the Pico reboots after step 1 — unplug, hold the button,
   replug) and copy
   [`cubiboot_picoboot_payload.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoboot_payload.uf2)
   — it replaces the gekkoboot payload embedded in the firmware with cubiboot, leaving
   the firmware itself untouched. One file for both boards, and the only file to
   re-flash on later updates.

> [!TIP]
> Pico misbehaving, or no idea what is flashed on it? In BOOTSEL mode, copy
> [`universal_flash_nuke.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/raw/main/tools/flash-nuke/universal_flash_nuke.uf2)
> to it first — it wipes the flash completely and drops the Pico straight back into
> BOOTSEL, ready for step 1. Works on both boards; see
> [tools/flash-nuke](tools/flash-nuke/README.md).

**PicoLoader:**

1. Flash your Pico with the `.uf2` from [PicoLoader](https://github.com/makeo/PicoLoader/releases/download/v1.3/picoloader.uf2).
2. Download [`cubiboot_picoloader_payload.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoloader_payload.uf2).
3. Hold the button on the RP2040 Pico while plugging it into your PC.
4. Copy the `.uf2` to the USB drive that appears; the Pico reboots running cubiboot.

**Either way:** put Swiss on your SD2SP2 / SD Gecko card as `swiss-gc.dol`, along with a
[`config.ini`](#configuration) and your games.

> [!WARNING]
> With this method every cubiboot update means opening the console and re-flashing the Pico.
> Method 1 is easier to live with.

### Method 3: GC Loader and CUBE-ODE

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

### Method 4: FlippyDrive

A FlippyDrive boots its own loader from its **internal flash**, so cubiboot replaces the
`cubeboot` entry in there. Games, `config.ini` and Swiss stay on the SD card as usual.

The drive loads it **by name**, so it has to end up called `cubeboot.dol`. Under any other
name it is just another file sitting in flash.

**Get the file onto the drive's SD card:**

1. Download [`flippydrive.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/flippydrive.dol)
   and **rename it to `cubeboot.dol`**. It is the same loader as `ipl.dol`; only the name the
   drive demands differs.
2. Copy it to the **root of the FlippyDrive's SD card**.

**Write it into the drive's flash, using Swiss:**

3. Power on **holding X** to reach the drive's [bootloader menu](https://docs.flippydrive.com/bootloader.html).
4. Choose **Boot Onboard DOL** → **swiss-gc**. (Swiss ships in the drive's flash, so this
   works before you have put Swiss on the card.)
5. In Swiss, turn on **Enable File Management** in the settings, if it is not on already —
   without it the menu in step 7 never appears.
6. Browse to `cubeboot.dol` on the SD card and highlight it.
7. Press **Z** to open *Manage File*, then **X** for *Copy*.
8. Choose **FlippyDrive Flash** as the destination device, then its root as the destination
   folder. Confirm overwriting the `cubeboot.dol` already there.
9. Reboot. The drive autoloads it and you land on the cubiboot menu.
10. Put Swiss on the SD card as `swiss-gc.dol`, plus a [`config.ini`](#configuration) and your
    games.

> [!IMPORTANT]
> Step 3 is what makes step 8 possible, and it is the step people skip. Booting normally
> leaves the drive's loader **holding the flash copy of `cubeboot.dol` open**, and an open
> file cannot be overwritten — the copy fails, sometimes without a clear error. Holding X
> hands control over without that file ever being opened.

> [!WARNING]
> **A FlippyDrive firmware update restores the stock `cubeboot`**, so cubiboot is gone and
> you have to redo this. The drive's own docs put it plainly: *"any custom DOL files might get
> erased during the firmware update process."* Nothing is lost from the SD card — only the
> copy inside the drive.

**Swiss and In-Game Reset on a FlippyDrive** work the same as anywhere else, from the SD
card: `swiss-gc.dol` in the card root, `apploader.img` in `swiss/patches/`. The drive's flash
also carries a Swiss of its own, and cubiboot will accept that one if the card has none — but
the `apploader.img` has to be ours, from the same release as the loader, so that one belongs
on the card.

> [!NOTE]
> The drive's bootloader menu also has a **`remote`** entry that serves the drive over FTP/SMB.
> If it reaches the flash, that is an easier way to drop the file in than steps 5–8. We haven't
> verified what it exposes, so the Swiss route above is the documented one.

### In-Game Reset

Optional, works with every method above.

1. Download [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip).
2. Extract it to the **root** of the SD card — this drops `apploader.img` into
   `swiss/patches/`.
3. In Swiss, go to **Settings → Global Game Settings (4/6)** and set **In-Game Reset** to
   **`Apploader`**, then *Save & Exit*. With any other value (`Disabled` / `Reboot`) Swiss
   never reads `apploader.img` — its own description of the option says "Apploader —
   Requires /swiss/patches/apploader.img".
4. Press **Z + A + START** in a game to return to the cubiboot menu.

## Updating

> [!IMPORTANT]
> `apploader.img` carries its own complete copy of the loader — that is how the reset combo
> hands control back without a cold boot. So if you set up In-Game Reset, **`apploader.img`
> has to be replaced on every update, together with the loader itself.** Replace only one of
> the two and the console runs two different versions of cubiboot.

Nothing warns you when they drift apart, which is what makes it worth knowing: a cold boot
lands on the new menu, In-Game Reset lands on the old one. The symptom is a fix or a new
setting that works fine until you reset out of a game, and then doesn't.

If you never installed `apploader.img`, there is nothing to keep in sync — replace the loader
and you are done.

| Installed with | Replace |
|---|---|
| [Method 1](#method-1-picoboot-or-picoloader-with-gekkoboot) | `ipl.dol` **and** `swiss/patches/apploader.img` |
| [Method 2](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) (PicoBoot) | re-flash `cubiboot_picoboot_payload.uf2`, **and** replace `swiss/patches/apploader.img` on the card |
| [Method 2](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) (PicoLoader) | re-flash `cubiboot_picoloader_payload.uf2`, **and** replace `swiss/patches/apploader.img` on the card |
| [Method 3](#method-3-gc-loader-and-other-odes) | `cubiboot.iso` **and** `swiss/patches/apploader.img` |
| [Method 4](#method-4-flippydrive) | re-flash the loader **inside the drive** (steps below), **and** replace `swiss/patches/apploader.img` on the card |

Both files come from the same release — mixing an `apploader.img` from one release with a
loader from another is the situation this section is about.

### Updating a FlippyDrive

The loader lives inside the drive, so there is no file on the card to swap — the new one has
to be written over the copy in flash, the same way it got there:

1. Download [`flippydrive.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/flippydrive.dol),
   **rename it to `cubeboot.dol`**, and copy it to the root of the drive's SD card, replacing
   any older one.
2. Power on **holding X** → **Boot Onboard DOL** → **swiss-gc**.
3. In Swiss, highlight `cubeboot.dol` on the SD card, press **Z** for *Manage File*, then **X**
   for *Copy*.
4. Destination device **FlippyDrive Flash**, destination folder its root; confirm the
   overwrite.
5. Reboot — the drive autoloads the new one.

Full detail, including the Swiss setting that has to be on first, is in
[Method 4](#method-4-flippydrive).

> [!IMPORTANT]
> Holding X in step 2 is what makes step 4 possible. Boot normally and the drive's loader is
> already **holding the flash copy open**, so the overwrite is refused: the update appears to
> work and the old version keeps booting. If a new release seems to change nothing on a
> FlippyDrive, this is why.

Also worth knowing: updating the **FlippyDrive's own firmware** restores the stock `cubeboot`
and wipes cubiboot out of flash, so redo the steps above afterwards. Your `config.ini`, games
and `swiss-gc.dol` on the SD card are never touched by any of this.

Re-extracting [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip)
handles both in one step for Method 1, but it also carries `config.ini` and will overwrite
yours — copy yours aside first, or take just those two files out of the zip.

To check what is actually installed, the two lines under the Cubiboot wordmark in the menu
name the build. Cold boot and In-Game Reset should show the same one.

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

; Render the menu anamorphic for a 16:9 TV (set the TV or GCVideo to Full/16:9):
; force_widescreen = 1
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
| `preboot_delay_ms` | milliseconds | `0` | Wait before the boot animation, for a TV to lock on |
| `postboot_delay_ms` | milliseconds | `0` | Hold the last frame after picking a game, before it boots |
| [`force_widescreen`](#widescreen-169) | `1` · `0` | `0` | Render the menu anamorphic for a 16:9 TV |

That is the whole list. Full reference: [docs/settings.md](docs/settings.md).

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
| `ode` | **Whichever ODE is installed** — resolved by asking the drive, so it covers both of the two below |
| `gcloader` (or `gcldr`) | The SD card **inside a [GC Loader](https://gcloaderhq.com/)**, or anything answering the same drive commands |
| `flippy`, `flippydrive` (or `fldrv`) | The SD card **inside a FlippyDrive** |

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

Cubiboot boots games by chainloading Swiss. That makes Swiss itself a special case: handing
it to Swiss would be asking Swiss to load a copy of itself, which resets the console to the
stock GameCube menu. Cubiboot avoids that by recognising Swiss and running it directly — but
it recognises it **by name**, so the name is what you have to get right.

**Name it so it starts with `swiss`.** Capitalisation does not matter, and anything after
the first five letters is ignored, so `Swiss v0.6r2073` works as well as `swiss`.

| How you keep Swiss | What has to start with `swiss` | Example |
|---|---|---|
| A `.dol` in any folder | the **filename** | `swiss-gc.dol`, `Swiss v0.6r2073.dol` |
| A [homebrew app](#homebrew-apps) folder | the **folder** name | `apps/Swiss v0.6r2073/default.dol` |
| A disc image | the **filename** | `Swiss v0.6r2073.iso` |

For the app folder, only `default.dol` inherits the folder's name — any other `.dol` sitting
in there is treated as a different program that happens to live beside Swiss.

Get the name wrong and Swiss is treated as an ordinary program: it gets handed to the Swiss
at your card root, and the console resets to the stock menu instead of starting. Nothing is
damaged, and renaming fixes it.

Two things worth knowing:

- **A Swiss app boots even with no `swiss-gc.dol` at the root.** Everything else needs that
  file, because everything else is booted through Swiss — but Swiss needs no chainloader. So
  a card that only carries Swiss as an app can still start it, which is what you want when
  that root file is what went missing.
- **Disc images do not need the name at all.** Every disc image boots through cubiboot's own
  apploader, so a Swiss `.iso` works whatever it is called. The naming rule matters for the
  `.dol` and the app folder, which are the forms that go through Swiss.

This is all separate from the `swiss-gc.dol` at the card **root**, which is the copy games
are launched with and has to be there regardless.

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

### Widescreen (16:9)

`force_widescreen = 1` renders the menu **anamorphic**: the picture is squeezed horizontally
in the signal, and a TV set to **Full/16:9** stretches it back into correct proportions —
the same trick GameCube games with a 16:9 option use. Everything scales together: boot
animation, grid, banners, info panel.

Two things to know:

- **Set the TV (or GCVideo) to Full/16:9.** On a 4:3 screen, or a TV left in 4:3 mode, the
  menu just looks horizontally squeezed.
- The same 640 pixels now cover a wider image, so some effective horizontal resolution is
  lost. That trade-off is inherent to anamorphic output.

Off by default. This only affects cubiboot's own menu — what a game does with the screen is
between the game and Swiss. Ported from
[cubeboot PR #57](https://github.com/OffBroadway/cubeboot/pull/57) by BenHetherington.

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
- The banner layouts may crash in folders over 128 files — see
  [Large folders and the banner pool](#large-folders-and-the-banner-pool).

## Building

### CI (recommended)

Every push builds `ipl.dol` + `apploader.img` + `cubiboot.iso` + `config.ini` +
`cubiboot_picoloader_payload.uf2` + `cubiboot_picoboot_payload.uf2` and uploads them as
artifacts. Pushing a `v*` tag publishes a GitHub Release with those files plus
`EXTRACT_TO_ROOT.zip`. See [.github/workflows/ci.yml](.github/workflows/ci.yml).

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
- **`cubiboot_picoloader_payload.uf2`** — the [PicoLoader](https://github.com/makeo/PicoLoader)
  firmware with `cubiboot.iso` embedded as the payload, replicating makeo's PicoLoader
  converter. See [.ci/make_picoloader_uf2.py](.ci/make_picoloader_uf2.py).
- **`cubiboot_picoboot_payload.uf2`** — the [PicoBoot](https://github.com/webhdx/PicoBoot)
  payload-only update. The payload is `entry/entry.dol` (the stage-1 stub linked at
  `0x81300000` — the only DOL PicoBoot can inject), scrambled with the BS2 bootrom
  scrambler and stamped with PicoBoot's `IPLBOOT `/`PICO` payload framing, replicating
  PicoBoot's `tools/process_ipl.py`. It targets flash `0x80000`, where official PicoBoot
  firmware ≥ v0.4 streams the payload from — which is why the official firmware has to be
  flashed first. See [.ci/make_picoboot_uf2.py](.ci/make_picoboot_uf2.py).

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
