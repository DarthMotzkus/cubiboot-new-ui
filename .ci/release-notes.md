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

## What's new in v1.12.4

* **Games that share a game ID now show their own banners.** A ROM hack, a translation or a re-bannered dump keeps the game ID of the original disc. In the grid, all of them showed whichever banner loaded first. The banner cache was keyed on the disc header, which these files share. It is now keyed on the file itself, so every file shows the banner inside it, just like Swiss. The name, maker and description shown under the grid follow the right banner too.

* **Banners no longer disappear after browsing for a while.** Each app folder (`default.dol` + `opening.bnr`) kept one of the menu's 128 banner slots every time you left the folder it sits in. Opening the disc screen with Z counted as leaving too. After enough round trips no slot was left: banners came up blank, or never filled in while scrolling, until the console was restarted. Leaving a folder now frees every banner it loaded, and the slots are reset on every folder change.

* **Coming back from the disc screen is instant.** Pressing Z and then B used to throw away the folder you were in and read every game on the card again, which took a long time on large folders. The list is now kept as it was. Going into a folder or up to its parent still reads it fresh.

* **Fixed a way for the game list to freeze.** The menu keeps a copy of each banner in the console's audio RAM and waited without any limit for each copy to finish. If one copy never reported back, the list froze and so did the menu. The wait now gives up after half a second. The menu then stops using that copy for the rest of the session and reads banners straight from the card.

**Full Changelog:** [v1.12.2...v1.12.4](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.12.2...v1.12.4)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
