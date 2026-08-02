# Settings

These are all of the values supported by the `config.ini` file.

The commented template that ships in every release lives at
[`.ci/config.ini`](../.ci/config.ini) — edit that one when adding or changing an option, and
the release picks it up. This page is the prose reference.

```
theme_color = 00ffff    # hex color code -- one color for the whole UI
cube_color = 00ffff     # hex color code -- boot logo only
menu_cube_color = green # hex color code, or a stock palette name
menu_box_color = 6e00b3         # hex color code
menu_start_color = ff2d55       # hex color code
cube_logo = path.png    # path to a 352x40px PNG image
force_progressive = 1   # enables progressive scan
device_order = sd2sp2, slot_b, slot_a, ode   # storage to read games from, most wanted first
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
  variant). `menu_cube_color` re-tints that whole set — every shade takes your hue, with
  saturation and lightness scaled by the ratio between your color and the stock bright shade
  — so the selected cube still reads as selected. The hue is assigned, not rotated: the IPL's
  palettes are not four shades of one hue (stock purple spans 265.7° to 310°), so rotating
  them all by one amount dragged the outliers elsewhere and the selected tile came out lime. The stock shades themselves are shared with the memory card menu
  and are never written to; the recolored copies live in the patch's own `.data`.
  `menu_cube_color` also accepts a stock palette name (`blue`, `green`, `yellow`, `orange`,
  `red`, `purple` — the default), which uses Nintendo's shades verbatim. Naming a palette and
  setting `theme_color` picks that palette *and* tints it.
- **Info panel.** One key drives both ends of its gradient. Stock is top `6e00b3`
  (H=196 S=255 L=89) fading to bottom `800057` (H=226 S=255 L=64): saturation holds, lightness
  drops to ~72%, hue swings +30 (about +42°, purple → magenta). `menu_box_color` sets the
  bright end; the far end keeps the hue and saturation and drops to **45%** lightness. The hue
  swing is deliberately dropped — rotating it onto another hue turns the gradient into a
  clash, with orange fading to lime — and the falloff is steeper than stock's to compensate,
  because lightness now carries alone what stock split between two cues. At stock's own ~72%
  a grey theme read as a solid panel on hardware.

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

## `device_order`

Which storage cubiboot reads from, most wanted first. The first entry that mounts becomes
the volume everything comes off: the IPL dump, `swiss-gc.dol`, banners and the games.

| Name | Where it is |
|------|-------------|
| `sd2sp2` (or `sdc`) | Serial Port 2 — an **SD2SP2** |
| `slot_b` (or `sdb`) | Memory card **slot B** — an SD Gecko |
| `slot_a` (or `sda`) | Memory card **slot A** — an SD Gecko |
| `ode`, `gcloader` (or `gcldr`) | The SD card **inside the ODE** — a [GC Loader](https://gcloaderhq.com/) or anything answering the same drive commands |

Separate the names with commas or spaces; case does not matter. Unknown names are reported
and skipped. Default when the key is absent:

```ini
device_order = sd2sp2, slot_b, slot_a, ode
```

Leaving a device out is how you keep cubiboot off it — there is no separate on/off switch.
A console with both an SD2SP2 and a GC Loader, whose games live on the ODE, writes:

```ini
device_order = ode
```

Two things to know:

- **One volume at a time.** cubiboot does not merge cards. Whichever entry wins holds
  everything.
- **`config.ini` can live on any of them.** cubiboot looks for it on every device it can
  mount, in the default order above, and reads the first one that actually has the file.
  It has to work that way, because `device_order` lives *inside* the file being looked
  for. If two cards both carry a `config.ini`, the default order breaks the tie.

If nothing in the list mounts, cubiboot keeps whatever the search settled on rather than
booting into an empty menu over one bad line.

The ODE's card is mounted read-only. A console without an ODE pays one drive inquiry per
boot for the `gcldr` entry, which gives up as soon as a real optical drive answers, or as
soon as nothing answers at all.
