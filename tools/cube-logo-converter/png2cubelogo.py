#!/usr/bin/env python3
"""Convert an image into the raw RGBA8 file the cube_logo setting expects.

The output is raw linear RGBA8, exactly 352x40 px (56320 bytes), top-left origin.
The console has no PNG decoder (upng/ok_png were both dropped upstream), so the
pixel conversion happens on the PC and the patch only does the GX tiling.

ALIGNMENT: the stock "GAMECUBE(tm)" art does not fill the texture -- the letters
occupy x=[6..321], y=[3..35], with the (tm) sign at [324..345]. The boot screen
model is positioned for THAT geometry, so art centered on the full 352x40 canvas
shows up shifted right on screen. The default mode fits your art into the stock
letter box (316x33 at +6,+3), which is what looks centered during boot.
(Measured from the NTSC 1.1 IPL's I8 texture, Yay0 archive at 0xb5260.)

Usage:
  python3 png2cubelogo.py logo.png                 -> logo.raw (stock geometry, recommended)
  python3 png2cubelogo.py logo.png out.raw
  python3 png2cubelogo.py logo.png --full          -> use the whole 352x40 canvas
  python3 png2cubelogo.py logo.png --stretch       -> stretch to fill instead of fitting

Then copy the .raw to the SD card and point config.ini at it:
  [cubeboot]
  cube_logo = /logo.raw

Requires Pillow (pip install Pillow). For a no-install version, open
index.html in this folder in any browser.
"""

import sys
from pathlib import Path

from PIL import Image

WIDTH, HEIGHT = 352, 40
SIZE = WIDTH * HEIGHT * 4

# box occupied by the stock "GAMECUBE" letters in the texture (excluding the tm sign)
STOCK_X, STOCK_Y, STOCK_W, STOCK_H = 6, 3, 316, 33


def main() -> int:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    stretch = "--stretch" in sys.argv[1:]
    full = "--full" in sys.argv[1:]

    if not args:
        print(__doc__)
        return 1

    src = Path(args[0])
    dst = Path(args[1]) if len(args) > 1 else src.with_suffix(".raw")

    img = Image.open(src).convert("RGBA")

    # trim transparent margins baked into the art, so alignment follows the
    # visible content
    bbox = img.getchannel("A").getbbox()
    if bbox:
        img = img.crop(bbox)

    # target area: stock geometry (default) or the whole canvas (--full)
    if full:
        bx, by, bw, bh = 0, 0, WIDTH, HEIGHT
    else:
        bx, by, bw, bh = STOCK_X, STOCK_Y, STOCK_W, STOCK_H

    if stretch:
        new_w, new_h = bw, bh
    else:
        scale = min(bw / img.width, bh / img.height)
        new_w = max(1, round(img.width * scale))
        new_h = max(1, round(img.height * scale))
    scaled = img.resize((new_w, new_h), Image.LANCZOS)

    out = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    out.paste(scaled, (bx + (bw - new_w) // 2, by + (bh - new_h) // 2))

    data = out.tobytes("raw", "RGBA")
    assert len(data) == SIZE, f"unexpected size: {len(data)}"
    dst.write_bytes(data)

    mode = "stretch" if stretch else "fit"
    area = "full canvas" if full else f"stock geometry {bw}x{bh}@+{bx},+{by}"
    print(f"OK: {src} -> {dst} ({mode}, {area}, final art {new_w}x{new_h})")
    print("Copy it to the SD card and add to config.ini:  cube_logo = /" + dst.name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
