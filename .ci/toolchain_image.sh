#!/usr/bin/env bash
#
# Puts the cubiboot-dev toolchain image in place for every `docker run ...
# cubiboot-dev` step in .github/workflows/ci.yml. Two ways to get there:
#
#   1. pull the prebuilt image published by .github/workflows/toolchain.yml,
#      pinned by digest in .ci/toolchain.digest -- no apt, no libogc2 compile,
#      and nothing in the run depends on Debian's mirrors still serving
#      bullseye;
#   2. build it here from .ci/Dockerfile, byte for byte the step CI has always
#      run.
#
# (2) is not a nicety. It is what keeps this repo buildable when the digest file
# is absent (the state this script ships in: behaviour identical to before),
# when GHCR is unreachable, and -- the case that matters -- when .ci/Dockerfile
# has been edited since the image was published. A pinned image is the WRONG
# toolchain then, so it is deliberately ignored rather than silently used, and
# the run says so. Deleting .ci/toolchain.digest is a complete rollback to the
# old behaviour, with no other change anywhere.
set -euo pipefail

cd "$(dirname "$0")/.."

DIGEST_FILE=.ci/toolchain.digest
dockerfile_hash="$(sha256sum .ci/Dockerfile | cut -d' ' -f1)"

build_locally() {
    echo ">> building the toolchain image from .ci/Dockerfile"
    # Retry: the Docker Hub base-image pull can hit transient i/o timeouts on
    # runners.
    for i in 1 2 3 4 5; do
        docker build -t cubiboot-dev - < .ci/Dockerfile && return 0
        echo "docker build attempt $i failed; retrying in 20s..." >&2
        sleep 20
    done
    echo "docker build failed after 5 attempts" >&2
    return 1
}

use_prebuilt() {
    local image="$1"
    for i in 1 2 3; do
        if docker pull "$image"; then
            docker tag "$image" cubiboot-dev
            echo ">> using prebuilt toolchain image $image"
            return 0
        fi
        echo "docker pull attempt $i failed; retrying in 10s..." >&2
        sleep 10
    done
    return 1
}

if [ -f "$DIGEST_FILE" ]; then
    image="$(sed -n 's/^image=//p' "$DIGEST_FILE" | tail -n1)"
    recorded="$(sed -n 's/^dockerfile_sha256=//p' "$DIGEST_FILE" | tail -n1)"

    if [ -z "$image" ] || [ -z "$recorded" ]; then
        echo "::warning::$DIGEST_FILE is missing an image= or dockerfile_sha256= line; building the toolchain image from .ci/Dockerfile instead"
    elif [ "$recorded" != "$dockerfile_hash" ]; then
        echo "::warning::.ci/Dockerfile has changed since $image was published (recorded $recorded, current $dockerfile_hash), so the pinned image is the wrong toolchain and is being ignored. This build is correct but slower. Run the 'Toolchain image' workflow and commit the digest it prints to go back to the prebuilt image."
    elif use_prebuilt "$image"; then
        exit 0
    else
        echo "::warning::could not pull $image; building the toolchain image from .ci/Dockerfile instead"
    fi
fi

build_locally
