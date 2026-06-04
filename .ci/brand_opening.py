#!/usr/bin/env python3
"""
brand_opening.py — set the BNRDesc text inside patches/data/default_opening.bin.

default_opening.bin is the cubeboot default banner (a BNR1) compiled into the loader
(default_opening_bin.h) and shown on the menu cube via banner_pointer. Its desc text
is what appears under the banner — stock it reads "Cubeboot Loader" / "Team
OffBroadway". This rewrites it to the cubiboot branding.

BNR1 layout: 0x20 header + 6144 px + one BNRDesc @ 0x1820:
  gameName     0x1820 (0x20)   <- title line   (short)
  company      0x1840 (0x20)   <- maker line   (short)
  fullGameName 0x1860 (0x40)   <- title line   (full)
  fullCompany  0x18A0 (0x40)   <- maker line   (full)
  description  0x18E0 (0x80)   <- info text    (left as-is)

Usage: brand_opening.py <default_opening.bin>   (patched in place)
"""
import sys

DESC = 0x1820
TITLE = b"Cubiboot"
MAKER = b"Games Loader"
FIELDS = [
    (0x00, 0x20, TITLE),   # gameName
    (0x20, 0x20, MAKER),   # company
    (0x40, 0x40, TITLE),   # fullGameName
    (0x80, 0x40, MAKER),   # fullCompany
]


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    path = sys.argv[1]
    buf = bytearray(open(path, "rb").read())
    if buf[0:4] != b"BNR1":
        raise SystemExit("not a BNR1 file — aborting")
    for off, length, text in FIELDS:
        base = DESC + off
        buf[base:base + length] = b"\x00" * length
        n = min(len(text), length - 1)
        buf[base:base + n] = text[:n]
    open(path, "wb").write(buf)
    print(f">> {path}: title '{TITLE.decode()}' / maker '{MAKER.decode()}'")


if __name__ == "__main__":
    main()
