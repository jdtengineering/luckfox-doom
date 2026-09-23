#!/bin/sh
# Usage: TOOLCHAIN=/path/to/arm-rockchip830-linux-uclibcgnueabihf ./scripts/build-luckfox.sh
set -eu
: "${TOOLCHAIN:?Set TOOLCHAIN to the Luckfox SDK toolchain directory}"
cd "$(dirname "$0")/.."
cc="$TOOLCHAIN/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc"
make -j"${JOBS:-4}" CC="$cc" CFLAGS='-O3 -flto -Wall -D_GNU_SOURCE -DNORMALUNIX -DLINUX -DVERSION=0.3.2 -std=c99'
"$TOOLCHAIN/bin/arm-rockchip830-linux-uclibcgnueabihf-strip" _unix/game/doom-ascii
