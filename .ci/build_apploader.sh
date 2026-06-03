#!/usr/bin/env bash
# Build apploader.img (the Swiss In-Game-Reset redirect) embedding the freshly
# built cubeboot loader.
#
# Mechanism (reverse-engineered from makeo's release apploader.img):
#   apploader.img = [0x20 GameCube-apploader header] + [packer stub + XZ payload]
#     header: 16B version string "2004/02/01" + 8B zero + 4B body-size(BE) + 4B CRC32(body)(BE)
#     body  : swiss-gc cube/packer output, REBOOT variant (.init = 0x01300000)
#     payload: cubeboot loader, flat binary, XZ-compressed with the PPC BCJ filter,
#              self-extracted to 0x80003100 (== cubeboot entry point) and run.
#
# Requires (in PATH/env): devkitPPC (DEVKITPPC), xz, python3, git, and a built
# cubeboot/cubeboot.elf. Produces <repo>/apploader.img.
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PFX="${DEVKITPPC}/bin/powerpc-eabi-"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

[ -f "$REPO/cubeboot/cubeboot.elf" ] || { echo "ERROR: cubeboot/cubeboot.elf not built" >&2; exit 1; }

# 1) fetch only the swiss-gc packer
git clone --depth 1 --filter=blob:none --sparse https://github.com/emukidid/swiss-gc.git "$WORK/swiss-gc"
git -C "$WORK/swiss-gc" sparse-checkout set cube/packer

# 2) payload = the cubeboot loader ELF (packer reads ../swiss/swiss.elf)
mkdir -p "$WORK/swiss-gc/cube/swiss"
cp "$REPO/cubeboot/cubeboot.elf" "$WORK/swiss-gc/cube/swiss/swiss.elf"

# 3) packer compresses with 7z by default; swap to xz + PPC BCJ (decoder is generic)
cd "$WORK/swiss-gc/cube/packer"
sed -i 's#.*7z a .*#\t$(SILENTCMD)xz -k -c -F xz --powerpc --lzma2=preset=9e --check=crc32 $< > $@#' Makefile

# 4) build (reboot.elf is the variant linked at 0x01300000 that makeo's apploader uses)
make

# 5) raw body + GameCube-apploader header
"${PFX}objcopy" -O binary reboot.elf "$WORK/body.bin"
python3 - "$WORK/body.bin" "$REPO/apploader.img" <<'PY'
import sys, zlib, struct
body = open(sys.argv[1], "rb").read()
ver  = b"2004/02/01"; ver += b"\x00" * (16 - len(ver))
hdr  = ver + b"\x00" * 8 + struct.pack(">I", len(body)) + struct.pack(">I", zlib.crc32(body) & 0xffffffff)
open(sys.argv[2], "wb").write(hdr + body)
print("apploader.img: %d bytes (body=%d, crc32=%#010x)" % (len(hdr) + len(body), len(body), zlib.crc32(body) & 0xffffffff))
PY
echo ">> wrote $REPO/apploader.img"
