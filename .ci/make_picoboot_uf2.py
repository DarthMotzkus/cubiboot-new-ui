#!/usr/bin/env python3
"""Build cubiboot_picoboot_payload.uf2 from the cubiboot stage-1 DOL (entry/entry.dol).

PicoBoot (github.com/webhdx/PicoBoot) is the RP2040/RP2350 IPL-injection modchip:
the Pico streams a payload to the GameCube as if it were the scrambled BS2 ROM.
Its firmware DMA-streams the payload straight from flash offset 0x80000 (limit
1.5 MiB, per its memmap_picoboot.ld), expecting the shape built by PicoBoot's
tools/process_ipl.py, replicated here:

  [32B header: "IPLBOOT " + u32be total-size + 20B zero]
  [flattened DOL image, scrambled with the BS2 scrambler (keystream offset 0x720)]
  [4-byte alignment padding + unscrambled "PICO" tail tag]

The DOL must be linked AND have its entry point at 0x81300000 — which is exactly
where entry/nosys.ld links the cubiboot stage-1 stub. So the payload is
entry.dol (stub + gzipped cubeboot loader), not the released ipl.dol
(= cubeboot.dol, linked at 0x80003100, which PicoBoot cannot boot).

Output:
  cubiboot_picoboot_payload.uf2  universal payload-only update (RP2040 + RP2350
                                 family blocks interleaved). Flash it onto a Pico
                                 already running the official PicoBoot firmware
                                 >= v0.4 (Pico 2: >= v0.5.0, the first RP2350
                                 build); it replaces the stock gekkoboot payload
                                 in place, firmware untouched. Older firmware
                                 (<= v0.3.x) has no separate payload region and
                                 silently ignores this file — flash the official
                                 picoboot_full_*.uf2 first.

Usage:
  make_picoboot_uf2.py <entry.dol> <outdir>
"""

import math
import struct
import sys
from pathlib import Path

FAMILY_RP2040 = 0xE48BFF56
FAMILY_RP2350 = 0xE48BFF59  # ARM secure

PAYLOAD_FLASH_ADDR = 0x10080000
UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
CHUNK = 256


# BS2 (bootrom) descrambler reversed by segher; XOR keystream, so it is its own
# inverse. Identical to PicoBoot's tools/process_ipl.py.
def scramble(data):
    acc = 0
    nacc = 0
    t = 0x2953
    u = 0xD9C2
    v = 0x3FF1
    x = 1
    it = 0
    while it < len(data):
        t0 = t & 1
        t1 = (t >> 1) & 1
        u0 = u & 1
        u1 = (u >> 1) & 1
        v0 = v & 1

        x ^= t1 ^ v0
        x ^= u0 | u1
        x ^= (t0 ^ u1 ^ v0) & (t0 ^ u0)

        if t0 == u0:
            v >>= 1
            if v0:
                v ^= 0xB3D0

        if t0 == 0:
            u >>= 1
            if u0:
                u ^= 0xFB10

        t >>= 1
        if t0:
            t ^= 0xA740

        nacc = (nacc + 1) % 256
        acc = (acc * 2 + x) % 256
        if nacc == 8:
            data[it] ^= acc
            nacc = 0
            it += 1

    return data


def flatten_dol(data):
    header = struct.unpack(">64I", data[:256])
    offsets = header[:18]
    addresses = header[18:36]
    sizes = header[36:54]
    entry = header[56]

    dol_min = min(a for a in addresses if a)
    dol_max = max(a + s for a, s in zip(addresses, sizes))

    img = bytearray(dol_max - dol_min)
    for offset, address, size in zip(offsets, addresses, sizes):
        img[address - dol_min:address + size - dol_min] = data[offset:offset + size]

    return entry, dol_min, img


def build_payload(dol_path):
    """entry.dol -> the raw payload blob PicoBoot's firmware validates."""
    exe = bytearray(Path(dol_path).read_bytes())
    entry, load, img = flatten_dol(exe)
    entry = (entry & 0x017FFFFF) | 0x80000000
    load &= 0x017FFFFF

    print(f"  entry point:  0x{entry:08X}")
    print(f"  load address: 0x{load:08X}")
    print(f"  image size:   {len(img)} bytes")

    if entry != 0x81300000 or load != 0x01300000:
        raise SystemExit(
            f"ERROR: {dol_path} must be linked with entry point 0x81300000 "
            f"(got entry=0x{entry:08X} load=0x{load:08X}). PicoBoot takes "
            f"entry/entry.dol, not the released ipl.dol/cubeboot.dol."
        )

    # The scrambler keystream starts 0x720 bytes before the payload's position
    # in the BS2 ROM image; prime it with dummy bytes and drop them.
    img = scramble(bytearray(0x720) + img)[0x720:]

    img = img.ljust(math.ceil(len(img) / 4) * 4, b"\x00") + b"PICO"

    header = struct.pack("> 8s I 20x", b"IPLBOOT ", len(img) + 32)
    return header + img


def pack_uf2(data, base_address, family_ids):
    """Pack a blob as UF2 blocks; families interleaved per chunk (matches
    process_ipl.py's universal output)."""
    out = bytearray()
    total = math.ceil(len(data) / CHUNK)
    for seq in range(total):
        chunk = data[seq * CHUNK:(seq + 1) * CHUNK]
        for family in family_ids:
            out += struct.pack(
                "< 8I 476s I",
                UF2_MAGIC0, UF2_MAGIC1,
                0x00002000,  # flags: family ID present
                base_address + seq * CHUNK,
                CHUNK, seq, total, family,
                chunk, UF2_MAGIC_END,
            )
    return bytes(out)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <entry.dol> <outdir>")
        return 1

    dol, outdir = sys.argv[1:]
    outdir = Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    print(f"payload from {dol}:")
    payload = build_payload(dol)

    blob = pack_uf2(payload, PAYLOAD_FLASH_ADDR, [FAMILY_RP2040, FAMILY_RP2350])
    out = outdir / "cubiboot_picoboot_payload.uf2"
    out.write_bytes(blob)
    print(f"  wrote {out} ({len(blob)} bytes)")


if __name__ == "__main__":
    sys.exit(main())
