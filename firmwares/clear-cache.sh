#!/usr/bin/env bash
# Clears arduino-cli's compiled-sketch cache.
#
# arduino-cli caches every compiled sketch under ~/.cache/arduino/sketches/,
# keyed by a hash of the sketch's source *path* -- not its name or content.
# That means moving or renaming a sketch folder (as happened when these
# firmwares moved from the repo root into firmwares/) leaves the old
# path's cache entry behind as orphaned clutter, since a new hash/entry
# gets created for the new path instead of reusing the old one.
#
# This script doesn't try to figure out which entries are stale -- it just
# clears the whole cache. That's simple and safe: the only cost is that the
# next `arduino-cli compile` of any sketch has to rebuild from scratch
# (slower once, then back to normal, since later compiles cache again).
#
# Usage: ./clear-cache.sh
set -euo pipefail

CACHE_DIR="$HOME/.cache/arduino/sketches"

if [ ! -d "$CACHE_DIR" ]; then
  echo "No cache directory found at $CACHE_DIR -- nothing to clear."
  exit 0
fi

echo "--- clearing $CACHE_DIR ---"
rm -rf "${CACHE_DIR:?}"/*
echo "--- done -- the next compile of any sketch will rebuild from scratch ---"
