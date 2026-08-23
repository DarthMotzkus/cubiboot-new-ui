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

## What's new in v1.10.0

* **Custom boot logo.** [`cube_logo`](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/settings.md#cube_logo) swaps the "GAMECUBE" text under the cube in the boot animation for your own art. The setting existed in the original cubeboot but has been dead since its PNG decoder was removed upstream; it is now revived on a decoder-free diet — the console reads a raw RGBA8 352×40 file, so a bad file can never break the boot (wrong size just falls back to the stock text). Set `cube_logo = /logo.raw` in `config.ini`.

* **A converter that runs in your browser.** The `.raw` is produced by the new [cube logo converter](https://github.com/DarthMotzkus/cubiboot-new-ui/tree/main/tools/cube-logo-converter): open `index.html` in any browser on Windows, Linux or macOS — nothing to install, nothing uploaded — drop an image in, check the live preview and download the file. It resizes with a proper Lanczos filter and, by default, places the art in the exact box the stock "GAMECUBE" letters occupy (measured from the BIOS texture itself — art centered on the full canvas shows up visibly shifted right on screen, because the stock art leaves room for the ™ sign). A command-line `png2cubelogo.py` with the same geometry ships next to it.

**Full Changelog:** [v1.9.8...v1.10.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.9.8...v1.10.0)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
