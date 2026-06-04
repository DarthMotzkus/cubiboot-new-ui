# Reproducible cubiboot build image (mirrors the local .localbuild/Dockerfile and
# the agreed definitions: GameCube-only, devkitPPC, libogc2/libfat pinned).
# Used by .github/workflows/ci.yml (docker build - < .ci/Dockerfile).
FROM devkitpro/devkitppc:20231110

RUN apt-get update && apt-get install -y \
        genisoimage git golang nodejs build-essential gcc-arm-none-eabi \
        python3-pip python3-distutils python3-setuptools curl xz-utils zip \
    && rm -rf /var/lib/apt/lists/*

# pip via apt (the image ships Python 3.9; current get-pip.py needs 3.10+)
RUN pip3 install pyelftools==0.28

# libogc2 + libfat (extremscorner forks) installed into devkitPro.
# Pin to --before=2026-01-20 (last commit before the 2026-01-26 thread bump that
# would require a newer devkitPPC than this :20231110 image).
#
# cubiboot is GameCube-only -> install ONLY the 'cube' target. The Wii build does
# not compile on this image (wiiuse needs sys/endian.h, removed since 2024-11-11)
# and is not needed. New libogc2 layout (post-2025-10): libogc2/gamecube/{lib,include}.
RUN git clone https://github.com/extremscorner/libogc2.git /opt/src/libogc2 \
    && cd /opt/src/libogc2 \
    && git checkout "$(git rev-list -1 --before=2026-01-20 HEAD)" \
    && make cube \
    && mkdir -p "$DEVKITPRO/libogc2/gamecube/lib" \
    && cp -fr include "$DEVKITPRO/libogc2/gamecube" \
    && cp -fr lib/cube/*.a "$DEVKITPRO/libogc2/gamecube/lib" \
    && cp -fr *_rules "$DEVKITPRO/libogc2" \
    && (cp -fr *_license.txt "$DEVKITPRO/libogc2" || true)

# libfat: cube target only. Merge into the same libogc2/gamecube layout.
RUN git clone https://github.com/extremscorner/libfat.git /opt/src/libfat \
    && cd /opt/src/libfat \
    && git checkout "$(git rev-list -1 --before=2026-01-20 HEAD)" \
    && sed -i '/rice/d' Makefile \
    && make cube-release \
    && cp -fr include libogc2/gamecube \
    && (cp -fr libfat_license.txt "$DEVKITPRO/libogc2" || true) \
    && cp -fr libogc2/gamecube "$DEVKITPRO/libogc2"

# cubeboot-tools (makeo): provides gbi.hdr (kept for parity; not required for .dol/apploader).
RUN git clone --depth 1 https://github.com/makeo/cubeboot-tools /opt/src/cubeboot-tools

WORKDIR /work
