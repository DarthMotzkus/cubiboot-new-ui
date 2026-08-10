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

## What's new in v1.8.0

* **New: PicoBoot `.uf2` install method.** Three new release files for [PicoBoot](https://github.com/webhdx/PicoBoot) owners: `cubiboot_picoboot_pico.uf2` (Pico) and `cubiboot_picoboot_pico2.uf2` (Pico 2) are the official PicoBoot v0.5.0 firmware with cubiboot embedded — flash one and the SD card needs no `ipl.dol` or gekkoboot. On later updates flash only the small `cubiboot_picoboot_payload.uf2`, which swaps the embedded cubiboot without re-flashing the firmware (works on both boards). See [Method 2](https://github.com/DarthMotzkus/cubiboot-new-ui#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader).
* **Docs: In-Game Reset now spells out the required Swiss setting** — **Settings → Global Game Settings (4/6) → In-Game Reset = `Apploader`**. With any other value Swiss never reads `apploader.img`.

**Full Changelog:** [v1.7.1...v1.8.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.7.1...v1.8.0)
