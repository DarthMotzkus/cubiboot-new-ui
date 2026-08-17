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
preboot_delay_ms = 3000         # wait before the boot animation, in milliseconds
postboot_delay_ms = 2000        # hold the last frame before the game boots
force_widescreen = on           # render the menu anamorphic for a 16:9 TV
swiss_on_dvd_boot = off         # boot physical discs with the console instead of Swiss
remember_last_game = on         # open on the last game you booted, already highlighted
device_order = sd2sp2, slot_b, slot_a, ode   # storage to read games from, most wanted first
```

## On/off switches

Every switch below -- `swiss_on_dvd_boot`, `remember_last_game`, `force_widescreen`,
`force_progressive`, `show_watermark`, `disable_mcp_select` -- reads `on` or `off`. `1`/`0`,
`yes`/`no` and `true`/`false` are accepted as well, so a config carried over from another
tool still reads. A value that is none of those is ignored and the default stays, rather
than a typo silently flipping the switch; the loader prints what it decided either way.

Parsed by `ini_get_bool()` in
[`cubeboot/source/settings.c`](../cubeboot/source/settings.c).

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
  bright end, which renders at the **top**; the dark end keeps the hue and saturation and
  drops to **20%** lightness at the bottom (orientation confirmed on hardware). The hue
  swing is deliberately dropped — rotating it onto another hue turns the gradient into a
  clash, with orange fading to lime — and the falloff is far steeper than stock's, because
  lightness now carries alone what stock split between two cues. Stock's own ~72% read as a
  solid panel on hardware, and so did 45%, so the dark end is near-black: the panel runs from
  dark at the bottom up into the color you picked.

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

## `force_widescreen`

Renders the menu anamorphic for a 16:9 TV: the IPL's perspective and orthographic
projections are widened by 4:3 → 16:9, which squeezes the picture horizontally in the
framebuffer so it comes out proportioned once the TV stretches it back. Set the TV (or
GCVideo) to Full/16:9 mode; on a 4:3 screen the menu just looks squeezed. The whole UI —
boot animation, grid, banners — goes through the same projection, so everything scales
together. The trade-off is inherent to anamorphic output: the same 640 pixels now cover a
wider image, so effective horizontal resolution drops. Off by default. Ported from
[OffBroadway/cubeboot#57](https://github.com/OffBroadway/cubeboot/pull/57).

## `swiss_on_dvd_boot`

What boots a **physical disc** when you press START on the disc screen. **On by default**,
which chainloads Swiss with `Autoload=dvd:/*.gcm`. Two things come with that:

- **IGR** — the in-game reset combo returns to cubiboot instead of the stock IPL. This needs
  `apploader.img` (built alongside `IPL.dol`) copied to `/swiss/patches/apploader.img` on the
  card; without it Swiss still boots the disc, but the reset combo does not come back.
- **Out-of-region discs boot.** Nothing on the Swiss path consults the console's region.

Set it to `off` and the disc is handed to the console's own apploader instead -- the stock
boot, without IGR and without the region bypass. Games on the card are unaffected either
way: they always go through Swiss.

Either way `swiss-gc.dol` must be at the card root.

## `device_order`

Which storage cubiboot reads from, most wanted first. The first entry that mounts becomes
the volume everything comes off: the IPL dump, `swiss-gc.dol`, banners and the games.

| Name | Where it is |
|------|-------------|
| `sd2sp2` (or `sdc`) | Serial Port 2 — an **SD2SP2** |
| `slot_b` (or `sdb`) | Memory card **slot B** — an SD Gecko |
| `slot_a` (or `sda`) | Memory card **slot A** — an SD Gecko |
| `ode` | **Whichever ODE is installed** — resolved by asking the drive, so it covers both of the two below |
| `gcloader` (or `gcldr`) | The SD card **inside a [GC Loader](https://gcloaderhq.com/)**, or anything answering the same drive commands |
| `flippy`, `flippydrive` (or `fldrv`) | The SD card **inside a FlippyDrive** |

Separate the names with commas or spaces; case does not matter. Unknown names are reported
and skipped. Default when the key is absent:

```ini
device_order = sd2sp2, slot_b, slot_a, ode
```

`ode` is the one to reach for: there is a single drive connector, so a GC Loader and a
FlippyDrive can never both be installed, and cubiboot identifies which one is there from the
drive's own inquiry. Naming a specific one is only useful for forcing the issue while
diagnosing something.

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

A GC Loader's card is mounted read-only. A console without an ODE pays **one** drive inquiry
per boot no matter how many ODE names are in the list — the inquiry identifies every drive
cubiboot knows in one answer, and gives up as soon as a real optical drive replies, or as
soon as nothing replies at all.

A FlippyDrive is not mounted at all, because it is not a disk: it serves files itself and
cubiboot asks it for paths directly. Nothing about that is visible in `config.ini` — the
same `device_order`, the same `config.ini` on the card root, the same everything. It matters
only in that the drive, not cubiboot, decides what the filesystem looks like.

> [!NOTE]
> On a FlippyDrive, cubiboot itself lives in the **drive's internal flash**, not on the SD
> card — see [Method 4](../README.md#method-4-flippydrive) in the README. The SD card still
> holds `config.ini`, Swiss and your games, exactly as on any other setup.
