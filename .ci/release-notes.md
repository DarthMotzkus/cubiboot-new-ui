## What's new in v1.6.3

Game lists load faster, and the settings list now only contains options this fork actually stands behind.

* **Folders of games populate noticeably faster** — two changes underneath, both about the same thing. Finding a file meant reading through the folder's index from the beginning every single time, so a folder of 60 games read the same index off the card 60-plus times, in tiny pieces the card is slow at. Cubiboot now remembers what it has already read, so those repeat trips stop happening. Separately, reading a game's banner used to wipe the folder index that had just been read — they were sharing one scratch area — which forced it to be fetched again immediately; they now have their own. Leaving a folder and coming back is quicker too.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

* **How to actually see it: hold A at power-on to skip the boot animation.** With the animation playing, the loading finishes behind it and this release looks identical to the last one — those few seconds were hiding the whole cost. Skip the animation and v1.6.2 still fills banners in while this build has them ready. It is also how the change was confirmed on hardware.

* **Four settings removed from the documentation and the bundled `config.ini`** — `force_progressive`, `force_swiss_default`, `disable_mcp_select` and `show_watermark`, along with the inherited-from-upstream keys that never worked here (`cube_logo`, `button_b`, `default_program`). Listing an option is a promise that it behaves as described, and those had not been verified in this fork. Nothing is taken away from a card that already sets one — the loader still reads them exactly as before — they are simply no longer presented as things to try. What remains is what has been used and tested.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

* **The two boot delays are now listed in the README's options table**, which they had never been, and moved into a section of their own in `config.ini`. They shared a heading with `force_progressive`, and with that gone the heading no longer described them.

Still to come: re-entering a folder rebuilds its list from scratch. The banners now come mostly from memory, which is the half that got faster, but every game is reopened to rebuild the list. Remembering folders you have already visited is the next piece of work.

<!--
STANDING BLOCK -- keep the "Updating from an earlier release" section below in every release,
verbatim. It is not tied to what changed in any one version.

Why it earns the space: apploader.img contains a whole second copy of the loader, so anyone
who replaces only the loader keeps reaching the previous version through In-Game Reset, and
nothing on the console says so. It reads as a fix that works until you reset out of a game.
Only the per-version notes above and the compare link below need rewriting each release.
-->

### Updating from an earlier release

`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui#updating).

**Full Changelog:** [v1.6.2...v1.6.3](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.6.2...v1.6.3)
