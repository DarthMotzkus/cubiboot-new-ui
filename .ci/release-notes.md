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

## What's new in v1.6.4

`default_folder` and `remember_last_game` now work on SD Gecko card readers in the memory card slots.

* **Fixed: the starting folder was ignored on memory card slot readers** — with the SD card in slot A or B, `default_folder` always fell back to the card root and `remember_last_game` never pre-selected anything, while the exact same `config.ini` worked on an SD2SP2. The folder itself was fine: it opened normally from the menu. The cause was *when* the folder was chosen, not *where*: the choice ran so early in the boot that on the memory card slots the console's BIOS still owns the port, the very first card access failed, and the menu silently fell back to the root — every boot. The SD2SP2 sits on a port the BIOS never touches, which is why it was immune. The decision now happens a moment later, on the same path that loads the game list — the one access that demonstrably works — with no retries, no waiting, and no change to how fast anything loads.
    _Reported on Discord by a slot B user, who also confirmed the fix on real hardware._

Still to come: re-entering a folder rebuilds its list from scratch. The banners now come mostly from memory, but every game is reopened to rebuild the list. Remembering folders you have already visited is the next piece of work.

**Full Changelog:** [v1.6.3...v1.6.4](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.6.3...v1.6.4)
