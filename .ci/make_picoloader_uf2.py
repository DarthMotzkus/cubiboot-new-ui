#!/usr/bin/env python3
"""Build a PicoLoader .uf2 = PicoLoader firmware + a GameCube payload (.iso or .dol).

Replicates makeo's PicoLoader web converter
(https://makeo.github.io/PicoLoader/converter/): the payload is written as UF2 blocks
at flash address 0x10031000 (PicoLoader's PAYLOAD region) for BOTH the RP2040
(0xe48bff56) and RP2350 (0xe48bff59) family ids, then concatenated with the PicoLoader
firmware .uf2 and the block numbers are reindexed per family.

Usage: make_picoloader_uf2.py <firmware.uf2> <payload.iso|.dol> <out.uf2>
"""
import struct
import sys

UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY = 0x00002000
PAYLOAD_SIZE = 256          # data bytes per UF2 block
PAYLOAD_BASE = 0x10031000   # PicoLoader PAYLOAD region (see picoloader/memmap_data.ld)
FAMILIES = (0xE48BFF56, 0xE48BFF59)  # RP2040, RP2350


def parse_uf2(data):
    blocks = []
    for off in range(0, len(data), 512):
        blk = data[off:off + 512]
        if len(blk) < 512:
            break
        m0, m1, flags, addr, size, _bno, _nblk, fam = struct.unpack_from("<8I", blk, 0)
        if m0 != UF2_MAGIC0 or m1 != UF2_MAGIC1:
            raise SystemExit("bad UF2 magic in firmware image")
        blocks.append({"flags": flags, "addr": addr, "fam": fam, "data": blk[32:32 + size]})
    return blocks


def make_payload_blocks(payload):
    blocks = []
    for fam in FAMILIES:
        nblk = (len(payload) + PAYLOAD_SIZE - 1) // PAYLOAD_SIZE
        for i in range(nblk):
            chunk = payload[i * PAYLOAD_SIZE:(i + 1) * PAYLOAD_SIZE]
            blocks.append({"flags": UF2_FLAG_FAMILY,
                           "addr": PAYLOAD_BASE + i * PAYLOAD_SIZE,
                           "fam": fam, "data": chunk})
    return blocks


def emit(blocks):
    counts = {}
    for b in blocks:
        counts[b["fam"]] = counts.get(b["fam"], 0) + 1
    idx = {}
    out = bytearray()
    for b in blocks:
        fam = b["fam"]
        bno = idx.get(fam, 0)
        idx[fam] = bno + 1
        data = b["data"][:PAYLOAD_SIZE]
        blk = struct.pack("<8I", UF2_MAGIC0, UF2_MAGIC1, b["flags"] | UF2_FLAG_FAMILY,
                          b["addr"], len(data), bno, counts[fam], fam)
        blk += data + b"\x00" * (476 - len(data))
        blk += struct.pack("<I", UF2_MAGIC_END)
        assert len(blk) == 512
        out += blk
    return out


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    firmware = open(sys.argv[1], "rb").read()
    payload = open(sys.argv[2], "rb").read()
    fw_blocks = parse_uf2(firmware)
    pl_blocks = make_payload_blocks(payload)
    out = emit(fw_blocks + pl_blocks)
    open(sys.argv[3], "wb").write(out)
    print(f">> wrote {sys.argv[3]} ({len(out)} bytes): "
          f"{len(fw_blocks)} firmware + {len(pl_blocks)} payload blocks "
          f"(payload {len(payload)} B at 0x{PAYLOAD_BASE:08x})")


if __name__ == "__main__":
    main()
