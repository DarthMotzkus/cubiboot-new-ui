# Settings

These are all of the values supported by the `cubeboot.ini` file.

```
theme_color = 00ffff    # hex color code -- one color for the whole UI
cube_color = 00ffff     # hex color code -- boot logo only
menu_cube_color = green # hex color code, or a stock palette name
menu_box_color = 6e00b3         # hex color code
menu_start_color = ff2d55       # hex color code
cube_logo = path.png    # path to a 352x40px PNG image
force_progressive = 1   # enables progressive scan
load_from_ode_sd = off  # read games off the SD card inside a GC Loader / ODE
```

## Colors

Every color key takes a hex RGB code or `random` (re-rolled on each boot). `000000` is a
real black, not "unset". With no color key at all, the stock look is untouched.

`theme_color` is the umbrella: it feeds the boot logo, the grid cubes, the info-box gradient
at the bottom of the game list and the big block "PRESS START". Each of those has its own
key, which wins when set.

| Key | Paints | Falls back to |
|-----|--------|---------------|
| `theme_color` | everything below | stock |
| `cube_color` | boot logo cube | `theme_color` |
| `menu_cube_color` | grid cubes / banner tiles | `theme_color` |
| `menu_box_color` | info panel under the game list | `theme_color` |
| `menu_start_color` | big block "PRESS START" | `theme_color` |

Neither derivation is a straight fill:

- **Grid cubes.** The IPL stores four shades per cube (bright / dimmed, each with a selected
  variant). `menu_cube_color` re-tints that whole set — hue rotated, saturation and lightness
  scaled by the ratio between your color and the stock bright shade — so the selected cube
  still reads as selected. The stock shades themselves are shared with the memory card menu
  and are never written to; the recolored copies live in the patch's own `.data`.
  `menu_cube_color` also accepts a stock palette name (`blue`, `green`, `yellow`, `orange`,
  `red`, `purple` — the default), which uses Nintendo's shades verbatim. Naming a palette and
  setting `theme_color` picks that palette *and* tints it.
- **Info panel.** One key drives both ends of its gradient. Stock is top `6e00b3`
  (H=196 S=255 L=89) fading to bottom `800057` (H=226 S=255 L=64): saturation holds, lightness
  drops to ~72%, hue swings +30 (about +42°, purple → magenta). `menu_box_color` sets the
  bright end and the far end reuses that lightness falloff. The hue swing is deliberately
  dropped — rotating it onto another hue turns the gradient into a clash, with orange fading
  to lime.

- **The big "PRESS START".** Drawn by the stock BIOS's `draw_start_anim`, which takes only an
  alpha byte — there is no color parameter. Disassembling it on all seven dumps shows it loops
  over `count` blocks and, per block, builds two `GXColorS10` out of two parallel `GXColor`
  arrays and writes them into the block model's `mat[0].tev_color[0..1]`. Each array is
  reached through one `lis/addi/blr` getter that is called from `draw_start_anim` and nowhere
  else, so writing them disturbs nothing. cubiboot rewrites only R/G/B, every frame, right
  before the draw: absolute writes never compound a tint, and byte 3 of the first array is the
  per-block intensity feeding the model alpha, so leaving it alone keeps the fly-in and fade
  exactly as stock. The small `Press START to begin!` line above is a separate draw
  (`draw_blob_fixed`) and always stays white.

  The addresses are per revision, in `get_start_blocks()` in
  [`patches/source/theme.c`](../patches/source/theme.c). Each was decoded from that revision's
  own dump, identified by its r2/r13 pair and by CRC32 of its code region against
  `bios_table[]`: NTSC 1.0-001, 1.1-001, 1.2-001, 1.2-101, PAL 1.0-001, PAL 1.2-101 and
  MPAL 1.1 — **every revision cubiboot can boot on**. `get_ipl_revision()` also knows
  NTSC 1.0-002, PAL 1.0-002, DEV 1.0 and TDEV 1.1 (the NPDP / dev-kit BIOSes), but those are
  not in `bios_table[]`, so `load_ipl` halts with `Bad IPL image` before the menu is ever
  injected, and no linker script supplies their relocs. The `default:` branch is unreachable
  on retail hardware.

  This is deliberately **not** a `.reloc` symbol. A missing relocation is fatal
  (`prog_halt("Failed BIOS Patching relocation")`), so an unknown revision would refuse to
  boot; a `switch` on `get_ipl_revision()` with a `default:` just leaves the color alone.

The parsing lives in [`cubeboot/source/settings.c`](../cubeboot/source/settings.c)
(`ini_get_color`), the derivations in
[`patches/source/theme.c`](../patches/source/theme.c).

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
