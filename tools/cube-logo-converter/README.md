# Cube logo converter

Turns an ordinary image into the file the [`cube_logo`](../../docs/settings.md) setting
expects: the custom art that replaces the "GAMECUBE" text under the cube during the boot
animation.

The console side reads **raw linear RGBA8, exactly 352×40 px (56,320 bytes)** — not a PNG.
The firmware no longer carries a PNG decoder (both upng and ok_png were dropped upstream to
save memory in the injected patch), so the pixel conversion happens on the PC and the patch
only does the GX texture tiling. A wrong-sized file is rejected and the stock text is shown,
so a bad conversion can never break the boot.

## Making a logo

Any image editor works; for text logos,
[fontmeme.com/gamecube-font](https://fontmeme.com/gamecube-font/) renders your text in
GameCube-style fonts and exports a PNG.

**Whatever you use to draw the logo, the PNG still has to be converted here.** This fork's
firmware reads only the raw format described above — a `.png` on the SD card is rejected
(instructions elsewhere saying `cube_logo = /logo.png` are for the original cubeboot, whose
PNG decoder no longer exists in this fork).

## Option 1: the web page (no install)

**[Open the converter](https://htmlpreview.github.io/?https://github.com/DarthMotzkus/cubiboot-new-ui/blob/main/tools/cube-logo-converter/index.html)** — it runs in the browser on Windows,
Linux and macOS, and nothing is uploaded anywhere. Drop your image in, pick the options,
check the live preview and download the `.raw`.

That link renders this folder's `index.html` as a page. Linking the file inside the repo
instead would only show its source, because GitHub serves `.html` as text. Offline, opening
your local copy of `index.html` in a browser works exactly the same.

## Option 2: the command line script

```sh
pip install Pillow
python3 png2cubelogo.py logo.png              # -> logo.raw
```

Flags: `--stretch` fills the target box instead of fitting with the aspect ratio kept;
`--full` targets the whole 352×40 canvas instead of the stock letter box (see below).

## Installing the result

Copy the `.raw` to the SD card and point `config.ini` at it:

```ini
[cubeboot]
cube_logo = /logo.raw
```

## Why "match stock logo" is the default

The stock "GAMECUBE™" art does not fill its texture: the letters occupy a 316×33 box at
+6,+3, with the ™ sign taking the right edge, and the boot screen 3D model is positioned to
compensate for exactly that layout. Art centered on the full canvas therefore shows up
shifted right on screen. Both tools default to placing your art inside the stock letter box,
which is what looks centered during boot; use the full-canvas mode only if you want your art
slightly larger and don't mind re-checking the centering on a real console.

Transparency is kept (the boot background shows through), and a fully colored logo is used
as-is — it is no longer tinted by `cube_color` the way the stock intensity texture is.
