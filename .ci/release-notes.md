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

## What's new in v1.11.3

**Who needs to update: only [Method 2](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader) installs with cubiboot flashed directly into the Pico as the PicoLoader payload `.uf2`** — and even there only if the double animation bothers you; it is purely cosmetic. **Method 1 (gekkoboot + `ipl.dol`), PicoBoot in either method, GC Loader and FlippyDrive are not affected** — no other file changed in this release, so there is nothing to update on those setups.

* **PicoLoader flash install: one boot animation instead of two.** The payload now skips the factory boot animation, so the console goes straight to cubiboot's own. Holding **A** at power-on still shows the factory one; on an IPL revision the patch does not know, both animations play as before. Hardware-validated.

* **Flashing the PicoLoader payload no longer aborts the copy midway.** The file carries data for both Pico models, and the old layout made a Pico 1 reboot when the copy was only about half through — the host then reported a failed copy that had actually succeeded, which looked like a broken flash. The file is now laid out so the copy completes before the Pico reboots, on Pico 1 and Pico 2 alike. What ends up in the Pico's flash is unchanged.

**Full Changelog:** [v1.11.0...v1.11.3](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.11.0...v1.11.3)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
