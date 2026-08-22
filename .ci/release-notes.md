<!--
STANDING BLOCK -- the quoted block at the very BOTTOM of this file goes LAST in every
release, verbatim, after the compare link. Do not move it back to the top and do not
reword it.

The ">>" prefix is deliberate: it renders as a quote, which sets it apart from the release's
own notes as a standing notice rather than something that changed in this version.

Why the bottom: it still matters (apploader.img contains a whole second copy of the loader,
so anyone who replaces only the loader keeps reaching the previous version through In-Game
Reset, and nothing on the console says so) — but at the top it read as this version's
headline and pushed the actual news below the fold. As a closing notice it stays visible
without stealing the release.

Only the "What's new" notes and the compare link get rewritten each release.
-->

## What's new in v1.9.8

* **The title marquee is now a ticker.** A long title runs left, a gap follows the tail, and the head re-enters right behind it; the loop parks only once the beginning is back at the box edge, then holds and goes again. The resting frame is always the readable start of the name — v1.9.7 parked on the tail and could leave the last letter clipped. The Game Play screen also got its own width limit (35 characters, hardware-measured), so titles that fit there no longer scroll for nothing.

* **L and R scroll on analog travel.** The description scroll now reacts to the triggers' analog pressure, not just the full-press digital click — pads whose triggers never reach the click, or that lack it entirely, work now. Hysteresis keeps a worn trigger from flapping, and holding still auto-repeats as before.

* **The header shows a real Z button.** "Load Disc (Z)" is now a purple GameCube-Z pill (the pad's #6B5CB1, drawn with the Z glyph from the BIOS's own font) pulsing next to "Load Disc", matching the stock A/B button pills.

* **The yellow "BETA TEST" overlay is gone.** It sat on the header row and got in the way. The build id is still stamped on the boot banner and readable inside the binary; the `show_watermark` key is currently inert.

* **Install guides now say to wipe the Pico first.** Leftovers of a previous firmware or payload are the classic cause of a double boot — the console visibly passing through two loaders — so `universal_flash_nuke.uf2` is now the recommended first step of every PicoBoot/PicoLoader install, in the [README](https://github.com/DarthMotzkus/cubiboot-new-ui#installation) and the [install guide](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#wipe-the-pico-first-recommended).

**Full Changelog:** [v1.9.7...v1.9.8](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.9.7...v1.9.8)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
