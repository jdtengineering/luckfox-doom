#!/bin/sh
set -eu
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 root@BOARD binary-path wad-path" >&2
    exit 2
fi
target=$1
binary=$2
wad=$3
[ -f "$binary" ] && [ -f "$wad" ] || { echo "Missing binary or WAD" >&2; exit 1; }
base=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ssh "$target" 'mkdir -p /opt/doom'
scp -O "$binary" "$target:/opt/doom/doom-ascii.new"
scp -O "$wad" "$target:/opt/doom/doom1.wad"
scp -O "$base/src/.default.cfg" "$target:/opt/doom/.default.cfg.dist"
scp -O "$base/scripts/doom" "$base/scripts/doom-pixels" "$target:/usr/bin/"
ssh "$target" 'set -e; cd /opt/doom; chmod 755 doom-ascii.new /usr/bin/doom /usr/bin/doom-pixels; mv doom-ascii.new doom-ascii; test -f .default.cfg || cp .default.cfg.dist .default.cfg; sha256sum doom-ascii'
printf 'Play: ssh -t %s doom\n' "$target"
