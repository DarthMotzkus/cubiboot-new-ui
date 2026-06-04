# Fork changes over makeo/cubiboot

This fork = pristine **makeo/cubiboot `main`** + the `OffBroadway/cubeboot@custom-loader-menu`
banner-grid layout + a cold-boot banner-corruption fix + Cubiboot branding + a CI that
releases all artifacts. This document records every change so it can be re-applied onto a
fresh makeo clone.

## A. custom-loader-menu banner layout (cherry-picked)

The banner grid is the upstream `OffBroadway/cubeboot@custom-loader-menu` feature, applied
to makeo by cherry-picking exactly these commits (oldest → newest):

```
92d75b1  Add variable for number of columns in grid
0d6bb5e  Add `menu_grid_type` .ini setting
1b6d148  Add layout and cube scaling for a grid of banners
cb9889a  Update asset loading to handle many column counts
5a631df  Add an alternative 4x4 grid of small banners
831023e  Move `menu_grid_type_t` into its own file
2adc77d  Extrude the save cube's vertices
e918477  Reduce the rotation used for banner-width icons
```

Two trivial merge conflicts resolved: kept makeo's `__attribute_data_empty__` on
`browser_lines` while adding CLM's `columns_per_line` (`patches/source/grid.c`); combined
CLM's `menu_grid_type` `set_patch_value` line with makeo's commented block
(`cubeboot/source/main.c`).

## B. Cold-boot banner-corruption fix  (`patches/source/games.c`)

**Root cause:** the banner/icon buffer pools live in the custom `.data_lowmem` section
(`attr.h`: `__attribute_aligned_data_lowmem__` = `section(".data_lowmem")`), which
PicoBoot/gekko does **not** zero on cold boot. So `gm_banner_pool[].used` came up as random
garbage → `gm_get_banner_buf` either aliased two banners onto one buffer (corruption) or
found none free (blank), worse the colder the RAM. Only `.iso` (disc) banners were affected;
`.dol`/program entries set `use_banner=false` and never allocate a banner buffer.

Fix:
1. **Zero the pools at startup** (the actual root fix): `gm_init_heap()` does
   `memset(gm_banner_pool, 0, …)` + `memset(gm_icon_pool, 0, …)` before any asset loads.
2. **Keep banners resident in MRAM** (the async per-scroll ARAM unload/reload amplified the
   random-`used` bug into corruption): `gm_banner_setup`/`gm_banner_setup_unload` just set
   `state = GM_LOAD_STATE_LOADED` (no MRAM↔ARAM DMA); banners display straight from their
   resident buffers.

## C. Scalable past the 128-buffer pool  (`patches/source/games.c`)

`ASSET_BUFFER_COUNT = 128`. ≤128 banners/folder stay fully resident (the proven path,
unchanged). When a folder has **more** than 128, a sliding window engages:
- `gm_evict_on_scroll` flips true once the pool fills during `gm_check_files`.
- While true: `gm_line_free` releases off-screen lines and `gm_line_load` re-reads
  on-screen lines from disc via `gm_load_banner(entry, 0, false, /*use_cache=*/false)`.
- The `use_cache=false` re-reads **bypass makeo's `bnr_cache`** so scroll re-reads never
  touch ARAM (ARAM is the corruption path). `bnr_cache` is kept intact for the ≤128 path.
- `gm_load_banner` guards against re-loading an already-LOADED banner (no buffer leak), and
  `gm_line_changed` frees before it loads so the window always has a pool buffer.

## D. Branding

- **Menu header** = `"Games"` (`patches/source/menu.c`, `custom_gameselect_menu`).
- **Menu / loader banner** = "Cubiboot" / "Games Loader" with the cubeboot banner image —
  baked into `patches/data/default_opening.bin` via `.ci/brand_opening.py`.
- **`.iso` BIOS-intro banner** = the cubeboot banner + "Cubiboot" / "Games Loader" (replaces
  the stock gc-linux "Game Play" banner) — `.ci/brand_gbi.py` patches `gbi.hdr` at iso-build
  time (invoked from `.ci/build_iso.sh`), pulling the banner pixels from
  `default_opening.bin`.
- **`small_banners` is the in-code default** (`cubeboot/source/settings.c`
  `load_settings()`), so the layout works even without a `config.ini`; the `.ini` can still
  override `menu_grid_type`.

## E. CI / release  (`.github/workflows/ci.yml`, `.ci/`)

On push it builds in the reproducible `cubiboot-dev` Docker image (`.ci/Dockerfile`):
`ipl.dol`, `apploader.img` (`.ci/build_apploader.sh`), and `cubiboot.iso`
(`.ci/build_iso.sh`, branded). It then generates `config.ini`
(`[cubeboot]\n\nmenu_grid_type = small_banners`) and `EXTRACT_TO_ROOT.zip`
(= `ipl.dol` + `config.ini` + `swiss/patches/apploader.img`), and builds
`cubiboot_picoloader.uf2` — PicoLoader firmware (`makeo.github.io/PicoLoader/fw/picoloader.uf2`)
with `cubiboot.iso` embedded at flash `0x10031000` for both RP2040/RP2350 family ids, via
`.ci/make_picoloader_uf2.py` (replicates makeo's PicoLoader converter). On a `v*` tag it
publishes a GitHub Release with all six artifacts.

## Re-applying onto a fresh makeo clone

1. `git clone https://github.com/makeo/cubiboot && cd cubiboot`
2. Add the cubeboot remote, fetch, cherry-pick the 8 commits in section A (resolve the 2
   conflicts as noted).
3. Apply the `games.c` fix (B) + scalable (C), the branding (D), and the `.ci/` + workflow (E).
4. Run `brand_opening.py` once on `patches/data/default_opening.bin` (force-add it; it is
   `*.bin`-gitignored).
