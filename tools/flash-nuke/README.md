# Flash nuke (Pico recovery)

`universal_flash_nuke.uf2` —

[pico-universal-flash-nuke](https://github.com/Gadgetoid/pico-universal-flash-nuke)
v1.1.0 by Gadgetoid (BSD-3-Clause), vendored verbatim for easy linking from the install
instructions. One file for both boards: it carries RP2040 and RP2350 block sets, loads
into RAM (`0x20000000`) and erases the entire flash, so it works no matter what state
the flash is in. After it runs, the Pico comes back in BOOTSEL mode by itself, ready
for the next `.uf2`.

Use it as the recovery step when a Pico refuses to take the PicoBoot firmware or
behaves oddly after flashing — see the PicoBoot install notes in the
[install guide](../../docs/INSTALL.md#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader).

sha256: `3e26888f1393bee4fd0cc6a5191be470910c838feab751cba6c5fa254e9131b6`
