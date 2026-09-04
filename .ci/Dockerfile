# Reproducible cubiboot build image (mirrors the local .localbuild/Dockerfile and
# the agreed definitions: GameCube-only, devkitPPC, libogc2/libfat pinned).
# Used by .github/workflows/ci.yml (docker build - < .ci/Dockerfile).
FROM devkitpro/devkitppc:20231110

# Debian 11 (bullseye) reached end of life on 2026-08-31, so the live mirrors are
# being purged while their indexes still advertise the packages: apt resolves a
# version and then gets a 404 on the .deb. It is rolling, not a fixed list -- the
# first failure seen was python3-setuptools 52.0.0-4+deb11u2, the next run got
# that one and 404ed on xz-utils instead -- and it is not transient, so the
# workflow's five docker build retries all fail on it.
#
# The live mirror is still tried first, so this behaves exactly as before while it
# is intact, and only on failure is the suite repointed at archive.debian.org,
# which is frozen and complete. Check-Valid-Until is off because the archived
# Release files are past their expiry date by design.
#
# bullseye-security is asked for but not required: the archive takes the security
# suite some time after EOL and until then its Release file 404s, which makes apt
# reject the whole update. So a failed update drops that line and retries against
# the plain bullseye suite (the 11.11 point release), which carries every package
# this image installs. The line starts working by itself once Debian archives the
# suite -- nothing here needs revisiting for that.
RUN set -eux; \
    PKGS="genisoimage git golang nodejs build-essential gcc-arm-none-eabi \
          python3-pip python3-distutils python3-setuptools curl xz-utils zip"; \
    echo 'Acquire::Retries "5";' > /etc/apt/apt.conf.d/80-cubiboot-retries; \
    echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/80-cubiboot-archive; \
    if ! (apt-get update && apt-get install -y $PKGS); then \
        echo "live Debian mirror incomplete; falling back to archive.debian.org" >&2; \
        printf '%s\n' \
            'deb http://archive.debian.org/debian bullseye main' \
            'deb http://archive.debian.org/debian-security bullseye-security main' \
            > /etc/apt/sources.list; \
        for f in /etc/apt/sources.list.d/*.list; do \
            [ -f "$f" ] || continue; \
            sed -i -E 's,https?://(deb|security)\.debian\.org/debian-security,http://archive.debian.org/debian-security,g; s,https?://deb\.debian\.org/debian,http://archive.debian.org/debian,g' "$f"; \
        done; \
        apt-get update || { \
            echo "archive.debian.org has no bullseye-security yet; dropping it" >&2; \
            sed -i '/debian-security/d' /etc/apt/sources.list; \
            for f in /etc/apt/sources.list.d/*.list; do \
                [ -f "$f" ] || continue; \
                sed -i '/debian-security/d' "$f"; \
            done; \
            apt-get update; \
        }; \
        apt-get install -y $PKGS; \
    fi; \
    rm -rf /var/lib/apt/lists/*

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
