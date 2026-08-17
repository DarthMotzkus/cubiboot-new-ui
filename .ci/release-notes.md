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

## What's new in v1.9.6

* **The FlippyDrive stopped being treated as an ODE — because it isn't one.** A GC Loader or CUBE-ODE takes the optical drive's place; a FlippyDrive rides the drive ribbon *beside* the drive, so the drive is still there. Everything in this release follows from that. Confirmed working on real hardware.

* **The disc screen (Z) works on a FlippyDrive.** v1.9.5 refused it along with the real ODEs. Now cubiboot switches the drive into bypass for the read — the same mechanism Swiss uses — shows the console's own Game Play screen for the disc in the tray, and switches back afterwards, with the menu's file access intact. A GC Loader / CUBE-ODE still answers **Z** with the error tone, since there is genuinely no drive behind it; a FlippyDrive console whose optical drive was physically removed just gets the stock no-disc sequence.

* **In-Game Reset on a FlippyDrive needs nothing installed.** cubiboot now passes Swiss `IGRType=Reboot` on this hardware: the drive autoloads cubiboot from its flash on every reboot, so the reset combo lands back on the menu with no `apploader.img` anywhere. (`apploader.img` never worked on a FlippyDrive; if you copied one to `swiss/patches/`, it is simply ignored. Every other setup keeps using it exactly as before.) The [install](https://github.com/DarthMotzkus/cubiboot-new-ui#method-4-flippydrive) and [update](https://github.com/DarthMotzkus/cubiboot-new-ui#updating) instructions were rewritten accordingly.

* **`swiss-gc.dol` on the FlippyDrive's card is now documented as optional.** Swiss ships in the drive's flash and cubiboot has always preferred that copy; a `swiss-gc.dol` at the card root is only a fallback for a flash copy that went missing. Keeping one there is a harmless precaution.

* **`device_order`: `ode` now means the real ODEs only** (GC Loader / CUBE-ODE — one entry covers both, they speak the same commands). The FlippyDrive answers to its own `flippy` entry (also `flippydrive` / `fldrv`), which is part of the default order — if you never set the key, nothing changes. If your `config.ini` says `device_order = ode` on a FlippyDrive, change it to `flippy`; until you do, cubiboot falls back to the flippy on its own, so the games still appear. The menu header now reads **FLIPPY SD** instead of ODE SD.

* **Fixed: leaving the disc screen could hang a console with no optical drive.** The stop-motor command on the way out waited forever on a bus where nothing answers. The wait is now bounded, so a driveless console (or a FlippyDrive without its drive) falls through to the stock no-disc flow instead of freezing. The hazard predates v1.9.6; giving the FlippyDrive back the disc screen made it worth closing.

**Full Changelog:** [v1.9.5...v1.9.6](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.9.5...v1.9.6)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui#updating).
