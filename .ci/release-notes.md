<!--
STANDING BLOCK -- the quoted block immediately below goes FIRST in every release, verbatim,
before the "What's new" heading. Do not move it to the end and do not reword it.

The ">>" prefix is deliberate: it renders as a quote, which sets it apart from the release's
own notes as a standing notice rather than something that changed in this version.

Why it earns the top of the page: apploader.img contains a whole second copy of the loader,
so anyone who replaces only the loader keeps reaching the previous version through In-Game
Reset, and nothing on the console says so. It reads as a fix that works until you reset out
of a game -- which is why it belongs where nobody can miss it.

Only the "What's new" notes and the compare link at the bottom get rewritten each release.
-->

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui#updating).

## What's new in v1.7.0

cubiboot now runs natively on a **FlippyDrive** — tested on real hardware.

* **New: native FlippyDrive support** — the drive boots cubiboot itself, from its internal flash, and the menu reads games straight off the drive's SD card. No EXI card reader needed. This release ships a new artifact, **`flippydrive.dol`** — the same binary as `ipl.dol`, named so it can be told apart in this listing. Rename it to `cubeboot.dol` and flash it into the drive: [Method 4](https://github.com/DarthMotzkus/cubiboot-new-ui#method-4-flippydrive) in the README has the step-by-step.
* **`device_order` learns the new names** — `flippy` / `flippydrive` name the drive's SD card, and `ode` now means *whichever ODE is installed*: there is only one drive connector, so cubiboot asks the drive what it is and `ode` resolves to a GC Loader or a FlippyDrive accordingly. Existing configs keep working unchanged.
* Files cubiboot looks for in the drive's flash are also searched on the SD card — so **In-Game Reset works with `apploader.img` in `swiss/patches/` on the card**, where every install doc puts it, without flashing it into the drive.

Two things FlippyDrive owners should know, both in the README: **updating cubiboot means re-flashing it inside the drive** (plus the usual `apploader.img` on the card — see [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui#updating)), and **a FlippyDrive firmware update restores the stock `cubeboot`**, so cubiboot has to be re-flashed after one.

Still to come: re-entering a folder rebuilds its list from scratch. The banners now come mostly from memory, but every game is reopened to rebuild the list. Remembering folders you have already visited is the next piece of work.

**Full Changelog:** [v1.6.4...v1.7.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.6.4...v1.7.0)
