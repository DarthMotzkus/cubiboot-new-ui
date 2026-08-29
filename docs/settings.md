# Settings

These are all of the values supported by the `config.ini` file, which lives in the root of
the card cubiboot reads from. The file is optional: without one you get the `small_banners`
layout and the card root as the starting folder.

The commented template that ships in every release lives at
[`.ci/config.ini`](../.ci/config.ini) — edit that one when adding or changing an option, and
the release picks it up. This page is the prose reference.

```
menu_grid_type = small_banners  # grid layout: small_banners | banners | square_icons
default_folder = /games         # folder the menu opens in; unset = card root
theme_color = 00ffff    # hex color code -- one color for the whole UI
cube_color = 00ffff     # hex color code -- boot logo only
menu_cube_color = green # hex color code, or a stock palette name
menu_box_color = 6e00b3         # hex color code
menu_start_color = ff2d55       # hex color code
cube_logo = /logo.raw           # your art instead of the "GAMECUBE" boot text (raw RGBA8 352x40)
preboot_delay_ms = 3000         # wait before the boot animation, in milliseconds
postboot_delay_ms = 2000        # hold the last frame before the game boots
force_widescreen = on           # render the menu anamorphic for a 16:9 TV
force_progressive = on          # menu in 480p -- IPL 1.1/1.2 only, no-op on IPL 1.0
swiss_on_dvd_boot = off         # boot physical discs with the console instead of Swiss
remember_last_game = on         # open on the last game or app you booted, already highlighted
text_scroll = 2                 # long titles scroll after this many seconds (on/off work too)
big_titles_scroll_speed = 10    # marquee pace in frames per character: 1 fastest, higher slower
device_order = sd2sp2, slot_b, slot_a, ode, flippy   # storage to read games from, most wanted first
```

## On/off switches

Every switch below -- `swiss_on_dvd_boot`, `remember_last_game`, `force_widescreen`,
`force_progressive`, `disable_mcp_select` -- reads `on` or `off`. `1`/`0`,
`yes`/`no` and `true`/`false` are accepted as well, so a config carried over from another
tool still reads. A value that is none of those is ignored and the default stays, rather
than a typo silently flipping the switch; the loader prints what it decided either way.

Parsed by `ini_get_bool()` in
[`cubeboot/source/settings.c`](../cubeboot/source/settings.c).

## `menu_grid_type`

The selection-menu grid layout:

| Value | Layout |
|-------|--------|
| `small_banners` | small banners, 4 columns (**default**) |
| `banners` | large banners, 3 columns |
| `square_icons` | square icons, 8 columns |

