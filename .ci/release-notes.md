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

## What's new in v1.7.1

One bug fix, confirmed on real hardware.

* **Fixed: apps showed a blank banner and no title/description when they lived in the same folder as the last-played game.** With `remember_last_game` on, that folder loads its banners in the background so the menu can appear instantly — and the background loader only knew about games, so any app (`<dir>/default.dol` + `opening.bnr`) in the folder never had its `opening.bnr` read. Apps now load, scroll and release their banners exactly like games do, everywhere. Folders without the last-played game, or setups with the option off, were never affected.

**Full Changelog:** [v1.7.0...v1.7.1](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.7.0...v1.7.1)
