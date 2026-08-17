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

## What's new in v1.9.5

* **New: the disc screen is back, and it reads out-of-region discs.** Press **Z** in the menu and the console's own Game Play screen opens, with the disc's cover art and **PRESS START** exactly as a stock GameCube shows them — including for **imports**, which the stock screen refuses to read. **B** returns to the menu. The screen itself is the console's; what changed is who reads the disc. The stock flow only reaches a banner by running the apploader first, which loads the game into the memory cubiboot occupies and takes the console down mid-read, so cubiboot reads `opening.bnr` off the disc itself instead — a step per frame, so "Reading disc…" animates over the drive's spin-up rather than freezing on the button press. The region check lives in the machine being skipped, which is why an import boots. Built on the disc-screen infrastructure from [PR #8](https://github.com/DarthMotzkus/cubiboot-new-ui/pull/8) by [@Jpe230](https://github.com/Jpe230), whose reverse-engineering of the stock screen across all seven IPL revisions is what made this possible. See [Booting a physical disc](https://github.com/DarthMotzkus/cubiboot-new-ui#booting-a-physical-disc).

* **New: physical discs boot through Swiss** (`swiss_on_dvd_boot`, on by default). That is what carries **In-Game Reset** for disc games — the reset combo returns to the cubiboot menu instead of the stock IPL — and what lets an out-of-region disc boot at all, since nothing on that path consults the console's region. Set `swiss_on_dvd_boot = off` to hand the disc to the console's own apploader instead: the stock boot, without either. Games on the card are unaffected; they always went through Swiss.

* **New: the menu header names the device you are reading from.** At the card root the header reads **SD2SP2**, **SLOT A SD**, **SLOT B SD** or **ODE SD** instead of the product name, so a console with more than one card says which one the list came from. Inside a folder it still shows the folder name.

* **Every `config.ini` switch now reads `on`/`off`.** `swiss_on_dvd_boot`, `remember_last_game`, `force_widescreen` and the rest all take the same spellings — `1`/`0`, `yes`/`no` and `true`/`false` work too. Existing configs keep working and every default is unchanged. A value that is none of those now keeps the default instead of being read as off, so a typo can no longer turn a setting off silently.

* **Fixed: booting a physical disc could fail for no reason.** The drive is reset on the way into a disc boot, which spins the disc back down; a single read landing in that window reported the disc as unreadable and dropped the boot. It now waits the spin-up out.

* **Fixed: NTSC 1.2 (DOL-001) disc restart.** One address in the stock-screen map was off by four bytes on that revision — verified against a real 1.2-001 dump, along with the other six revisions.

**Full Changelog:** [v1.9.0...v1.9.5](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.9.0...v1.9.5)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui#updating).
