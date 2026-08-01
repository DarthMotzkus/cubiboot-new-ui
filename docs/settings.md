# Settings

These are all of the values supported by the `cubeboot.ini` file.

```
cube_color = 00ffff     # hex color code
cube_logo = path.png    # path to a 352x40px PNG image
force_progressive = 1   # enables progressive scan
load_from_ode_sd = off  # read games off the SD card inside a GC Loader / ODE
```

## `load_from_ode_sd`

`on` / `off` (also accepts `1`/`0`, `true`/`false`, `yes`/`no`). Defaults to `off`.

By default cubiboot only reads from an EXI card reader (SD2SP2, SD Gecko,
Slot A/B). With `load_from_ode_sd = on` it will instead use the SD card that
sits **inside** the ODE — a [GC Loader](https://gcloaderhq.com/) or anything
else that answers the same drive commands — so the menu can list and boot the
games already on it, with no second card reader.

Two things to know:

- **One volume at a time.** When this is on, everything cubiboot reads comes
  off the ODE's card: the IPL dump, `swiss-gc.dol`, banners and the games. Keep
  them together on that card.
- **Where `config.ini` is read from.** cubiboot has to read `config.ini` before
  it can know this setting, so it mounts the first card it finds and tries
  EXI card readers first, the ODE's card last. On a console with no card reader
  that is the ODE's card, which is exactly where you want `config.ini` to be.
  If you have *both* a card reader and an ODE, put `config.ini` on the card
  reader (that is what gets read) and turn this on to move everything else to
  the ODE's card.

The ODE's card is mounted read-only.
