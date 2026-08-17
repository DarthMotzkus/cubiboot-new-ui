# CLAUDE.md

Orientation for agents working in this repo. Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
before touching anything under `cubeboot/`, `patches/` or `entry/` — the layout is not
guessable from the directory names.

## What this is

cubiboot replaces the GameCube's IPL (the BS2 boot program). It shows a grid of banners for
the games on an SD card and chainloads [Swiss](https://github.com/emukidid/swiss-gc) to run
them. Fork chain: `OffBroadway/cubeboot` → `makeo/cubiboot` → this repo.

## The one thing to internalise

**There are two programs, and the menu is not in the one you would expect.**

| | `cubeboot.dol` (the loader) | `patches.elf` (the menu) |
|---|---|---|
| Source | `cubeboot/source/` | `patches/source/` |
| Runs as | a normal DOL, with libogc | code injected into the stock BIOS |
| Job | read `config.ini`, load the IPL, patch it, jump to BS2 | everything the user sees: grid, banners, booting games |
| Has libogc? | yes | **no** — freestanding, `-DIPL_CODE` |
| Ends up | gzipped inside `entry.dol`; also shipped raw as `ipl.dol`/`flippydrive.dol` | embedded as a blob inside `cubeboot.dol` |

Adding a "menu feature" almost always means editing `patches/`, not `cubeboot/`.

Code that both sides need lives in **`cubeboot/source/emu/`** and is **copied to
`patches/source/emu/` at build time** by `entry/Makefile`. That copy is gitignored — never
edit `patches/source/emu/`, your changes will be overwritten. That directory is how the FAT
stack, the EXI SD driver (`tsd.c`), the ODE backend (`gcode.c`) and the FlippyDrive backend
(`fldrv.c`) serve both programs from one source. (A FlippyDrive is NOT an ODE: an ODE
replaces the optical drive, the flippy rides its ribbon beside it — it keeps the disc
screen, has its own `flippy` device_order entry, and gets IGR via `IGRType=Reboot`.)

## Repo map

```
entry/              stage 1: gzip stub linked at 0x81300000; output entry.dol is the
                    PicoBoot payload (.ci/make_picoboot_uf2.py). The released ipl.dol is
                    cubeboot.dol itself (see ci.yml), NOT entry.dol.
cubeboot/source/    stage 2: the loader
  emu/              SHARED with patches (copied at build time)
    ffs/            FatFs (ChaN) + diskio glue
    tsd.c           SD over EXI (SD2SP2, SD Gecko)
    gcode.c         SD inside a GC Loader style ODE, over the drive interface
    fldrv.c         FlippyDrive native file protocol; not a FatFs volume
    drive_probe.c   one OEM inquiry decides who answers the drive bus, shared by both
    flippy_emu.c    the dvd_custom_* file API both sides call; picks the device
  boot/             sidestep / ARAM helpers from iplboot
patches/source/     stage 3: the injected menu
  games.c           game enumeration, banner/icon pools, last-played
  menu.c grid.c     the UI
  theme.c           all UI colour derivation (config.ini -> cubes, info box, PRESS START)
  linker/           one link script per IPL revision
.ci/                scripts the GitHub workflow runs
.localbuild/        local build wrappers (gitignored, comments in Portuguese)
docs/               ARCHITECTURE.md, FORK_CHANGES.md, settings.md, README_es.md
```

## Building

The only supported build is the pinned Docker image, inside WSL:

```sh
bash .localbuild/build.sh       # -> dist/IPL.dol + dist/apploader.img
bash .localbuild/build_iso.sh   # -> the above + dist/IPL.iso
```

`make clean` is **not optional** and the wrappers already do it: the `data/cubeboot.gz`
target in `entry/Makefile` has no dependencies, so an incremental build silently ignores
every change under `patches/` and `cubeboot/source/`. A build that "succeeds" without
picking up your edits is the classic failure here.

CI runs the same steps from `.ci/` on every branch — see `.github/workflows/ci.yml`.

## Hard rules

- **Never mask the build's exit code** with `| tail`, `| head` or `; echo`. Redirect to a
  file and read it afterwards.
- **Neither `ipl.dol` nor `cubiboot.iso` runs in Dolphin**, even with an IPL.bin set. Don't
  try to validate a change that way.
- **Commits are authored as `Richard "Darth" Motzkus`.** Pinned in the repo's local
  `.git/config`. Pushing needs `HOME=/home/richard` (Windows Git Credential Manager; there is
  no SSH key).
- **`gh` is installed but not logged in**, and reading the credential store to log it in gets
  denied. The repo is public, so read-only checks need no auth at all — `curl` the API for
  release and run status, e.g.
  `curl -sfL https://api.github.com/repos/DarthMotzkus/cubiboot-new-ui/releases/tags/<tag>`.
  Authenticated `gh` needs `gh auth login` in an interactive session, or `GH_TOKEN` in the env.
- **Verify before claiming.** This is bare-metal code with no test suite and no emulator. If
  you can't run it, say so — disassembling the built `patches.elf` and comparing against a
  known-good build is the strongest evidence available, and it is cheap:
  `powerpc-eabi-objdump -d -j .text.<fn> patches/patches.elf` inside the image.
- `USE_FAT_*` in `cubeboot/source/config.h` looks like a live knob. It isn't — three of the
  four options reference source directories that no longer exist. Leave it on
  `USE_FAT_LIBFLIPPY`.

## Where to look

| Question | File |
|---|---|
| How does a setting get from `config.ini` into the menu? | `cubeboot/source/settings.c` → `main.c` (`set_patch_value`) → `patches/source/main.c` |
| How is the stock BIOS patched? | `cubeboot/source/main.c`, the `.patch.*` / `.reloc` walk |
| How are files read? | `cubeboot/source/emu/flippy_emu.c` (`dvd_custom_*`) |
| Which storage device is used? | `flippy_emu.c` `device_prio[]` + `emu_sd_device` |
| Where do the menu's colours come from? | `patches/source/theme.c` (loader only parses; see `docs/settings.md`) |
| Why is a banner blank or corrupted? | `docs/FORK_CHANGES.md` §B and §C |
| What did this fork change vs upstream? | `docs/FORK_CHANGES.md` |
