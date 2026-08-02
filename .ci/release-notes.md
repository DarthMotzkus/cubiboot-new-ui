## What's new in v1.6.1

A display fix on top of v1.6.0 — homebrew apps now show their banner on the screen before you launch them, and everything else is unchanged.

* **App banner on the pre-boot screen** — picking a homebrew app and landing on the "press START" screen showed the cube with no banner, while discs showed theirs. That screen checked for a disc specifically before drawing, so an app was skipped even though it carries a banner the same way. Apps also lost their author and description lines there — both now come out of the app's `opening.bnr`, exactly as a disc's come out of its own.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

**Full Changelog:** [v1.6.0...v1.6.1](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.6.0...v1.6.1)
