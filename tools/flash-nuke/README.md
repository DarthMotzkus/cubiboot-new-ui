# Flash nuke (Pico recovery)

`universal_flash_nuke.uf2` —

[pico-universal-flash-nuke](https://github.com/Gadgetoid/pico-universal-flash-nuke)
v1.1.0 by Gadgetoid (BSD-3-Clause), vendored verbatim for easy linking from the install
instructions. One file for both boards: it carries RP2040 and RP2350 block sets, loads
into RAM (`0x20000000`) and erases the entire flash, so it works no matter what state
the flash is in. After it runs, the Pico comes back in BOOTSEL mode by itself, ready
for the next `.uf2`.

Use it as the **recommended first step before any PicoBoot/PicoLoader install**, not just
as recovery: leftovers of whatever was flashed before can survive next to the new install,
and the classic symptom is a **double boot** — the console visibly passes through two
loaders back to back, or keeps booting the old one as if the new flash never happened.
Wiping first guarantees the next `.uf2` is the only thing on the chip. It is also the
recovery step when a Pico refuses to take the PicoBoot firmware or behaves oddly after
flashing — see the [install guide](../../docs/INSTALL.md#wipe-the-pico-first-recommended).

sha256: `3e26888f1393bee4fd0cc6a5191be470910c838feab751cba6c5fa254e9131b6`
