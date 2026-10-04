#!/usr/bin/env bash
# Build cubiboot.iso — the bootable GameCube disc image for GC Loader (and other
# ODEs / Dolphin with a real IPL configured).
#
# Mechanism (from makeo/cubeboot-tools): a GameCube El-Torito ISO9660 image where
# the boot catalog header is a prebuilt `gbi.hdr` (GC disc header + apploader)
# and the El-Torito boot image is the cubiboot loader .dol. GC Loader reads the
# .dol straight off the disc and runs it.
#
#   mkisofs -R -J -G gbi.hdr -no-emul-boot -boot-load-seg 0 -b cubeboot.dol -o cubiboot.iso disc/
#
# The header is .ci/noipl/gbi_noipl.hdr: cubeboot-tools' apploader built with
# PATCH_IPL=3 + IGNORE_BOOT_MODE=1, which patches the factory boot animation out of
# the running stock IPL (see .ci/noipl/README.md). That matters for every way this
# disc is started: a GC Loader is a drive replacement, so the console's own IPL
# always runs first, plays its animation, and only then loads the disc -- as
# boot.iso at power-on, or from the GC Loader menu. Without the patch users saw
# that animation and then cubiboot's own, back to back. Started from Swiss instead,
# the stock IPL is not in RAM and the patch is a no-op (same fail-safe as an
# unknown IPL revision). The classic gbi.hdr from cubeboot-tools is NOT used.
#
# Requires (in PATH): genisoimage (provides mkisofs) and a built
# cubeboot/cubeboot.dol. Produces <repo>/cubiboot.iso, plus cubiboot-noipl.iso
# as an identical copy -- the name the PicoLoader uf2 step in ci.yml consumes.
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GBI_HDR="${GBI_HDR:-$REPO/.ci/noipl/gbi_noipl.hdr}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

[ -f "$REPO/cubeboot/cubeboot.dol" ] || { echo "ERROR: cubeboot/cubeboot.dol not built" >&2; exit 1; }
[ -f "$GBI_HDR" ]                    || { echo "ERROR: gbi.hdr not found at $GBI_HDR" >&2; exit 1; }

# Re-brand the disc-intro banner baked into the header: drop in the cubeboot banner
# pixels from default_opening.bin and set the text to "Cubiboot" / "Games Loader",
# replacing the stock gc-linux "Game Play" banner the BIOS would otherwise show.
BRANDED_HDR="$WORK/gbi.cubiboot.hdr"
python3 "$REPO/.ci/brand_gbi.py" "$GBI_HDR" "$REPO/patches/data/default_opening.bin" "$BRANDED_HDR"

# Disc directory tree: the loader .dol is the El-Torito boot image.
mkdir -p "$WORK/disc"
cp "$REPO/cubeboot/cubeboot.dol" "$WORK/disc/cubeboot.dol"

genisoimage -R -J \
    -G "$BRANDED_HDR" \
    -no-emul-boot -boot-load-seg 0 -b cubeboot.dol \
    -o "$REPO/cubiboot.iso" \
    "$WORK/disc"

echo ">> wrote $REPO/cubiboot.iso ($(stat -c%s "$REPO/cubiboot.iso") bytes)"

# Same disc under the name the PicoLoader uf2 step expects. Kept as a copy rather than
# a second genisoimage run so the two can never drift apart again.
cp "$REPO/cubiboot.iso" "$REPO/cubiboot-noipl.iso"
echo ">> wrote $REPO/cubiboot-noipl.iso (copy of cubiboot.iso)"
