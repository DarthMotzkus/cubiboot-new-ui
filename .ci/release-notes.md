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

The "**Build:**" line (swiss-gc commit behind apploader.img, toolchain image) is NOT in this
file: ci.yml generates it at release time and inserts it just before the standing block.
Do not add it by hand.
-->

## What's new in v1.12.2

**Who needs to update: anyone who plays Japanese discs from the disc screen (press Z).** The fix is in the menu itself, which ships inside every artifact, so all install methods get it the same way — [Method 1](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md) and [Method 2](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) alike. On a GC Loader style ODE there is no disc screen, and if you never boot Japanese discs from the drive, this release changes nothing you can see.

* **Japanese disc names now read correctly on the disc screen.** A Japanese disc showed its name, maker and description as garbage (`ƒoƒ‹ƒ…`) on the Game Play screen, and again on the main-menu panel the boot animation passes through after START — while the very same names read fine in the grid. Both screens are drawn by the console's own BIOS, which cubiboot runs in English on every NTSC console, Japanese ones included, and the BIOS only decodes Japanese text while its language is Japanese. Cubiboot now switches to the Japanese font for just that banner text, the same switch the grid has always made. The screens' own labels ("Game Play", PRESS START) keep the menu's font, discs from other regions are untouched, and all seven IPL revisions, NTSC and PAL, are covered.

**Full Changelog:** [v1.12.0...v1.12.2](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.12.0...v1.12.2)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
