## What's new in v1.6.2

Both boot-delay options are fixed. `preboot_delay_ms` and `postboot_delay_ms` came from upstream cubeboot, and neither produced the delay you configured — one waited about a quarter of it, the other did nothing whatsoever. The values are milliseconds, and that is now what you get.

* **`preboot_delay_ms` waited roughly a quarter of the time you asked for** — the millisecond value was divided by the frame rate instead of multiplied by it, so `preboot_delay_ms = 15000` held for about 4 seconds on NTSC and 6 on PAL rather than 15. This is the option to reach for when a TV or GCVideo needs a moment to lock onto the signal before the boot animation starts, so coming out short defeated the point of setting it. Worth knowing: a value that felt about right before will now wait roughly 3.6x longer on NTSC.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

* **`postboot_delay_ms` did nothing at all** — it was checked at a point in the boot that only the Z-trigger DVD passthrough ever reached, and it measured its wait from the end of the cold-boot logo animation, which by the time you pick a game is long past. Choosing a game from the grid never reached it. The wait now happens where every boot path converges, holding the last frame on screen before the game takes over — games, homebrew programs and disc passthrough alike. Pure flair: it recovers the feeling of waiting for a disc to spin up.
    _Designed by [@DarthMotzkus](https://github.com/DarthMotzkus)._

Both options are described in the bundled `config.ini`, which until now covered the pair in a single line that fit neither of them.

**Full Changelog:** [v1.6.1...v1.6.2](https://github.com/DarthMotzkus/cubiboot-new-ui/compare/v1.6.1...v1.6.2)
