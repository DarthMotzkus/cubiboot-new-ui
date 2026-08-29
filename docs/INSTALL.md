# Installing cubiboot

Every way of getting cubiboot onto a console, in full detail. The
[README](../README.md#installation) has the short version; this page is the reference.

- [Files on the card](#files-on-the-card)
- [Wipe the Pico first (recommended)](#wipe-the-pico-first-recommended)
- [Method 1: PicoBoot or PicoLoader with gekkoboot](#method-1-picoboot-or-picoloader-with-gekkoboot)
- [Method 2: cubiboot flashed into the modchip (PicoBoot or PicoLoader)](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader)
- [Method 3: GC Loader and CUBE-ODE](#method-3-gc-loader-and-cube-ode)
- [Method 4: FlippyDrive](#method-4-flippydrive)
- [In-Game Reset](#in-game-reset)
- [Updating](#updating)

Pick the method that matches your console:

| Your setup | Use |
|---|---|
| PicoBoot or PicoLoader modchip | [Method 1](#method-1-picoboot-or-picoloader-with-gekkoboot) — recommended, updates by swapping files on the SD card |
| PicoBoot or PicoLoader, and you want no loader file on the card | [Method 2](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) |
| GC Loader or CUBE-ODE, no modchip | [Method 3](#method-3-gc-loader-and-cube-ode) |
| FlippyDrive | [Method 4](#method-4-flippydrive) — the drive boots cubiboot itself |

## Files on the card

Whatever the method, these files always live at the **root** of the card cubiboot reads from:

```
/ipl.dol                        (only for installation Method 1)
/config.ini
/swiss-gc.dol
/swiss/patches/apploader.img    (only if you want In-Game Reset)
```

You will need [Swiss](https://github.com/emukidid/swiss-gc/releases/latest): cubiboot
chainloads it to actually boot games. Rename its `.dol` to **`swiss-gc.dol`**.

Your games can live anywhere, including subfolders — see
[`default_folder`](settings.md#default_folder).

> [!NOTE]
> **On a FlippyDrive** the last two lines work differently. The drive carries a Swiss of its
> own in its **internal flash**, and cubiboot uses that copy **first** — `swiss-gc.dol` on the
> card is only a precaution for a flash that lost its own. And `apploader.img` is not used at
> all: In-Game Reset on a FlippyDrive goes through Swiss's **Reboot** option instead
> ([details](#in-game-reset)). The `config.ini` also needs
> [`swiss_on_dvd_boot = off`](settings.md#swiss_on_dvd_boot) there — see
> [Method 4](#method-4-flippydrive).

## Wipe the Pico first (recommended)

Before flashing anything in Method 1 or 2, wipe the Pico: hold **BOOTSEL** while plugging
it into your PC and copy
[`universal_flash_nuke.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/raw/main/tools/flash-nuke/universal_flash_nuke.uf2)
to the USB drive that appears. It erases the whole flash and drops the Pico straight back
into BOOTSEL, ready for the real firmware.

Why it matters: a Pico that was flashed before keeps whatever lived there — an old
gekkoboot, another payload, a different firmware — and those leftovers can survive next to
the new install. The classic symptom is a **double boot** (the console visibly passes
through two loaders back to back), or the old loader still coming up as if the new flash
never happened. Wiping first guarantees the next `.uf2` is the only thing on the chip.
Works on Pico and Pico 2; details in [tools/flash-nuke](../tools/flash-nuke/README.md).

## Method 1: PicoBoot or PicoLoader with gekkoboot

**Recommended.** Updating cubiboot later is just replacing files on the SD card — no
disassembly. See [Updating](#updating) for which ones.

1. [Wipe the Pico](#wipe-the-pico-first-recommended) if it has ever been flashed before.
2. Flash your Pico with the `.uf2` for the chip you actually have — they are different
   boards, not two names for one thing: [PicoBoot](https://github.com/webhdx/PicoBoot) for a
   PicoBoot install, or
   [PicoLoader+Gekkoboot](https://github.com/makeo/PicoLoader/releases/download/v1.3/picoloader_gekkoboot.uf2)
   for a PicoLoader one.
3. Download [`ipl.dol`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/ipl.dol)
   and copy it to the **root** of your SD card.
4. Put [Swiss](https://github.com/emukidid/swiss-gc/releases/latest) on the card as
   `swiss-gc.dol`, plus a [`config.ini`](settings.md) and your games.

## Method 2: cubiboot flashed into the modchip (PicoBoot or PicoLoader)

Cubiboot lives in the Pico's firmware, so nothing but games and `swiss-gc.dol` needs to be
on the card.

**PicoBoot (Pico or Pico 2):**

1. [Wipe the Pico](#wipe-the-pico-first-recommended) if it has ever been flashed before —
   leftovers of a previous install are what cause a double boot.
2. Flash the **official PicoBoot firmware**: hold the **BOOTSEL** button on the Pico while
   plugging it into your PC, and copy `picoboot_full_pico.uf2` (Pico) or
   `picoboot_full_pico2.uf2` (Pico 2) from the
   [PicoBoot releases](https://github.com/webhdx/PicoBoot/releases) to the USB drive that
   appears. Already running a freshly-flashed PicoBoot ≥ v0.4 (Pico 2 shipped with
   v0.5.0)? Skip this step.
3. Enter BOOTSEL mode again (the Pico reboots after step 2 — unplug, hold the button,
   replug) and copy
   [`cubiboot_picoboot_payload.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoboot_payload.uf2)
   — it replaces the gekkoboot payload embedded in the firmware with cubiboot, leaving
   the firmware itself untouched. One file for both boards, and the only file to
   re-flash on later updates.

**PicoLoader:**

1. [Wipe the Pico](#wipe-the-pico-first-recommended) if it has ever been flashed before, so
   leftovers of a previous install can't shadow the new one.
2. Download [`cubiboot_picoloader_payload.uf2`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot_picoloader_payload.uf2)
   — it already contains the full [PicoLoader](https://github.com/makeo/PicoLoader)
   firmware with cubiboot embedded as the payload, so there is nothing to flash before it.
3. Hold the button on the RP2040 Pico while plugging it into your PC.
4. Copy the `.uf2` to the USB drive that appears; the Pico reboots running cubiboot.

> [!NOTE]
> PicoLoader boots the payload through the stock IPL (like a disc), which used to add the
> factory boot animation before cubiboot's own — two animations back to back. The payload
> now patches the factory animation out, so a **single** (cubiboot) animation is the
> expected boot. Holding **A** at power-on shows the factory animation instead; on an IPL
> revision the patch does not know, both animations still play — harmless, but worth
> reporting.

**Either way:** put Swiss on your SD2SP2 / SD Gecko card as `swiss-gc.dol`, along with a
[`config.ini`](settings.md) and your games.

> [!WARNING]
> With this method every cubiboot update means opening the console and re-flashing the Pico.
> Method 1 is easier to live with.

## Method 3: GC Loader and CUBE-ODE

`cubiboot.iso` is a bootable GameCube disc image that simply *is* the cubiboot loader — no
modchip needed.

1. Download [`cubiboot.iso`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/cubiboot.iso)
   and copy it onto your [GC Loader](https://gcloaderhq.com/)'s storage, in the folder you
   boot images from.
2. Boot `cubiboot.iso` from the GC Loader menu — it lands on the cubiboot menu.
3. Choose where the games come from:
   - **The ODE's own SD card** (no second reader): put `swiss-gc.dol` and a `config.ini`
     containing `device_order = gcldr` in the **root of that same card**. See
     [`device_order`](settings.md#device_order).
   - **An SD card adapter** (SD2SP2 / SD Gecko): nothing to set — card readers come first by
     default. Set the adapter's card up as usual.

## Method 4: FlippyDrive

A FlippyDrive boots its own loader from its **internal flash**, so cubiboot replaces the
`cubeboot` entry in there. Games, `config.ini` and Swiss stay on the Flippydrive SD card as usual.

The drive loads it **by name**, so it has to end up called `cubeboot.dol`. Under any other
name it is just another file sitting in flash.

> [!IMPORTANT]
> **Update the FlippyDrive firmware first: cubiboot needs 1.4.6-pre-release or newer.** On
> older firmware the menu comes up but **games do not load**. Firmware updates live at
> [flippydrive.com/updates](https://flippydrive.com/updates) — and since a firmware update
> restores the stock `cubeboot`, do it **before** flashing cubiboot in, not after.

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
10. Put a [`config.ini`](settings.md) and your games on the SD card, and make sure it has
    [`swiss_on_dvd_boot = off`](settings.md#swiss_on_dvd_boot) — the Swiss disc boot does not
    work on a FlippyDrive, and with it on (the default) a physical disc boots to a black
    screen. `swiss-gc.dol` on the card is **optional** here: cubiboot boots games with the
    Swiss already in the drive's **flash**, and only falls back to a `swiss-gc.dol` at the
    card root if the flash copy is missing. Keeping one there anyway is a cheap precaution.

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

**Swiss and In-Game Reset on a FlippyDrive** are simpler than anywhere else. Swiss comes from
the drive's **flash** — cubiboot prefers that copy, and a `swiss-gc.dol` at the root of the
FlippyDrive's SD card is only the fallback. And In-Game Reset needs **no `apploader.img`**:
the drive autoloads cubiboot from flash on every reboot, so a plain reboot already lands back
on the menu. Set Swiss's **In-Game Reset** to **`Reboot`** (Settings → Global Game Settings) —
cubiboot passes that automatically for games it boots, so the setting only matters for games
you start from inside Swiss yourself.

**Physical discs are the one exception**: on a FlippyDrive they boot natively, not through
Swiss. Keep [`swiss_on_dvd_boot = off`](settings.md#swiss_on_dvd_boot) in `config.ini` — with
it on, pressing START on a disc ends on a black screen, because Swiss refuses to take over
the optical drive while a FlippyDrive is present. The native boot is still region-free.

> [!NOTE]
> The drive's bootloader menu also has a **`remote`** entry that serves the drive over FTP/SMB.
> If it reaches the flash, that is an easier way to drop the file in than steps 5–8. We haven't
> verified what it exposes, so the Swiss route above is the documented one.

## In-Game Reset

Optional, works with every method above. The reset combo is **Z + A + START** in a game, and
it returns to the cubiboot menu. How it gets there depends on the hardware:

**FlippyDrive ([Method 4](#method-4-flippydrive)): nothing to install.** The drive autoloads
cubiboot from its flash on every reboot, so a plain reboot already lands on the menu —
cubiboot tells Swiss to use its **Reboot** IGR automatically for every game it boots. No
`apploader.img` anywhere. (If you also start games from inside Swiss itself, set Swiss's
**In-Game Reset** to **`Reboot`** in **Settings → Global Game Settings (4/6)** so those get
it too. The `Apploader` option does **not** work on a FlippyDrive.)

**Everything else (Methods 1–3):**

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
and you are done. A **FlippyDrive** never has one: its In-Game Reset goes through a plain
reboot, so on that hardware the loader in the drive's flash is the only thing to replace.

| Installed with | Replace |
|---|---|
| [Method 1](#method-1-picoboot-or-picoloader-with-gekkoboot) | `ipl.dol` **and** `swiss/patches/apploader.img` |
| [Method 2](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) (PicoBoot) | re-flash `cubiboot_picoboot_payload.uf2`, **and** replace `swiss/patches/apploader.img` on the card |
| [Method 2](#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) (PicoLoader) | re-flash `cubiboot_picoloader_payload.uf2`, **and** replace `swiss/patches/apploader.img` on the card |
| [Method 3](#method-3-gc-loader-and-cube-ode) | `cubiboot.iso` **and** `swiss/patches/apploader.img` |
| [Method 4](#method-4-flippydrive) | re-flash the loader **inside the drive** (steps below) — no `apploader.img` involved |

Both files come from the same release — mixing an `apploader.img` from one release with a
loader from another is the situation this section is about.

Re-extracting [`EXTRACT_TO_ROOT.zip`](https://github.com/DarthMotzkus/cubiboot-new-ui/releases/latest/download/EXTRACT_TO_ROOT.zip)
handles both in one step for Method 1, but it also carries `config.ini` and will overwrite
yours — copy yours aside first, or take just those two files out of the zip.

To check what is actually installed, the two lines under the Cubiboot wordmark in the menu
name the build. Cold boot and In-Game Reset should show the same one.

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
