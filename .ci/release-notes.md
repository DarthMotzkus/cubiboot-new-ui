## What's new in v1.6.1

Two fixes on top of v1.6.0, both about homebrew apps: they now show their banner before you launch them, and a Swiss app starts Swiss instead of bouncing you to the stock menu.

* **App banner on the pre-boot screen** — picking a homebrew app and landing on the "press START" screen showed the cube with no banner, while discs showed theirs. That screen checked for a disc specifically before drawing, so an app was skipped even though it carries a banner the same way. Apps also lost their author and description lines there — both now come out of the app's `opening.bnr`, exactly as a disc's come out of its own.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

* **Swiss installed as an app now boots** — every program is launched by handing it to Swiss, which for Swiss itself means asking it to load a copy of itself, and the console resets to the stock GameCube menu. Swiss kept in an app folder is now recognised by that folder's name and run directly. It starts even with no `swiss-gc.dol` at the card root, which is exactly the situation where you need it most. Name the folder so it begins with `swiss` — `Swiss v0.6r2073` is fine; capitalisation and anything past the first five letters are ignored. The [README](https://github.com/DarthMotzkus/cubiboot-new-ui#launching-swiss-from-the-menu) now spells out that naming rule for all three forms: app folder, `.dol` and `.iso`.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

**Full Changelog:** [v1.6.0...v1.6.1](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.6.0...v1.6.1)
