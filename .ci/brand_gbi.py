#!/usr/bin/env python3
"""
brand_gbi.py — rebrand the opening.bnr embedded in a GameCube "generic boot image"
header (gbi.hdr) so cubiboot.iso shows cubeboot branding in the console BIOS intro
instead of the stock gc-linux "Game Play" banner.

The console IPL/BIOS reads the disc's opening.bnr (baked into gbi.hdr) to draw the
"press start / press A" intro cube. mkgbi (cubeboot-tools) embeds the gc-linux banner
there. We overwrite, in place:

  * the 96x32 RGB5A3 banner image  -> the cubiboot loader banner (the same one the
                                      menu shows on the cube, default_opening.bin)
  * the BNRDesc text fields        -> copied from that same banner, so the .iso says
                                      whatever brand_opening.py stamped and there is
                                      no second copy of the strings to keep in step

mkgbi places the banner right after the apploader, so its absolute offset moves with
the apploader's size (0x43C0 in the classic gbi.hdr, 0x4CA0 in the PATCH_IPL=3
no-animation header). The BNR1 magic is therefore located by search; the pixelData
(+0x20, 96*32*2 = 6144 bytes) and desc[0] (+0x1820) offsets are fixed relative to it
by the opening.bnr format itself.

The banner source may be either:
  * a full opening.bnr (BNR1/BNR2) — e.g. patches/data/default_opening.bin — whose
    96x32 RGB5A3 pixelData (at file offset 0x20) is copied across verbatim, or
  * a raw 32x32 RGB5A3 texture (e.g. dol_tex.bin) — centred into the 96x32 banner.
Both are GX-tiled RGB5A3 (4x4-pixel tiles, 32 bytes each, left-to-right then top-to-
bottom); for the 32x32 case the logo is exactly 8x8 tiles and drops in at tile column
8 with whole-tile copies, so no per-pixel rescaling is needed.

Usage: brand_gbi.py <in_gbi.hdr> <banner_src.bnr|dol_tex.bin> <out_gbi.hdr>
"""
import sys

PIXELDATA_LEN = 96 * 32 * 2          # 6144
BNR_PIXEL_OFF = 0x20                 # pixelData offset inside an opening.bnr

BNR_DESC_OFF  = 0x1820               # desc[0] offset inside an opening.bnr
DESC_LEN      = 0x140                # one BNRDesc

BANNER_TILES_W = 96 // 4             # 24
LOGO_TILES_W   = 32 // 4             # 8
TILES_H        = 32 // 4             # 8
LOGO_TILE_COL  = (BANNER_TILES_W - LOGO_TILES_W) // 2   # 8 -> centred
TILE_BYTES     = 32


def build_banner(src: bytes) -> bytes:
    # Full opening.bnr (BNR1/BNR2): copy its 96x32 pixelData straight across.
    if src[0:3] == b"BNR" and len(src) >= BNR_PIXEL_OFF + PIXELDATA_LEN:
        return src[BNR_PIXEL_OFF:BNR_PIXEL_OFF + PIXELDATA_LEN]

    # Otherwise treat it as a raw 32x32 RGB5A3 texture and centre it in the banner.
    if len(src) < LOGO_TILES_W * TILES_H * TILE_BYTES:
        raise SystemExit("banner source is neither a BNR nor a 32x32 RGB5A3 texture")

    # Background = the logo's top-left pixel (first tile, first texel) so the banner
    # reads as a horizontal extension of the icon rather than an abrupt border.
    bg = src[0:2]
    banner = bytearray(bg * (PIXELDATA_LEN // 2))

    for ty in range(TILES_H):
        for sx in range(LOGO_TILES_W):
            soff = (ty * LOGO_TILES_W + sx) * TILE_BYTES
            dst = (ty * BANNER_TILES_W + (LOGO_TILE_COL + sx)) * TILE_BYTES
            banner[dst:dst + TILE_BYTES] = src[soff:soff + TILE_BYTES]

    assert len(banner) == PIXELDATA_LEN
    return bytes(banner)


def patch_desc(buf: bytearray, desc_off: int, src: bytes):
    """Copy the banner's own text across, rather than restating it here.

    The strings used to be duplicated between this script and brand_opening.py, which
    meant every wording change had to be made twice and could silently drift. Now the
    .bin is the single source and this just carries it over.
    """
    if len(src) < BNR_DESC_OFF + DESC_LEN:
        raise SystemExit("banner source has no BNRDesc to copy — pass an opening.bnr")

    buf[desc_off:desc_off + DESC_LEN] = src[BNR_DESC_OFF:BNR_DESC_OFF + DESC_LEN]


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    in_gbi, banner_src_path, out_gbi = sys.argv[1:4]

    buf = bytearray(open(in_gbi, "rb").read())
    bnr_off = buf.find(b"BNR1")
    if bnr_off < 0 or len(buf) < bnr_off + BNR_DESC_OFF + DESC_LEN:
        raise SystemExit("no complete BNR1 banner found — gbi.hdr layout unexpected, aborting")
    pixel_off = bnr_off + BNR_PIXEL_OFF
    desc_off = bnr_off + BNR_DESC_OFF

    banner_src = open(banner_src_path, "rb").read()
    buf[pixel_off:pixel_off + PIXELDATA_LEN] = build_banner(banner_src)
    patch_desc(buf, desc_off, banner_src)

    line1 = buf[desc_off:desc_off + 0x20].split(b"\x00")[0].decode("latin-1")
    line2 = buf[desc_off + 0x20:desc_off + 0x40].split(b"\x00")[0].decode("latin-1")
    open(out_gbi, "wb").write(buf)
    print(f">> wrote {out_gbi} ({len(buf)} bytes): banner + '{line1}' / '{line2}' branded")


if __name__ == "__main__":
    main()
