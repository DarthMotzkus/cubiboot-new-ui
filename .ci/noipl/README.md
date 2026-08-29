# gbi_noipl.hdr — no-animation boot header (PicoLoader payload only)

A prebuilt GameCube "generic boot image" header, identical in role to the classic
`gbi.hdr` from cubeboot-tools that `cubiboot.iso` is built with, except its embedded
apploader is compiled with **`PATCH_IPL=3`**: after loading the boot DOL it patches the
running stock IPL in RAM to **skip and hide the factory boot animation**. It exists for
one artifact only — `cubiboot_picoloader_payload.uf2` — where the stock IPL actually
runs before cubiboot (PicoLoader boots the payload like a disc), so without this header
the console shows two boot animations back to back: the factory one, then cubiboot's.

`cubiboot.iso` (GC Loader, Method 3) keeps being built from the classic
`gbi.hdr` and is byte-for-byte unaffected; there the disc is booted from an ODE menu
(Swiss), no stock IPL is present at those addresses, and the patch would be a no-op
anyway — the same fail-safe that protects unknown IPL revisions (see below).

## Behavior

- On the IPL revisions in the patch table (retail NTSC 1.0/1.1/1.2, PAL 1.0/1.2,
  MPAL 1.1, plus DEV/TDEV — NTSC covers JPN; verified on a JPN 1.1 console) the factory
  animation is skipped entirely: power-on goes straight to cubiboot's own animation.
- **Holding A during power-on shows the factory animation** (`DISABLE_A_SKIP=0`), kept
  as an escape hatch / proof the patch is conditional.
- `IGNORE_BOOT_MODE=1`: the suppression also applies when the SRAM "boot to menu" flag
  is set (e.g. after a reset), matching PicoLoader's own staged behavior.
- Unknown IPL revision, or instructions that don't match what the patcher expects:
  every patch is a no-op and the animation simply plays as before. No crash path.

## Provenance / how to regenerate

Built from [makeo/cubeboot-tools](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
at commit `a68d82e` ("support more ipl versions"), with three edits to
`ppc/apploader/apploader.c`:

```
#define PATCH_IPL 3          (was 1)
#define IGNORE_BOOT_MODE 1   (was 0)
+#include <stdbool.h>        (before <stddef.h> — PATCH_IPL>2 does not compile without it)
```

Then, inside the `cubiboot-dev` image (`DEVKITPPC=/opt/devkitpro/devkitPPC`):

```
make -C ppc/common && make -C ppc/apploader       # -> apploader.bin (entry 0x81200000)
make -C mkgbi                                     # build apploader BEFORE mkgbi: its
                                                  # Makefile rebuilds ../common/lib.o
                                                  # with the host gcc
./mkgbi/mkgbi -a ppc/apploader/apploader.bin \
              -b <repo>/patches/data/default_opening.bin -o gbi_noipl.hdr
```

The banner baked in here is a placeholder: `.ci/build_iso.sh` re-brands it on every
build via `brand_gbi.py` (which locates the BNR1 block by search — its offset is
0x4CA0 here vs 0x43C0 in the classic header, because mkgbi places the banner right
after the apploader).

Hardware-validated 2026-08-29 (PicoLoader + retail console): single boot animation,
A-hold fallback working. The apploader region of this file is byte-identical to that
tested build.

```
sha256 gbi_noipl.hdr        ced166161a98713a4ed56ef996f3e6ea2006d2beb6a3f23545f657b1751733c4
sha256 apploader (10236 B)  b4c7fb91197d155abe19b355c15d885cc1019c40c76a633bb6f07560db44358e
```
