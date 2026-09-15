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

## What's new in v1.12.0

**Who needs to update: anyone whose card holds NKit-compressed games (`*.nkit.iso`).** The fix is in the menu itself, which ships inside every artifact, so all install methods get it the same way — [Method 1](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md) and [Method 2](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) alike, PicoBoot, PicoLoader, GC Loader and FlippyDrive included. If none of your games are NKit images, this release changes nothing you can see.

* **NKit games no longer carry `.nkit` in their title.** `Resident Evil 4.nkit.iso` showed up in the grid as `Resident Evil 4.nkit`, while the very same game as a plain `.iso` showed as `Resident Evil 4`. The leftover suffix is now stripped, so both read the same. Only a `.nkit` sitting immediately before the real extension is taken — a name like `Ver.1.2.iso` keeps its `.2`, and a file called nothing but the suffix keeps its title rather than going blank. This is **display only**: which files show up in the grid, how they are sorted, which banner each one gets and what actually boots are all untouched.

* **Under the hood: the release build is now pinned and checkable.** Every artifact here is built from a toolchain image pinned by digest instead of one assembled fresh on each run, and the **Build:** line just below records which swiss-gc commit `apploader.img` embeds. A new CI job can rebuild any published tag with that same pinned toolchain and diff the result against the files attached to the release — so "does this download match the source" is answered with bytes. Nothing about how the artifacts behave on the console changed.

**Full Changelog:** [v1.11.3...v1.12.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.11.3...v1.12.0)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
