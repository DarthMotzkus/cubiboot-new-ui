#!/usr/bin/env bash
# Build cubiboot.iso — the bootable GameCube disc image for GC Loader (and other
# ODEs / Dolphin with a real IPL configured).
#
# Mechanism (from makeo/cubeboot-tools): a GameCube El-Torito ISO9660 image where
# the boot catalog header is the prebuilt `gbi.hdr` (GC disc header + apploader)
# and the El-Torito boot image is the cubiboot loader .dol. GC Loader reads the
# .dol straight off the disc and runs it.
#
#   mkisofs -R -J -G gbi.hdr -no-emul-boot -boot-load-seg 0 -b cubeboot.dol -o cubiboot.iso disc/
#
# Requires (in PATH): genisoimage (provides mkisofs) and a built
# cubeboot/cubeboot.dol. gbi.hdr comes from the cubeboot-tools checkout in the
# cubiboot-dev image (/opt/src/cubeboot-tools). Produces <repo>/cubiboot.iso.
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GBI_HDR="${GBI_HDR:-/opt/src/cubeboot-tools/mkgbi/gbi.hdr}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

[ -f "$REPO/cubeboot/cubeboot.dol" ] || { echo "ERROR: cubeboot/cubeboot.dol not built" >&2; exit 1; }
[ -f "$GBI_HDR" ]                    || { echo "ERROR: gbi.hdr not found at $GBI_HDR" >&2; exit 1; }

# Disc directory tree: the loader .dol is the El-Torito boot image.
mkdir -p "$WORK/disc"
cp "$REPO/cubeboot/cubeboot.dol" "$WORK/disc/cubeboot.dol"

genisoimage -R -J \
    -G "$GBI_HDR" \
    -no-emul-boot -boot-load-seg 0 -b cubeboot.dol \
    -o "$REPO/cubiboot.iso" \
    "$WORK/disc"

echo ">> wrote $REPO/cubiboot.iso ($(stat -c%s "$REPO/cubiboot.iso") bytes)"