The default applies even without a `config.ini`. `square_icons` is the one to use for very
large folders — see [Large folders and the banner pool](#large-folders-and-the-banner-pool).

## `default_folder`

The directory the menu opens in. Leave it unset (or commented) to open the card root. A
leading `/` is added automatically if you omit it, and if the folder can't be opened
cubiboot falls back to the root.

`default_folder` only changes where the menu browses for **games and homebrew**
(`.dol` / `.iso` / …). The system files still have to sit at the card root: `ipl.dol`,
`config.ini`, `swiss-gc.dol` and `swiss/patches/apploader.img`.

When [`remember_last_game`](#remember_last_game) is on, it overrides this: the menu opens in
the last played folder, and `default_folder` is only the fallback.

## `remember_last_game`

`on` makes the menu open **in the folder of the last thing you booted** — a game or a
homebrew app/`.dol` alike — with it already highlighted, so on the next boot you just press
**A**. Off by default. A physical-disc launch is skipped (there is no list entry to come
back to), so the most recent card entry is selected instead.

> [!IMPORTANT]
> This reads Swiss's own recent list, so Swiss has to be keeping one. In Swiss, open
> **Settings** and set **Recent List** to **On** (it writes `RecentListLevel=On` into
> `/swiss/settings/global.ini`). With it **Off** there is no `recent.ini` to read and
> cubiboot falls back to [`default_folder`](#default_folder).

How it works, and how it interacts with `default_folder`:

- Cubiboot boots games by chainloading **Swiss** with autoload, so Swiss records every launch
  in its own recent-games list (`/swiss/settings/recent.ini`). Cubiboot just **reads** that
  list back — there is no extra file to write.
- On the next cold boot the menu opens directly in the folder holding the most recent entry
  — including a letter/genre subfolder, not just `default_folder` — and highlights it.
  Navigate away normally (**B** goes up a level).
- **No stalls:** for that first folder cubiboot does *not* wait for every banner before
  showing the list. It scans the folder (fast — headers only), puts the cursor on your last
  game, and a **background thread** fills banners in priority order: the on-screen window
  around your game first, so it appears almost immediately regardless of folder size, then
  the rest while the menu is already usable. Pressing **A** works even while banners are
  still loading.
- **`remember_last_game` overrides `default_folder`.** When it's on, the menu **always**
  opens in the last played folder. `default_folder` (or the card root, if unset) is only a
  fallback: on the very first boot before anything has been played, or if the last entry's
  folder is gone.
- **Large folders:** if the last played folder holds more games than fit in the banner pool
  (>128), it falls back to the sliding window — banners are read from the card as they
  scroll into view. Either way your highlighted game shows first. See
  [Large folders and the banner pool](#large-folders-and-the-banner-pool).

## `preboot_delay_ms` and `postboot_delay_ms`

Both in milliseconds, both `0` by default. `preboot_delay_ms` waits before the boot
animation, holding a live video signal, so a TV or GCVideo has time to lock on before the
animation starts. `postboot_delay_ms` holds the last frame after picking a game, before it
boots.

## `text_scroll`

A game title longer than the info box holds still, then scrolls sideways so the whole name
can be read; the description line under it scrolls by hand with the L and R triggers. This
key controls the automatic part:

- `on` (default) — long titles scroll after holding still for 2 seconds.
- `off` — titles never scroll on their own. L/R on the description still works.
- a number — seconds to hold still before scrolling starts. **This is the one key where
  `1` and `0` are not switch values**: `1` waits one second, `0` starts scrolling
  immediately. Capped at 600.

Texts that already fit are never scrolled, whatever this is set to.

## `big_titles_scroll_speed`

How fast the title marquee moves once it starts, in **frames per character**: `1` steps every
frame (fastest), `10` (the default) steps about six characters a second on NTSC, and larger
numbers are slower. Accepts 1 to 255; anything else keeps the default. Only the automatic
title marquee is affected -- the L/R scrolling of the description has its own fixed pace.

## Colors

Every color key takes a [hex RGB](https://www.w3schools.com/colors/colors_hexadecimal.asp) code or `random` (re-rolled on each boot). `000000` is a
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

## `cube_logo`

Replaces the "GAMECUBE" text under the cube in the boot animation with your own art:

```ini
[cubeboot]
cube_logo = /logo.raw
```

The file is **raw RGBA8, exactly 352×40 px (56,320 bytes)** — not a PNG. Convert your
image with the [cube logo converter](https://htmlpreview.github.io/?https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/tools/cube-logo-converter/index.html) — (refer to it's [doc](https://github.com/DarthMotzkus/cubiboot-new-ui/tree/main/tools/cube-logo-converter) for more) it opens as a
page in any browser, nothing to install — or run
[`png2cubelogo.py`](../tools/cube-logo-converter/) instead, then copy the resulting `.raw`
to the card. For drawing the logo itself,
[fontmeme's GameCube font](https://fontmeme.com/gamecube-font/) is a good starting point —
but the PNG still has to go through the converter (a `.png` on the card is rejected; this
fork's firmware has no PNG decoder, unlike the original cubeboot other guides may
reference).

Transparency is kept, and the art's own colors are shown as-is. A missing or wrong-sized
file falls back to the stock text, so a bad conversion can never break the boot. Loaded
once per boot, on the game-enumeration thread
([`patches/source/games.c`](../patches/source/games.c), `gm_load_cube_logo`).

## `force_widescreen`

Renders the menu anamorphic for a 16:9 TV: the IPL's perspective and orthographic
projections are widened by 4:3 → 16:9, which squeezes the picture horizontally in the
framebuffer so it comes out proportioned once the TV stretches it back. Set the TV (or
GCVideo) to Full/16:9 mode; on a 4:3 screen the menu just looks squeezed. The whole UI —
boot animation, grid, banners — goes through the same projection, so everything scales
together. The trade-off is inherent to anamorphic output: the same 640 pixels now cover a
wider image, so effective horizontal resolution drops. Off by default. Ported from
[OffBroadway/cubeboot#57](https://github.com/OffBroadway/cubeboot/pull/57).

## `force_progressive`

Renders the **menu** in progressive scan (480p) — useful over component cables or a
GCVideo. Off by default. Inherited from the original cubeboot: the loader rewrites the
IPL's render mode to NTSC 480p before the BIOS main runs (a PAL IPL is switched to NTSC
timing for the menu, with the boot animation retimed to match).

Only covers **IPL 1.1 and 1.2**: on a launch console's **IPL 1.0** the loader force-disables
it (`cubeboot/source/main.c`, inherited from upstream commit `f3d74d1`), so the key is
safely ignored there — nothing breaks, it just does nothing.

Menu only: when a game boots, video returns to interlaced (`patches/source/boot.c`) and the
game's own progressive mode is negotiated by the game and Swiss as usual.

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

> [!WARNING]
> **On a FlippyDrive this switch must be `off` -- the Swiss disc boot does not work on that
> hardware.** Swiss refuses to take over the optical drive while a FlippyDrive is present, so
> with the switch on, pressing START on a disc ends on a permanent black screen. Put
> `swiss_on_dvd_boot = off` in `config.ini` and physical discs boot through the console's own
> apploader -- still region-free, since cubiboot's disc screen never consults the console's
> region. Games on the SD card are unaffected: they keep going through Swiss, IGR included.

Either way Swiss has to be reachable: `swiss-gc.dol` at the card root, except on a
FlippyDrive, where the copy in the drive's flash is used first and the card root is only the
fallback.

The screen reads the disc when it opens, and again whenever the lid is closed -- so a disc put
in after pressing **Z** is picked up where it used to need leaving the screen and coming back.
A drive that has been idle takes seconds to spin up and answers every command with an error
until it is ready, so "Reading disc..." can stay up for a while before it gives an answer;
**B** leaves at any point, and **START** boots only a disc the screen has actually read.

None of this applies on a console with an ODE (GC Loader): the ODE is what sits on
the drive connector, so there is no optical drive to read and the disc screen does not open
at all -- **Z** answers with the menu's error tone. The check is a single inquiry made once
at startup, so it costs nothing per press. A **FlippyDrive is not an ODE** -- it rides the
drive ribbon beside the optical drive -- so it keeps the disc screen: cubiboot switches the
drive into bypass for the read and back out afterwards.

> [!NOTE]
> **Autobooting a disc straight from power-on is not a cubiboot feature, and is not planned.**
> Cubiboot always comes up on its own menu; a disc is played from the disc screen (**Z**). The
> console's own IPL is what autoboots a disc — power on with one in the drive and it boots
> after the intro animation — and getting that behaviour back inside cubiboot is more
> technical trouble than the convenience is worth, so the answer is to boot the stock BIOS
> when that is what you want:
>
> - **With gekkoboot in the chain** (installation [Method 1](INSTALL.md#method-1-picoboot-or-picoloader-with-gekkoboot)):
>   hold **D-Pad Left**, or hold the **reset button**, while powering the console on. Gekkoboot
>   steps aside and hands the boot to the onboard IPL, which then plays the disc by itself.
> - **A is not the shortcut for this.** Gekkoboot reads a held **A** as "load `/a.dol`", and
>   with no such file on the card it falls straight back to `ipl.dol` — cubiboot again.
> - **On a [Method 2](INSTALL.md#method-2-cubiboot-flashed-into-the-modchip-picoboot-or-picoloader)
>   install** (cubiboot flashed in place of gekkoboot) there is no such bypass: the modchip has
>   nothing else to hand the boot to. Reaching the stock BIOS there means reflashing.

## `device_order`

Which storage cubiboot reads from, most wanted first. The first entry that mounts becomes
the volume everything comes off: the IPL dump, `swiss-gc.dol`, banners and the games.

| Name | Where it is |
|------|-------------|
| `sd2sp2` (or `sdc`) | Serial Port 2 — an **SD2SP2** |
| `slot_b` (or `sdb`) | Memory card **slot B** — an SD Gecko |
| `slot_a` (or `sda`) | Memory card **slot A** — an SD Gecko |
| `ode` (or `gcloader`, `gcldr`) | The SD card **inside an ODE** — a [GC Loader](https://gcloaderhq.com/) |
| `flippy`, `flippydrive` (or `fldrv`) | The SD card **inside a FlippyDrive** |

Separate the names with commas or spaces; case does not matter. Unknown names are reported
and skipped. Default when the key is absent:

```ini
device_order = sd2sp2, slot_b, slot_a, ode, flippy
```

`ode` names the drive **replacements** — the GC Loader. A FlippyDrive is deliberately
**not** under it: it is not an ODE
(it rides the drive ribbon beside the optical drive rather than taking the drive's place),
it speaks its own protocol, and it has its own entry. The two still cannot coexist — an ODE
and a FlippyDrive want the same attachment point — which is why both being in the default
order costs nothing: at most one of them can ever answer.

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

A GC Loader's card is mounted read-only. A console with neither an ODE nor a FlippyDrive
pays **one** drive inquiry per boot no matter how many drive-interface names are in the
list — the inquiry identifies every drive cubiboot knows in one answer, and gives up as soon
as a real optical drive replies, or as soon as nothing replies at all.

A FlippyDrive is not mounted at all, because it is not a disk: it serves files itself and
cubiboot asks it for paths directly. Nothing about that is visible in `config.ini` — the
same `device_order`, the same `config.ini` on the card root, the same everything. It matters
only in that the drive, not cubiboot, decides what the filesystem looks like.

> [!NOTE]
> On a FlippyDrive, cubiboot itself lives in the **drive's internal flash**, not on the SD
> card — see [Method 4](INSTALL.md#method-4-flippydrive) in the install guide. The SD card
> still holds `config.ini` and your games. Swiss also comes from the drive's flash: a
> `swiss-gc.dol` at the card root is only a fallback for a flash copy that went missing, and
> `apploader.img` is not used on this hardware at all.

# Menu behavior

Not `config.ini` keys, but the reference for how the menu treats what it finds on the card.

## Homebrew apps

A folder that holds **`default.dol`** and **`opening.bnr`** side by side is treated as an
application, not as a folder. It shows up in the grid with the banner from its `opening.bnr`,
and pressing **A** runs the `.dol` directly instead of opening the folder.

```
/apps/
  my-app/
    default.dol     <- what gets launched
    opening.bnr     <- name, description and banner art
  another-app/
    default.dol
    opening.bnr
```

Both filenames are fixed. The banner is the same format retail discs use, so a title,
description and 96x32 image all come from that one file.

A folder missing either file behaves exactly as before — you enter it and browse. The check
costs one file probe per folder while the list is being built, and folders without an
`opening.bnr` stop right there, so a library of game folders is unaffected.

**Making the banner.** [`tools/banner-converter/run.py`](../tools/banner-converter) turns any
image into an `opening.bnr`. Download it from this repo, put your artwork next to it and run:

```sh
pip install Pillow
python run.py
```

Pick option **2**, answer the title/author/description prompts, and it writes
`output/<name>/opening.bnr`. Drop your `default.dol` beside that file and the folder is ready
for the card. See [its README](../tools/banner-converter) for the sizing rules — the slot is
96×32, so a wordmark that looks thin wants a *taller* source, not a wider one.

Prefer the browser? The [GameCube Banner Editor & Converter](https://git2358.github.io/GameCube-Banner-Editor-Converter/)
by [git2358](https://github.com/git2358) builds and edits `opening.bnr` files as a web page —
nothing to install, recommended for a one-off banner.

## Launching Swiss from the menu

Cubiboot boots games by chainloading Swiss. That makes Swiss itself a special case: handing
it to Swiss would be asking Swiss to load a copy of itself, which resets the console to the
stock GameCube menu. Cubiboot avoids that by recognising Swiss and running it directly — but
it recognises it **by name**, so the name is what you have to get right.

**Name it so it starts with `swiss`.** Capitalisation does not matter, and anything after
the first five letters is ignored, so `Swiss v0.6r2073` works as well as `swiss`.

| How you keep Swiss | What has to start with `swiss` | Example |
|---|---|---|
| A `.dol` in any folder | the **filename** | `swiss-gc.dol`, `Swiss v0.6r2073.dol` |
| A [homebrew app](#homebrew-apps) folder | the **folder** name | `apps/Swiss v0.6r2073/default.dol` |
| A disc image | the **filename** | `Swiss v0.6r2073.iso` |

For the app folder, only `default.dol` inherits the folder's name — any other `.dol` sitting
in there is treated as a different program that happens to live beside Swiss.

Get the name wrong and Swiss is treated as an ordinary program: it gets handed to the Swiss
at your card root, and the console resets to the stock menu instead of starting. Nothing is
damaged, and renaming fixes it.

Two things worth knowing:

- **A Swiss app boots even with no `swiss-gc.dol` at the root.** Everything else needs that
  file, because everything else is booted through Swiss — but Swiss needs no chainloader. So
  a card that only carries Swiss as an app can still start it, which is what you want when
  that root file is what went missing.
- **Disc images do not need the name at all.** Every disc image boots through cubiboot's own
  apploader, so a Swiss `.iso` works whatever it is called. The naming rule matters for the
  `.dol` and the app folder, which are the forms that go through Swiss.

This is all separate from the `swiss-gc.dol` at the card **root**, which is the copy games
are launched with and has to be there regardless.

## Large folders and the banner pool

Banners live in a fixed low-memory pool capped at **128 banner images** (ARAM streaming was
dropped — it corrupted banners on load). That cap defines two modes:

| Files in the folder | Behaviour |
|---|---|
| **≤ 128** | All banners stay resident. Scrolling is instant. Best case for the banner layouts. |
| **> 128** | The pool fills, then switches to an **on-demand sliding window** — off-screen banners are freed and re-read from the card as you scroll. Names still appear instantly; only images load on demand. |

> [!WARNING]
> In on-demand mode the banner layout gets sluggish and **may crash** while scrolling. For a
> very large list in one folder, switch to `menu_grid_type = square_icons`. Filenames,
> last-played and default-folder all still work in that layout.

**Tips**

- Keep folders under 128 files for instant scrolling.
- Split big libraries into subfolders (genre, favourites, next-to-play).
- For a large library you want to keep in one folder, use the cube layout.

The 128 limit is a fail-safe. It can be raised in code, but that risks out-of-memory errors.
