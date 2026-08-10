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

## What's new in v1.9.0

* **New: 16:9 widescreen menu.** Set `force_widescreen = 1` in `config.ini` and the menu renders **anamorphic**: squeezed in the signal, proportioned again once the TV (or GCVideo) is set to **Full/16:9** — the same trick GameCube games with a 16:9 option use. The whole UI scales together: boot animation, grid, banners, info panel. On a 4:3 screen it just looks squeezed, so leave it off (the default) there; the trade-off is some effective horizontal resolution, inherent to anamorphic output. Ported from [cubeboot PR #57](https://github.com/OffBroadway/cubeboot/pull/57) by BenHetherington. See [Widescreen (16:9)](https://github.com/DarthMotzkus/cubiboot-new-ui#widescreen-169).

**Full Changelog:** [v1.8.0...v1.9.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.8.0...v1.9.0)
