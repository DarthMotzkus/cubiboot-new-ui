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

## What's new in v1.11.0

* **The disc screen picks up a disc you put in after opening it.** Pressing **Z** used to read the drive once and stand by whatever it found, so inserting a disc left the screen saying "Please insert a NINTENDO GAMECUBE DISC" with the disc already in the tray — the only way through was leaving and coming back. It now reads again every time the lid closes. Two fixes came with it: an error from a drive that is still spinning up is no longer taken as an answer (a tray that was just closed never made the old ~1.4s ceiling), and **B** leaves at any point instead of being refused for the whole of a read. **START** now boots only a disc the screen has actually read.

* **The bottom prompt bar, corrected.** The row of button prompts on the BIOS screens loses the `...` between each glyph and its label, `Cancel` reads **Back**, and **A comes before B** on every screen. The game list gains the two prompts it never had — the animated analog stick and the A — drawn from the IPL's own elements at the positions its other screens use, so the two rows read identically. The pulse and the stick's animation are taken from BS2 rather than approximated, which is what puts the A, the Z pill and the stock B in step and gives the stick its cross-fade instead of a blink.

**Full Changelog:** [v1.10.0...v1.11.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.10.0...v1.11.0)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
