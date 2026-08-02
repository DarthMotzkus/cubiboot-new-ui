#!/usr/bin/env python3
"""
brand_opening.py — set the BNRDesc text inside patches/data/default_opening.bin.

default_opening.bin is the cubiboot banner (a BNR1) compiled into the loader
(default_opening_bin.h, generated at build time) and shown on the menu cube via
banner_pointer. Its desc text is the two lines under the banner art, and it is the
single place they come from: brand_gbi.py copies this whole desc block into gbi.hdr,
so the .iso BIOS intro says the same thing without a second copy of the strings to
keep in step.

The version is an argument rather than a constant so CI can stamp it from the tag it
is building. Baking it in by hand is how a release ends up showing the previous
version's number on the console.

BNR1 layout: 0x20 header + 6144 px + one BNRDesc @ 0x1820:
  gameName     0x1820 (0x20)   <- first line   (short)
  company      0x1840 (0x20)   <- second line  (short)
  fullGameName 0x1860 (0x40)   <- first line   (full)
  fullCompany  0x18A0 (0x40)   <- second line  (full)
  description  0x18E0 (0x80)   <- info text

Usage: brand_opening.py <default_opening.bin> <version>
  e.g. brand_opening.py patches/data/default_opening.bin v1.6.0
"""
import sys

DESC = 0x1820
TITLE = b"GC Games and Apps Loader"


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)

    path, version = sys.argv[1], sys.argv[2]
    build = f"build {version}".encode("latin-1", errors="replace")

    buf = bytearray(open(path, "rb").read())
    if buf[0:4] != b"BNR1":
        raise SystemExit("not a BNR1 file — aborting")

    fields = [
        (0x00, 0x20, TITLE),   # gameName
        (0x20, 0x20, build),   # company
        (0x40, 0x40, TITLE),   # fullGameName
        (0x80, 0x40, build),   # fullCompany
        (0xC0, 0x80, build),   # description
    ]

    for off, length, text in fields:
        base = DESC + off
        buf[base:base + length] = b"\x00" * length
        n = min(len(text), length - 1)          # keep the NUL terminator
        buf[base:base + n] = text[:n]

    open(path, "wb").write(buf)
    print(f">> {path}: '{TITLE.decode()}' / '{build.decode()}'")


if __name__ == "__main__":
    main()
