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

## What's new in v1.9.7

* **Long titles scroll.** A title wider than the info box holds still for a moment, then scrolls sideways so the whole name can be read — in the game list and on the Game Play screen. Names that fit never move, and Japanese titles scroll cleanly (the stepping never splits a Shift-JIS character). Two new `config.ini` keys tune it: [`text_scroll`](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/settings.md#text_scroll) — `on` (default, 2-second delay), `off`, or the delay in seconds (the one key where `0` and `1` mean seconds, not switches) — and [`big_titles_scroll_speed`](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/settings.md#big_titles_scroll_speed), the pace in frames per character (`1` fastest, default `10`). The old "keep filenames under 28 characters" advice is retired.

* **The description scrolls by hand with L and R.** One character per press, hold to keep scrolling, bounded at both ends. A banner description with embedded line breaks is joined into one stream while scrolling, so every line can be read — the info box's single visible row used to hide the rest of a multi-line description outright.

* **`remember_last_game` now comes back to apps too.** A homebrew app (`default.dol` + `opening.bnr`) or a plain `.dol` you booted is re-selected on the next boot exactly like a game, in whatever folder it lives. A physical-disc launch is skipped — there is no list entry to come back to — so it no longer shadows the most recent card entry.

* **The docs were restructured.** The [README](https://github.com/DarthMotzkus/cubiboot-new-ui#readme) is now a quick guide; the four installation methods, In-Game Reset and updating moved to [docs/INSTALL.md](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md), and every `config.ini` key with its full behavior notes lives in [docs/settings.md](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/settings.md). `force_progressive` is now documented there too: menu in 480p, IPL 1.1/1.2 only, safely ignored on IPL 1.0.

**Full Changelog:** [v1.9.6...v1.9.7](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.9.6...v1.9.7)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
