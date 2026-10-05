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

## What's new in v1.13.0

* **GC Loader and Cube ODE work again as the boot device.** With `cubiboot.iso` as `boot.iso` and the games on the ODE's own card, the menu came up fine, but every game or program picked in the grid went to a black screen. The loader jumped into the program before the last bytes it had read off the ODE's card had actually reached memory. It now makes sure they have before starting anything. SD2SP2, SD Gecko and FlippyDrive setups never had the problem, and they take exactly the path they always did.

* **One boot animation on a GC Loader or Cube ODE.** The console's own IPL always runs before an ODE loads a disc, so the factory animation played first and then cubiboot's. `cubiboot.iso` now carries the same no-animation boot header the PicoLoader payload already ships, so only cubiboot's animation plays. Holding **A** at power-on shows the factory animation instead.

* **The disc stops spinning after an In-Game Reset.** After an In-Game Reset from a disc game booted through Swiss, the drive kept spinning through the menu and through any game started from the card. It only stopped when you opened the disc screen or the lid. The menu now stops the motor as soon as it comes up. Only a stock optical drive gets this, and playing a disc works exactly as before.

A Cube ODE speaks the same protocol as a GC Loader and is set up the same way ([Method 3](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#method-3-gc-loader-or-cube-ode)). On a Cube ODE, Swiss itself can stop with "Failed to read FST" on a game whose file is fragmented on the card ([swiss-gc#954](https://github.com/emukidid/swiss-gc/issues/954)). Copying the games onto a freshly formatted card keeps each file in one piece.

**Full Changelog:** [v1.12.4...v1.13.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.12.4...v1.13.0)

>>## Updating from an earlier release?
>>`apploader.img` carries its own complete copy of the loader. If you set up **In-Game Reset**, replace `swiss/patches/apploader.img` as well as the loader itself, both from this release — otherwise a cold boot lands on the new menu while In-Game Reset keeps returning to the old one, with nothing to warn you. If you never installed it, replace the loader and you are done. On a **FlippyDrive** none of this applies: it never uses `apploader.img` — its In-Game Reset is a plain reboot, so the loader in the drive's flash is the only thing to replace. Details: [Updating](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/INSTALL.md#updating).
