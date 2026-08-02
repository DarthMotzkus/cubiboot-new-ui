## What's new in v1.6.0

The biggest release since the fork — games straight off an ODE's SD card, homebrew apps listed with their own banners, and a menu you can colour.

* **Games from a GC Loader's own SD card** — no second card reader needed. Which storage cubiboot reads is now an explicit list: `device_order = sd2sp2, slot_b, slot_a, ode`. Leaving a device out is how you keep cubiboot off it, so there's no separate on/off switch. `config.ini` is looked for on **every** device instead of just the first one that mounts — a console with both a card reader and an ODE no longer ignores the config sitting on the other card.

* **Homebrew apps show up as apps** — a folder holding `default.dol` next to `opening.bnr` is listed as a launchable application with its banner, instead of a folder you have to open. Press **A** and it runs. The repo now ships [`tools/banner-converter/run.py`](https://github.com/DarthMotzkus/cubiboot-new-ui/tree/main/tools/banner-converter), which turns any image into the `opening.bnr` an app needs.

* **Colour the menu** — `theme_color = ff9801` paints the boot logo, the grid cubes, the info panel and the big "PRESS START" in one line. `cube_color`, `menu_cube_color`, `menu_box_color` and `menu_start_color` override it per item. Hex or `random`, and `menu_cube_color` also takes one of the IPL's own palette names (`blue`, `green`, `yellow`, `orange`, `red`, `purple`). Set nothing and the stock look is untouched.

* **The header names the folder you're in** — browsing `Util` says Util, instead of always saying "Games".

* **New Cubiboot banner** on the loader and on the `.iso` BIOS intro.

* **Rewritten README and docs** — the README is reorganised around a clickable index, every `config.ini` option is in one reference table, and the release `config.ini` is a real commented template. The old install guides described cubeboot's filenames and options rather than this fork's, so they're gone; [`docs/ARCHITECTURE.md`](https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/docs/ARCHITECTURE.md) now maps how the loader, the injected menu and the storage stack fit together.

**Full Changelog:** [v1.5.2...v1.6.0](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.5.2...v1.6.0)
